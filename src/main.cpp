#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"
#include "FS.h"
#include "SPIFFS.h"

// ===== CONFIG =====
// Still 40 kHz target
static const float BASE_FREQUENCY_HZ = 40000.0f;     // 40 kHz
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;  // GPIO 25
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;  // GPIO 26

// --- TIMER SETUP FOR PHASED SQUARE ---
// APB = 80 MHz, divider = 10 => 8 MHz timer clock
#define TIMER_DIVIDER      10
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)  // 8 MHz

// 40 kHz period = 8e6 / 4e4 = 200 timer ticks
const uint32_t PERIOD_TICKS       = TIMER_TICKS_PER_S / (uint32_t)BASE_FREQUENCY_HZ; // 200
const uint32_t HALF_PERIOD_TICKS  = PERIOD_TICKS / 2;                                // 100
// We subdivide one period into 8 steps -> 25 ticks / step -> 45° per step
const uint32_t STEP_TICKS         = PERIOD_TICKS / 8;                                // 25

// Wi-Fi AP credentials
const char *AP_SSID     = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);

// phaseCounter runs from 0..PERIOD_TICKS-1
volatile uint32_t phaseCounter = 0;
// phaseTicks is the phase offset we apply to CH2
volatile int32_t phaseTicks = 0;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  // Advance phaseCounter by STEP_TICKS
  phaseCounter += STEP_TICKS;
  if (phaseCounter >= PERIOD_TICKS) {
    phaseCounter -= PERIOD_TICKS;  // wrap around one period
  }

  // Channel 1: high for first half of the period
  bool ch1_high = (phaseCounter < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH1, ch1_high ? 255 : 0);

  // Channel 2: same waveform, but shifted by phaseTicks
  uint32_t shifted = phaseCounter + phaseTicks;
  if (shifted >= PERIOD_TICKS) {
    shifted -= PERIOD_TICKS;
  }
  bool ch2_high = (shifted < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH2, ch2_high ? 255 : 0);
}

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  // normalize to [0, 360)
  while (degrees < 0.0f)    degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  // convert degrees → ticks in one full 40 kHz cycle
  float ticksPerCycle = (float)PERIOD_TICKS;  // 200
  int32_t newTicks = (int32_t)((degrees / 360.0f) * ticksPerCycle + 0.5f);

  // update atomically with respect to ISR
  noInterrupts();
  phaseTicks = newTicks;
  interrupts();

  float us = newTicks * (1e6f / (float)TIMER_TICKS_PER_S); // ticks -> µs
  Serial.printf("Phase = %.1f° → %ld ticks (%.2f µs)\n",
                degrees, (long)newTicks, us);
}

// ===== HTTP HANDLERS =====
void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "index.html not found on SPIFFS");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleSetPhase() {
  if (!server.hasArg("deg")) {
    server.send(400, "application/json", "{\"error\":\"missing deg param\"}");
    return;
  }

  float deg = server.arg("deg").toFloat();
  setPhaseDegrees(deg);

  String json = "{\"success\":true,\"phase\":" + String(deg, 1) + "}";
  server.send(200, "application/json", json);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ONDA 40 kHz Phase Generator (multi-step) ===");

  // --- DAC ---
  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);
  setPhaseDegrees(0.0f);  // start in-phase

  // --- SPIFFS ---
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
    return;
  }
  Serial.println("SPIFFS mounted successfully.");

  // --- Wi-Fi AP ---
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("Wi-Fi started. SSID: %s, IP: %s\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());

  // --- HTTP server ---
  server.on("/", handleRoot);
  server.on("/set_phase", handleSetPhase);
  server.begin();
  Serial.println("HTTP server started on port 80.");

  // --- Timer ---
  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  // Alarm every STEP_TICKS, not half-period
  timerAlarmWrite(g_timer, STEP_TICKS, true);
  timerAlarmEnable(g_timer);
  Serial.printf("Timer running with period %u ticks (step %u)\n",
                PERIOD_TICKS, STEP_TICKS);
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}

