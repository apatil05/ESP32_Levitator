#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp32-hal-timer.h"
#include "FS.h"
#include "SPIFFS.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"

// ===== CONFIG =====
static const float BASE_FREQUENCY_HZ = 40000.0f;     // 40 kHz

// ESP32-C3 GPIO pins (avoid GPIO18/19 USB, GPIO9/10 flash)
const int GPIO_CH1 = 2;  // Channel 1 output pin
const int GPIO_CH2 = 3;  // Channel 2 output pin
// Note: Using timer-based GPIO control for square waves (more precise than PWM for phase shifting)

// Timer for phase-shifted square wave generation
// ESP32-C3: APB clock is 80 MHz (already defined in SDK as APB_CLK_FREQ)
#define TIMER_DIVIDER      2
#ifndef APB_CLK_FREQ
#define APB_CLK_FREQ       80000000
#endif
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)
const uint32_t PERIOD_TICKS = TIMER_TICKS_PER_S / (uint32_t)BASE_FREQUENCY_HZ;  // 1000 ticks
const uint32_t HALF_PERIOD_TICKS = PERIOD_TICKS / 2;  // 500 ticks = 12.5 µs at 40 MHz

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);

volatile int32_t phaseTicks = 0;   // Phase offset in ticks
volatile bool level_ch1 = false;
volatile bool level_ch2 = false;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  // Track position: 0 = first half (high), HALF_PERIOD_TICKS = second half (low)
  static volatile uint32_t position = 0;
  
  // Channel 1: Toggle directly (square wave at 40 kHz)
  level_ch1 = !level_ch1;
  gpio_set_level((gpio_num_t)GPIO_CH1, level_ch1 ? 1 : 0);
  
  // Channel 2: Apply phase offset to position
  int32_t ch2_pos = (int32_t)position + phaseTicks;
  // Normalize to 0-PERIOD_TICKS range (handle negative values)
  if (ch2_pos < 0) ch2_pos += PERIOD_TICKS;
  if (ch2_pos >= PERIOD_TICKS) ch2_pos -= PERIOD_TICKS;
  
  bool ch2_should_be_high = (ch2_pos < HALF_PERIOD_TICKS);
  if (ch2_should_be_high != level_ch2) {
    level_ch2 = ch2_should_be_high;
    gpio_set_level((gpio_num_t)GPIO_CH2, level_ch2 ? 1 : 0);
  }
  
  // Advance position: toggle between 0 and HALF_PERIOD_TICKS
  position = (position == 0) ? HALF_PERIOD_TICKS : 0;
}

// ===== PHASE CONTROL =====
void setPhaseDegrees(float degrees) {
  while (degrees < 0) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;

  // Convert degrees → ticks in one full cycle
  // PERIOD_TICKS = 1000, so 360° = 1000 ticks
  uint32_t newTicks = (uint32_t)((degrees / 360.0f) * (float)PERIOD_TICKS + 0.5f);

  // Update atomically with respect to ISR
  noInterrupts();
  phaseTicks = (int32_t)newTicks;
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
  Serial.println("\n=== ONDA 40 kHz Phase Generator (ESP32-C3 PWM) ===");

  // --- GPIO Setup ---
  gpio_reset_pin((gpio_num_t)GPIO_CH1);
  gpio_reset_pin((gpio_num_t)GPIO_CH2);
  gpio_set_direction((gpio_num_t)GPIO_CH1, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)GPIO_CH2, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)GPIO_CH1, 0);
  gpio_set_level((gpio_num_t)GPIO_CH2, 0);
  Serial.printf("GPIO %d (CH1) and GPIO %d (CH2) configured for square wave output\n", 
                 GPIO_CH1, GPIO_CH2);
  
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

  // --- Timer for square wave generation ---
  // Fire timer every HALF_PERIOD_TICKS (500 ticks = 12.5 µs) to toggle square wave
  // This gives exactly 40 kHz: 40 MHz / 1000 ticks = 40,000 Hz
  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  if (!g_timer) {
    Serial.println("ERROR: Failed to initialize timer!");
    return;
  }
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, HALF_PERIOD_TICKS, true);  // Toggle every half period
  timerAlarmEnable(g_timer);
  Serial.printf("Timer configured: %u MHz clock, %u ticks per half-period\n",
                TIMER_TICKS_PER_S / 1000000, HALF_PERIOD_TICKS);
  Serial.printf("Expected frequency: %.1f Hz (target: %.1f Hz)\n",
                (float)TIMER_TICKS_PER_S / (2.0f * HALF_PERIOD_TICKS), BASE_FREQUENCY_HZ);
  Serial.println("40 kHz square waves active on both channels.");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}
