#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/ledc.h"
#include "FS.h"
#include "SPIFFS.h"

// ===== CONFIG =====
static const float BASE_FREQUENCY_HZ = 40000.0f;     // 40 kHz

// ESP32 GPIO pins (using original DAC pins, but with PWM)
const int GPIO_CH1 = 25;  // Channel 1 output pin (was DAC_CHANNEL_1)
const int GPIO_CH2 = 26;  // Channel 2 output pin (was DAC_CHANNEL_2)

// LEDC configuration for PWM square waves
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE  // Use high-speed mode for accurate 40 kHz
#define LEDC_CHANNEL_CH1        LEDC_CHANNEL_0
#define LEDC_CHANNEL_CH2        LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT  // 13-bit resolution (8192 steps)
#define LEDC_DUTY_50_PERCENT    (8192 / 2)  // 50% duty cycle for square wave
#define LEDC_MAX_DUTY           8192

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
WebServer server(80);
uint32_t current_phase_hpoint = 0;  // Phase offset in hpoint units

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;
  
  // Convert degrees to hpoint (phase offset)
  // hpoint controls when the PWM signal goes high within the period
  // For 50% duty cycle, hpoint can range from 0 to (MAX_DUTY - DUTY_50_PERCENT)
  // Phase 0° = hpoint 0, Phase 180° = hpoint at half period
  uint32_t max_hpoint = LEDC_MAX_DUTY - LEDC_DUTY_50_PERCENT;
  current_phase_hpoint = (uint32_t)((degrees / 360.0f) * (float)max_hpoint + 0.5f);
  
  // Reconfigure Channel 2 with new hpoint (hpoint can only be set during channel config)
  ledc_channel_config_t channel2_config;
  channel2_config.gpio_num = GPIO_CH2;
  channel2_config.speed_mode = LEDC_MODE;
  channel2_config.channel = LEDC_CHANNEL_CH2;
  channel2_config.timer_sel = LEDC_TIMER;
  channel2_config.duty = LEDC_DUTY_50_PERCENT;
  channel2_config.hpoint = current_phase_hpoint;
  channel2_config.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_config(&channel2_config);
  
  Serial.printf("Phase set to: %.1f° (hpoint: %u)\n", degrees, current_phase_hpoint);
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
  Serial.println("\n=== ONDA 40 kHz PWM Square Wave Generator (ESP32) ===");

  // --- LEDC PWM Setup ---
  // Configure timer (shared by both channels for synchronization)
  ledc_timer_config_t timer_config;
  timer_config.speed_mode = LEDC_MODE;
  timer_config.timer_num = LEDC_TIMER;
  timer_config.duty_resolution = LEDC_DUTY_RES;
  timer_config.freq_hz = (uint32_t)BASE_FREQUENCY_HZ;  // 40 kHz
  timer_config.clk_cfg = LEDC_AUTO_CLK;
  esp_err_t timer_result = ledc_timer_config(&timer_config);
  if (timer_result != ESP_OK) {
    Serial.printf("ERROR: Timer config failed: %d\n", timer_result);
    return;
  }
  
  // Verify actual frequency (LEDC may adjust it slightly)
  uint32_t actual_freq = ledc_get_freq(LEDC_MODE, LEDC_TIMER);
  Serial.printf("Requested frequency: %.1f Hz, Actual frequency: %lu Hz\n", 
                BASE_FREQUENCY_HZ, actual_freq);

  // Configure Channel 1 (GPIO 25) - Reference channel (no phase offset)
  ledc_channel_config_t channel1_config;
  channel1_config.gpio_num = GPIO_CH1;
  channel1_config.speed_mode = LEDC_MODE;
  channel1_config.channel = LEDC_CHANNEL_CH1;
  channel1_config.timer_sel = LEDC_TIMER;
  channel1_config.duty = LEDC_DUTY_50_PERCENT;
  channel1_config.hpoint = 0;  // Start at beginning (0° phase)
  channel1_config.intr_type = LEDC_INTR_DISABLE;
  esp_err_t ch1_result = ledc_channel_config(&channel1_config);
  if (ch1_result != ESP_OK) {
    Serial.printf("ERROR: Channel 1 config failed: %d\n", ch1_result);
    return;
  }
  Serial.printf("Channel 1 (GPIO %d): PWM at %.1f Hz, 50%% duty, hpoint=0\n", 
                GPIO_CH1, BASE_FREQUENCY_HZ);

  // Configure Channel 2 (GPIO 26) - Phase-shiftable channel
  ledc_channel_config_t channel2_config;
  channel2_config.gpio_num = GPIO_CH2;
  channel2_config.speed_mode = LEDC_MODE;
  channel2_config.channel = LEDC_CHANNEL_CH2;
  channel2_config.timer_sel = LEDC_TIMER;  // Same timer = synchronized
  channel2_config.duty = LEDC_DUTY_50_PERCENT;
  channel2_config.hpoint = 0;  // Will be updated by setPhaseDegrees()
  channel2_config.intr_type = LEDC_INTR_DISABLE;
  esp_err_t ch2_result = ledc_channel_config(&channel2_config);
  if (ch2_result != ESP_OK) {
    Serial.printf("ERROR: Channel 2 config failed: %d\n", ch2_result);
    return;
  }
  Serial.printf("Channel 2 (GPIO %d): PWM at %.1f Hz, 50%% duty, phase-shiftable\n", 
                GPIO_CH2, BASE_FREQUENCY_HZ);

  // Initialize phase to 0°
  setPhaseDegrees(0.0f);

  Serial.println("PWM square waves active on both channels (synchronized)!");

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
