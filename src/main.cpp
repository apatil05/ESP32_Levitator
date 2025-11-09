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

// Timer: 80 MHz / 2 = 40 MHz → 25 ns tick
#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)
const uint32_t HALF_PERIOD_TICKS = 500;  // 12.5 µs × 40 MHz = 500 ticks

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);

volatile bool level_ch1 = false;
volatile bool level_ch2 = false;
volatile int32_t phaseTicks = 0;
// phase offset in ticks
uint32_t tickCounter = 0;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  tickCounter += HALF_PERIOD_TICKS;

  // Channel 1 toggles every interrupt
  level_ch1 = !level_ch1;
  dac_output_voltage(DAC_CH1, level_ch1 ? 255 : 0);

  // Channel 2 toggles based on phase offset
  int32_t offsetCounter = (tickCounter + phaseTicks) % (2 * HALF_PERIOD_TICKS);
  bool ch2_state = offsetCounter < HALF_PERIOD_TICKS;
  dac_output_voltage(DAC_CH2, ch2_state ? 255 : 0);
}

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  float ticksPerCycle = 2.0f * (float)HALF_PERIOD_TICKS;
  phaseTicks = (int32_t)((degrees / 360.0f) * ticksPerCycle);

  Serial.printf("Phase = %.1f° → %ld ticks (%.2f µs)\n",
                degrees, phaseTicks, phaseTicks * 0.025f);
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
  Serial.println("\n=== ONDA 40 kHz Phase Generator ===");

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
  timerAlarmWrite(g_timer, HALF_PERIOD_TICKS, true);
  timerAlarmEnable(g_timer);
  Serial.println("Timer running at 40 kHz square-wave rate.");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}
