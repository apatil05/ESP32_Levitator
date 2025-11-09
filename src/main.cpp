#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"
#include "FS.h"
#include "SPIFFS.h"

// ===== CONFIG =====
static const float BASE_FREQUENCY_HZ = 40000.0f;     // 40 kHz
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;  // GPIO 25
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;  // GPIO 26

// Timer: 80 MHz / 5 = 16 MHz → 62.5 ns per tick
#define TIMER_DIVIDER      5
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)

// Derived timing
const uint32_t PERIOD_TICKS      = TIMER_TICKS_PER_S / (uint32_t)BASE_FREQUENCY_HZ;  // 400 ticks
const uint32_t HALF_PERIOD_TICKS = PERIOD_TICKS / 2;                                 // 200
const uint32_t STEP_TICKS        = PERIOD_TICKS / 32;                                // 32 steps ≈ 11.25°

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);
volatile uint32_t phaseAcc = 0;
volatile int32_t phaseOffsetTicks = 0;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  phaseAcc += STEP_TICKS;
  if (phaseAcc >= PERIOD_TICKS) phaseAcc -= PERIOD_TICKS;

  // CH1 — reference
  bool ch1_high = (phaseAcc < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH1, ch1_high ? 255 : 0);

  // CH2 — phase shifted
  uint32_t shifted = phaseAcc + phaseOffsetTicks;
  if (shifted >= PERIOD_TICKS) shifted -= PERIOD_TICKS;
  bool ch2_high = (shifted < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH2, ch2_high ? 255 : 0);
}

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0.0f)    degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  float ticksPerCycle = (float)PERIOD_TICKS;  // 400
  int32_t newOffset = (int32_t)((degrees / 360.0f) * ticksPerCycle + 0.5f);

  noInterrupts();
  phaseOffsetTicks = newOffset;
  interrupts();

  float us = newOffset * (1e6f / (float)TIMER_TICKS_PER_S);
  Serial.printf("Phase = %.1f° → %ld ticks (%.2f µs)\n", degrees, newOffset, us);
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
  Serial.println("\n=== ONDA 40 kHz Phase Generator (Smooth) ===");

  // --- DAC ---
  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);
  setPhaseDegrees(0.0f);

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
  timerAlarmWrite(g_timer, STEP_TICKS, true);   // trigger every fine step
  timerAlarmEnable(g_timer);
  Serial.println("Timer running with fine phase control.");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}

