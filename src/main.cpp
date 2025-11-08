#include <Arduino.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// ===== CONFIG =====
static const float BASE_FREQUENCY_HZ = 40000.0f;     // 40 kHz
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;  // GPIO 25
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;  // GPIO 26

// Timer: 80 MHz / 2 = 40 MHz → 25 ns tick
#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)

const uint32_t HALF_PERIOD_TICKS = 500;  // 12.5 µs × 40 MHz = 500 ticks

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
volatile bool level_ch1 = false;
volatile bool level_ch2 = false;
volatile int32_t phaseTicks = 0;  // phase offset in ticks (set via setPhaseDegrees)
uint32_t tickCounter = 0;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  tickCounter += HALF_PERIOD_TICKS;

  // toggle CH1 every interrupt
  level_ch1 = !level_ch1;
  dac_output_voltage(DAC_CH1, level_ch1 ? 255 : 0);

  // compute CH2 toggle relative to CH1
  int32_t offsetCounter = (tickCounter + phaseTicks) % (2 * HALF_PERIOD_TICKS);
  bool ch2_state = offsetCounter < HALF_PERIOD_TICKS;
  dac_output_voltage(DAC_CH2, ch2_state ? 255 : 0);
}

// ===== HELPERS =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  float ticksPerCycle = 2.0f * (float)HALF_PERIOD_TICKS;
  phaseTicks = (int32_t)((degrees / 360.0f) * ticksPerCycle);

  Serial.printf("Phase = %.1f° → %ld ticks (%.2f µs)\n",
                degrees, phaseTicks, phaseTicks * 0.025f);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ONDA 40 kHz Phase Generator ===");

  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);
  setPhaseDegrees(0.0f);

  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, HALF_PERIOD_TICKS, true);  // 12.5 µs
  timerAlarmEnable(g_timer);

  Serial.println("Timer running at 40 kHz square-wave rate");
}

// ===== LOOP =====
void loop() {
  // sweep for demonstration
  static float deg = 0;
  static unsigned long last = 0;
  if (millis() - last > 400) {
    setPhaseDegrees(deg);
    deg += 30.0f;
    if (deg >= 360.0f) deg = 0;
    last = millis();
  }
}
