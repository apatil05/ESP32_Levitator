#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/ledc.h"
#include "FS.h"
#include "SPIFFS.h"

// ===== CONFIG =====
#define FREQUENCY_HZ            40000UL    // 40 kHz exactly
#define GPIO_CH1                25          // Channel 1 (GPIO 25)
#define GPIO_CH2                26          // Channel 2 (GPIO 26)

// LEDC PWM Configuration
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_CHANNEL_CH1        LEDC_CHANNEL_0
#define LEDC_CHANNEL_CH2        LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_DUTY_50_PERCENT    4096        // 50% duty (8192/2)

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
WebServer server(80);

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  // Normalize to 0-360
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;
  
  // Convert phase to hpoint: 360° = 8192 steps (full period)
  uint32_t hpoint = (uint32_t)((degrees / 360.0f) * 8192.0f + 0.5f);
  
  // Reconfigure Channel 2 with new phase
  ledc_channel_config_t cfg;
  cfg.gpio_num = GPIO_CH2;
  cfg.speed_mode = LEDC_MODE;
  cfg.channel = LEDC_CHANNEL_CH2;
  cfg.timer_sel = LEDC_TIMER;
  cfg.duty = LEDC_DUTY_50_PERCENT;
  cfg.hpoint = hpoint;
  cfg.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_config(&cfg);
  
  Serial.printf("Phase: %.1f°\n", degrees);
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
  Serial.println("\n=== 40 kHz PWM Square Wave Generator ===");

  // Configure shared timer for both channels (ensures synchronization)
  ledc_timer_config_t timer_cfg;
  timer_cfg.speed_mode = LEDC_MODE;
  timer_cfg.timer_num = LEDC_TIMER;
  timer_cfg.duty_resolution = LEDC_DUTY_RES;
  timer_cfg.freq_hz = FREQUENCY_HZ;
  timer_cfg.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timer_cfg);
  
  // Verify frequency
  uint32_t actual_freq = ledc_get_freq(LEDC_MODE, LEDC_TIMER);
  Serial.printf("Frequency: %lu Hz (requested %lu Hz)\n", actual_freq, FREQUENCY_HZ);

  // Channel 1: Reference (GPIO 25)
  ledc_channel_config_t ch1_cfg;
  ch1_cfg.gpio_num = GPIO_CH1;
  ch1_cfg.speed_mode = LEDC_MODE;
  ch1_cfg.channel = LEDC_CHANNEL_CH1;
  ch1_cfg.timer_sel = LEDC_TIMER;
  ch1_cfg.duty = LEDC_DUTY_50_PERCENT;
  ch1_cfg.hpoint = 0;
  ch1_cfg.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_config(&ch1_cfg);
  Serial.printf("Channel 1 (GPIO %d): Active\n", GPIO_CH1);

  // Channel 2: Phase-shiftable (GPIO 26)
  ledc_channel_config_t ch2_cfg;
  ch2_cfg.gpio_num = GPIO_CH2;
  ch2_cfg.speed_mode = LEDC_MODE;
  ch2_cfg.channel = LEDC_CHANNEL_CH2;
  ch2_cfg.timer_sel = LEDC_TIMER;
  ch2_cfg.duty = LEDC_DUTY_50_PERCENT;
  ch2_cfg.hpoint = 0;
  ch2_cfg.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_config(&ch2_cfg);
  Serial.printf("Channel 2 (GPIO %d): Active\n", GPIO_CH2);

  Serial.println("40 kHz square waves active!");

  // --- SPIFFS (for web interface) ---
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed (web interface disabled)");
  } else {
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
  }
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}
