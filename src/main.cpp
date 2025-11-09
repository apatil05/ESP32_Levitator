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
// ESP32-C3: APB clock is 80 MHz
#define TIMER_DIVIDER      2
#define APB_CLK_FREQ       80000000
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)
const uint32_t PERIOD_TICKS = TIMER_TICKS_PER_S / (uint32_t)BASE_FREQUENCY_HZ;  // 1000 ticks
const uint32_t HALF_PERIOD_TICKS = PERIOD_TICKS / 2;  // 500 ticks
const uint32_t TIMER_STEP = 50;  // 50 ticks = 1.25 µs at 40 MHz (for smooth phase control)

// Wi-Fi AP credentials
const char *AP_SSID = "ONDA";
const char *AP_PASSWORD = "levitate123";

// ===== GLOBALS =====
hw_timer_t *g_timer = nullptr;
WebServer server(80);

volatile uint32_t position = 0;    // Position in period (0 to PERIOD_TICKS-1)
volatile int32_t phaseTicks = 0;   // Phase offset in ticks
volatile bool level_ch1 = false;
volatile bool level_ch2 = false;

// ===== ISR =====
void IRAM_ATTR onTimerISR() {
  // Increment position in the period (wraps automatically)
  position = (position + TIMER_STEP) % PERIOD_TICKS;
  
  // Channel 1: High for first half (0-499), low for second half (500-999)
  bool ch1_state = (position < HALF_PERIOD_TICKS);
  
  // Channel 2: Same waveform but phase-shifted
  uint32_t ch2_position = (position + phaseTicks) % PERIOD_TICKS;
  bool ch2_state = (ch2_position < HALF_PERIOD_TICKS);
  
  // Update GPIO register only if states changed (optimize for performance)
  bool need_update = false;
  uint32_t gpio_reg = REG_READ(GPIO_OUT_REG);
  
  if (ch1_state != level_ch1) {
    level_ch1 = ch1_state;
    need_update = true;
    if (level_ch1) {
      gpio_reg |= (1UL << GPIO_CH1);
    } else {
      gpio_reg &= ~(1UL << GPIO_CH1);
    }
  }
  
  if (ch2_state != level_ch2) {
    level_ch2 = ch2_state;
    need_update = true;
    if (level_ch2) {
      gpio_reg |= (1UL << GPIO_CH2);
    } else {
      gpio_reg &= ~(1UL << GPIO_CH2);
    }
  }
  
  if (need_update) {
    REG_WRITE(GPIO_OUT_REG, gpio_reg);
  }
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
  pinMode(GPIO_CH1, OUTPUT);
  pinMode(GPIO_CH2, OUTPUT);
  digitalWrite(GPIO_CH1, LOW);
  digitalWrite(GPIO_CH2, LOW);
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
  // Fire timer every 50 ticks for smooth phase control
  // Using 50 ticks = 1.25 µs intervals = 800 kHz interrupt rate
  // This gives 20 steps per period = 18° phase resolution (good enough for smooth control)
  g_timer = timerBegin(0, TIMER_DIVIDER, true);
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, TIMER_STEP, true);
  timerAlarmEnable(g_timer);
  Serial.printf("Timer: %u MHz, Period: %u ticks, Step: %u ticks\n",
                TIMER_TICKS_PER_S / 1000000, PERIOD_TICKS, TIMER_STEP);
  Serial.printf("Phase resolution: %.1f° per step\n", 360.0f / (PERIOD_TICKS / TIMER_STEP));
  Serial.println("40 kHz square waves active on both channels.");
}

// ===== LOOP =====
void loop() {
  server.handleClient();
}
