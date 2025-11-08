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
volatile uint8_t g_level = 0;  // 0 or 255 (square)

// ---------- ISR ----------
void IRAM_ATTR onTimerISR() {
  // Toggle between low and high
  g_level = g_level ? 0 : 255;

  // Output same value on both DACs
  dac_output_voltage(DAC_CH1, g_level);
  dac_output_voltage(DAC_CH2, g_level);
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

void loop() {
  // Nothing here; waveform generated entirely by the timer ISR
}

