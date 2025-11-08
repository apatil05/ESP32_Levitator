#include <Arduino.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// Config
static const float BASE_FREQUENCY_HZ = 40000.0f;   // target output frequency
static const float SAMPLES_PER_CYCLE = 8.0f;       // 8 DAC samples per sine period
#define TIMER_DIVIDER      80                      // 1 µs per tick (80 MHz / 80)
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)

// DAC channels
#define DAC1 DAC_CHANNEL_1   // GPIO25
#define DAC2 DAC_CHANNEL_2   // GPIO26

// Globals
hw_timer_t *timer0 = nullptr;
volatile uint32_t phase_acc = 0;
volatile uint32_t phase_step = 0;
uint8_t sine_lut[256];

// --- ISR: identical DAC outputs ---
void IRAM_ATTR onTimerISR() {
  uint16_t idx = (phase_acc >> 8) & 0xFF;
  uint8_t v = sine_lut[idx];

  dac_output_voltage(DAC1, v);
  dac_output_voltage(DAC2, v);

  phase_acc += phase_step;
}

// --- helpers ---
void initSineLUT() {
  for (int i = 0; i < 256; ++i) {
    float s = sinf(2.0f * PI * i / 256.0f);
    sine_lut[i] = (uint8_t)((s + 1.0f) * 127.5f);
  }
}

void initTimerAndDAC() {
  dac_output_enable(DAC1);
  dac_output_enable(DAC2);

  float desired_sample_rate = BASE_FREQUENCY_HZ * SAMPLES_PER_CYCLE;
  uint32_t ticks_per_sample = (uint32_t)((float)TIMER_TICKS_PER_S / desired_sample_rate + 0.5f);
  if (ticks_per_sample == 0) ticks_per_sample = 1;

  float actual_sample_rate = (float)TIMER_TICKS_PER_S / (float)ticks_per_sample;
  phase_step = (uint32_t)((BASE_FREQUENCY_HZ * 65536.0f / actual_sample_rate) + 0.5f);

  Serial.println("=== ONDA DAC TEST ===");
  Serial.printf("Freq: %.1f Hz  |  SampleRate: %.1f Hz  |  Ticks/sample: %lu\n",
                BASE_FREQUENCY_HZ, actual_sample_rate, ticks_per_sample);

  timer0 = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(timer0, &onTimerISR, true);
  timerAlarmWrite(timer0, ticks_per_sample, true);
  timerAlarmEnable(timer0);
}

// --- setup/loop ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  initSineLUT();
  initTimerAndDAC();
}

void loop() {
  // nothing — DAC runs in ISR
}
