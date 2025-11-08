#include <Arduino.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// ========================== CONFIG ==========================
#define DAC1 DAC_CHANNEL_1   // GPIO25
#define DAC2 DAC_CHANNEL_2   // GPIO26

const float TARGET_FREQ_HZ     = 40000.0f; // 40 kHz output
const uint16_t SAMPLES_PER_CYC = 64;       // more samples = smoother sine
#define TIMER_DIVIDER      80              // 80MHz / 80 = 1 MHz tick (1 µs)
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)

// Choose waveform type
enum WaveType { WAVE_SINE, WAVE_SQUARE };
WaveType waveMode = WAVE_SINE;  // change to WAVE_SQUARE for square output

// ========================== GLOBALS ==========================
hw_timer_t *timer0 = nullptr;
volatile uint16_t sampleIndex = 0;
uint8_t sineLUT[SAMPLES_PER_CYC];
uint32_t ticksPerSample = 0;

// ========================== ISR ==========================
void IRAM_ATTR onTimerISR() {
  uint8_t val;

  if (waveMode == WAVE_SINE) {
    val = sineLUT[sampleIndex];
  } else {
    // Square wave: half high, half low
    val = (sampleIndex < (SAMPLES_PER_CYC / 2)) ? 255 : 0;
  }

  dac_output_voltage(DAC1, val);
  dac_output_voltage(DAC2, val);

  sampleIndex++;
  if (sampleIndex >= SAMPLES_PER_CYC)
    sampleIndex = 0;
}

// ========================== HELPERS ==========================
void buildSineTable() {
  for (int i = 0; i < SAMPLES_PER_CYC; i++) {
    float theta = 2.0f * PI * (float)i / (float)SAMPLES_PER_CYC;
    float s = sinf(theta);
    sineLUT[i] = (uint8_t)((s + 1.0f) * 127.5f); // map [-1,1] -> [0,255]
  }
}

void initTimerDAC() {
  dac_output_enable(DAC1);
  dac_output_enable(DAC2);

  float desiredSampleRate = TARGET_FREQ_HZ * SAMPLES_PER_CYC;
  ticksPerSample = (uint32_t)((float)TIMER_TICKS_PER_S / desiredSampleRate + 0.5f);
  if (ticksPerSample == 0) ticksPerSample = 1;

  float actualSampleRate = (float)TIMER_TICKS_PER_S / (float)ticksPerSample;
  float actualFreq = actualSampleRate / (float)SAMPLES_PER_CYC;

  Serial.println("=== DAC CONFIG ===");
  Serial.printf("Waveform: %s\n", waveMode == WAVE_SINE ? "SINE" : "SQUARE");
  Serial.printf("Target Frequency: %.2f Hz\n", TARGET_FREQ_HZ);
  Serial.printf("Actual Frequency: %.2f Hz\n", actualFreq);
  Serial.printf("Samples per cycle: %d\n", SAMPLES_PER_CYC);
  Serial.printf("Timer ticks/sample: %lu\n", ticksPerSample);
  Serial.println("==================");

  timer0 = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(timer0, &onTimerISR, true);
  timerAlarmWrite(timer0, ticksPerSample, true);
  timerAlarmEnable(timer0);
}

// ========================== SETUP / LOOP ==========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ONDA Precision DAC Generator ===");

  buildSineTable();
  initTimerDAC();
}

void loop() {
  // Nothing - runs entirely via timer interrupt
}

