#include <Arduino.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// ---------- CONFIG ----------

// Target frequency
static const float BASE_FREQUENCY_HZ = 40000.0f;  // 40 kHz

// DAC pins (fixed by ESP32)
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;  // GPIO25
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;  // GPIO26

// Timer config
// Use small divider so we get fine timing resolution:
// APB = 80 MHz, divider = 2 => timer clock = 40 MHz (25 ns per tick)
#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)  // 40,000,000 Hz

// ---------- GLOBALS ----------
hw_timer_t *g_timer = nullptr;
volatile uint8_t g_level_ch1 = 0;      // DAC1 level (reference, no phase shift)
volatile uint8_t g_level_ch2 = 0;      // DAC2 level (phase-shifted)
volatile uint32_t g_cycle_counter = 0; // Counter for phase delay calculation
volatile float g_phase_shift_degrees = 0.0f;  // Current phase shift in degrees

// ---------- ISR ----------
void IRAM_ATTR onTimerISR() {
  // Toggle DAC1 (reference - no phase shift, always toggles)
  g_level_ch1 = g_level_ch1 ? 0 : 255;
  dac_output_voltage(DAC_CH1, g_level_ch1);
  
  // For DAC2: Calculate phase-shifted output
  // Phase shift = delay in degrees
  // For square wave: delay = (phase_shift / 360°) * period
  // Since we toggle every half-period, delay in half-periods = (phase_shift / 360°) * 2
  // 0° = no delay (same as CH1)
  // 180° = half period delay (inverted)
  // 90° = quarter period delay (needs 0.5 half-periods)
  
  // Calculate how many half-periods to delay
  float phase_shift_normalized = g_phase_shift_degrees / 360.0f;  // 0.0 to 1.0
  float delay_half_periods = phase_shift_normalized * 2.0f;  // 0.0 to 2.0
  
  // For square waves, we can only delay by integer half-periods easily
  // So we round to nearest half-period for simplicity
  // This means: 0-45° ≈ 0, 45-135° ≈ 90°, 135-225° ≈ 180°, etc.
  uint32_t delay_half_periods_int = (uint32_t)(delay_half_periods + 0.5f);
  
  // Calculate the phase-shifted cycle position
  // If delay is 0: same as current cycle
  // If delay is 1: opposite of current cycle (90° shift)
  // If delay is 2: same as current cycle (180° shift = inverted)
  uint32_t shifted_cycle = (g_cycle_counter + delay_half_periods_int) % 2;
  
  // Set DAC2 level based on shifted cycle
  g_level_ch2 = shifted_cycle ? 255 : 0;
  dac_output_voltage(DAC_CH2, g_level_ch2);
  
  // Increment cycle counter (0 or 1 for square wave)
  g_cycle_counter = (g_cycle_counter + 1) % 2;
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ONDA 40 kHz Square Generator ===");

  // Enable DACs
  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);

  // Compute half-period in timer ticks:
  // frequency = 1 / T
  // For 40 kHz, period = 25 µs, half-period = 12.5 µs
  // timer_tick = 1 / 40 MHz = 25 ns
  // halfPeriodTicks = 12.5 µs / 25 ns = 500 ticks
  const uint32_t halfPeriodTicks = 500;

  float timerClockHz = (float)TIMER_TICKS_PER_S; // 40 MHz
  float actualHalfPeriod = (float)halfPeriodTicks / timerClockHz;  // seconds
  float actualFreq = 1.0f / (2.0f * actualHalfPeriod);             // Hz

  Serial.print("Timer clock: ");
  Serial.print(timerClockHz);
  Serial.println(" Hz");
  Serial.print("Half-period ticks: ");
  Serial.println(halfPeriodTicks);
  Serial.print("Actual frequency: ");
  Serial.print(actualFreq, 2);
  Serial.println(" Hz");

  // Setup timer 0
  g_timer = timerBegin(0, TIMER_DIVIDER, true);          // timer 0, divider, count up
  timerAttachInterrupt(g_timer, &onTimerISR, true);      // edge-triggered
  timerAlarmWrite(g_timer, halfPeriodTicks, true);       // interrupt every half-period
  timerAlarmEnable(g_timer);                             // start timer
}

// Function to set phase shift (called from main loop, not ISR)
void setPhaseShift(float phase_degrees) {
  // Normalize to 0-360 range
  while (phase_degrees < 0) phase_degrees += 360.0f;
  while (phase_degrees >= 360.0f) phase_degrees -= 360.0f;
  g_phase_shift_degrees = phase_degrees;
}

void loop() {
  // Phase shift iteration - cycles through different phase values
  static unsigned long last_update = 0;
  static float test_phase = 0.0f;
  
  if (millis() - last_update > 2000) {  // Update every 2 seconds
    test_phase += 45.0f;  // Increment by 45° each step
    if (test_phase >= 360.0f) test_phase = 0.0f;
    
    setPhaseShift(test_phase);
    
    Serial.print("Phase shift: ");
    Serial.print(test_phase);
    Serial.println("°");
    
    last_update = millis();
  }
}

