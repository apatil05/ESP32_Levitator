#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/ledc.h"
#include "FS.h"
#include "SPIFFS.h"

// ===== CONFIG =====
static const float BASE_FREQUENCY_HZ = 40.0f;     // 40 Hz

// ESP32-C3 GPIO pins
const int GPIO_CH1 = 2;  // Channel 1 output pin
const int GPIO_CH2 = 3;  // Channel 2 output pin

// LEDC configuration for PWM square waves
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL_CH1        LEDC_CHANNEL_0
#define LEDC_CHANNEL_CH2        LEDC_CHANNEL_1
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT  // 13-bit resolution (8192 steps)
#define LEDC_DUTY_50_PERCENT    (8192 / 2)  // 50% duty cycle for square wave

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
WebServer server(80);
volatile int32_t phaseTicks = 0;   // Phase offset in ticks (for future use)

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;
  
  // For now, just store the phase (PWM phase shifting requires more complex setup)
  phaseTicks = (int32_t)((degrees / 360.0f) * 1000.0f);
  
  Serial.printf("Phase set to: %.1f°\n", degrees);
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
  Serial.println("\n=== ONDA 40 Hz PWM Square Wave Generator (ESP32-C3) ===");

  // --- LEDC PWM Setup for Channel 1 ---
  ledc_timer_config_t timer_config = {
    .speed_mode = LEDC_MODE,
    .timer_num = LEDC_TIMER,
    .duty_resolution = LEDC_DUTY_RES,
    .freq_hz = (uint32_t)BASE_FREQUENCY_HZ,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_config);

  // Configure Channel 1 (GPIO 2)
  ledc_channel_config_t channel1_config = {
    .gpio_num = GPIO_CH1,
    .speed_mode = LEDC_MODE,
    .channel = LEDC_CHANNEL_CH1,
    .timer_sel = LEDC_TIMER,
    .duty = LEDC_DUTY_50_PERCENT,
    .hpoint = 0
  };
  ledc_channel_config(&channel1_config);
  Serial.printf("Channel 1 (GPIO %d): PWM configured at %.1f Hz, 50%% duty\n", 
                GPIO_CH1, BASE_FREQUENCY_HZ);

  // Configure Channel 2 (GPIO 3) - same timer, same frequency
  ledc_channel_config_t channel2_config = {
    .gpio_num = GPIO_CH2,
    .speed_mode = LEDC_MODE,
    .channel = LEDC_CHANNEL_CH2,
    .timer_sel = LEDC_TIMER,
    .duty = LEDC_DUTY_50_PERCENT,
    .hpoint = 0
  };
  ledc_channel_config(&channel2_config);
  Serial.printf("Channel 2 (GPIO %d): PWM configured at %.1f Hz, 50%% duty\n", 
                GPIO_CH2, BASE_FREQUENCY_HZ);

  Serial.println("PWM square waves active on both channels!");

  // --- SPIFFS (optional, for web interface) ---
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
