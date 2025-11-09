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
// APB = 80 MHz, divider = 2 => 40 MHz timer clock (better resolution)
#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)  // 40 MHz

// 40 kHz: period = 40e6 / 40e3 = 1000 ticks, half-period = 500 ticks
const uint32_t PERIOD_TICKS       = TIMER_TICKS_PER_S / (uint32_t)BASE_FREQUENCY_HZ; // 1000
const uint32_t HALF_PERIOD_TICKS  = PERIOD_TICKS / 2;                                // 500

// Wi-Fi AP credentials
const char *AP_SSID     = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);

// Position tracking for smooth phase shifts
volatile uint32_t position = 0;  // Position in period (0 to PERIOD_TICKS-1)
volatile uint32_t phaseTicks = 0;  // Phase offset in ticks (0-1000 for 0-360°)

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  // Increment position in the period (wraps automatically at PERIOD_TICKS)
  position = (position + 1) % PERIOD_TICKS;

  // Channel 1: High for first half of period (0-499), low for second half (500-999)
  bool ch1_state = (position < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH1, ch1_state ? 255 : 0);

  // Channel 2: Same waveform but phase-shifted
  // Calculate CH2's position: add phase offset and wrap around
  uint32_t ch2_position = (position + phaseTicks) % PERIOD_TICKS;
  bool ch2_state = (ch2_position < HALF_PERIOD_TICKS);
  dac_output_voltage(DAC_CH2, ch2_state ? 255 : 0);
}

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  // normalize to [0, 360)
  while (degrees < 0.0f)    degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  // convert degrees → ticks in one full 40 kHz cycle
  // PERIOD_TICKS = 1000, so 360° = 1000 ticks
  uint32_t newTicks = (uint32_t)((degrees / 360.0f) * (float)PERIOD_TICKS + 0.5f);

  // update atomically with respect to ISR
  noInterrupts();
  phaseTicks = newTicks;
  interrupts();

  // Only print occasionally to avoid Serial spam
  static float lastPrinted = -1.0f;
  if (fabs(degrees - lastPrinted) >= 5.0f) {
    float us = newTicks * (1e6f / (float)TIMER_TICKS_PER_S);
    Serial.printf("Phase = %.1f° → %u ticks (%.2f µs)\n", degrees, newTicks, us);
    lastPrinted = degrees;
  }
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
  // Fire timer frequently enough for smooth phase control
  // Using 50 ticks = 1.25 µs intervals = 800 kHz interrupt rate
  // This gives 20 steps per period = 18° resolution (good enough for smooth control)
  const uint32_t TIMER_STEP = 50;  // 50 ticks = 1.25 µs at 40 MHz
  
  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, TIMER_STEP, true);
  timerAlarmEnable(g_timer);
  Serial.printf("Timer: %u MHz, Period: %u ticks, Step: %u ticks\n",
                TIMER_TICKS_PER_S / 1000000, PERIOD_TICKS, TIMER_STEP);
  Serial.printf("Phase resolution: %.1f° per step\n", 360.0f / (PERIOD_TICKS / TIMER_STEP));
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}

