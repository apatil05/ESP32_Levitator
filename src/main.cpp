#include <Arduino.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// ---------- CONFIG ----------
static const float BASE_FREQUENCY_HZ = 40000.0f;  // 40 kHz
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;  // GPIO25
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;  // GPIO26

#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)  // 40 MHz timer clock

// ---------- GLOBALS ----------
hw_timer_t *g_timer = nullptr;
volatile bool level_ch1 = false;
volatile bool level_ch2 = false;
volatile uint32_t tickCounter = 0;

// phase shift in timer ticks (set via setPhaseDegrees)
volatile int32_t phaseTicks = 0;

// half period (ticks)
const uint32_t HALF_PERIOD_TICKS = 500; // 12.5 µs at 40 MHz

// ---------- ISR ----------
void IRAM_ATTR onTimerISR() {
  tickCounter++;

  // CH1 toggles every half period
  if (tickCounter % HALF_PERIOD_TICKS == 0) {
    level_ch1 = !level_ch1;
    dac_output_voltage(DAC_CH1, level_ch1 ? 255 : 0);
  }

  // CH2 toggles same frequency but offset by phaseTicks
  uint32_t ch2tick = (tickCounter + phaseTicks);
  if (ch2tick % HALF_PERIOD_TICKS == 0) {
    level_ch2 = !level_ch2;
    dac_output_voltage(DAC_CH2, level_ch2 ? 255 : 0);
  }
}

// ---------- HELPERS ----------
void setPhaseDegrees(float degrees) {
  // wrap 0–360
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  // convert degrees → ticks
  // 360° = one full cycle = 2 * HALF_PERIOD_TICKS
  float ticksPerCycle = 2.0f * (float)HALF_PERIOD_TICKS;
  phaseTicks = (int32_t)((degrees / 360.0f) * ticksPerCycle);

  Serial.print("Phase set to ");
  Serial.print(degrees);
  Serial.print("° (");
  Serial.print(phaseTicks);
  Serial.println(" timer ticks)");
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ONDA 40 kHz Phase-Shift Generator ===");

  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);

  setPhaseDegrees(0.0f);  // start in-phase

  // configure timer
  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, 1, true); // run ISR every 1 tick (25 ns)
  timerAlarmEnable(g_timer);

  Serial.println("Timer started at 40 MHz");
}

// ---------- LOOP ----------
void loop() {
  // demo: sweep 0°→180° phase slowly for testing
  static float phase = 0;
  static unsigned long last = 0;
  if (millis() - last > 500) { // every 0.5 s
    setPhaseDegrees(phase);
    phase += 15.0f;
    if (phase > 180.0f) phase = 0;
    last = millis();
  }
}
