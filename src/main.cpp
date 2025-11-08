#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "driver/dac.h"
#include "esp32-hal-timer.h"

// ------------- Config -------------
static const float BASE_FREQUENCY_HZ = 40000.0f;  // target ultrasonic frequency
static const float SAMPLES_PER_CYCLE = 8.0f;      // DAC samples per period

// WiFi AP config
const char *AP_SSID     = "ONDA";
const char *AP_PASSWORD = "levitate123";

// DAC pins: DAC_CHANNEL_1 = GPIO25, DAC_CHANNEL_2 = GPIO26 on ESP32
static const dac_channel_t DAC_CH1 = DAC_CHANNEL_1;
static const dac_channel_t DAC_CH2 = DAC_CHANNEL_2;

// Timer config
#define TIMER_DIVIDER      80                         // 80 MHz / 80 = 1 MHz timer tick (1 µs)
#define APB_CLK_FREQ       80000000                  // 80 MHz APB clock
#define TIMER_TICKS_PER_S  (APB_CLK_FREQ / TIMER_DIVIDER)

// ------------- Globals used in ISR -------------
hw_timer_t *g_timer = nullptr;

// Fixed-point phase accumulator (16.16 format)
volatile uint32_t g_phase_acc    = 0;
volatile uint32_t g_phase_step   = 0;     // how much phase advances each sample
volatile uint32_t g_phase_offset = 0;     // phase difference between channel 2 and 1

// Sine lookup table (256 samples)
uint8_t g_sine_lut[256];

// Actual sample rate and frequency (for debug)
float g_actual_sample_rate = 0.0f;
float g_actual_frequency   = 0.0f;

// HTTP server
WebServer server(80);

// Current phase in degrees (for status/JSON)
volatile float g_current_phase_deg = 0.0f;

// ------------- ISR -------------
void IRAM_ATTR onTimerISR() {
  // Use upper 8 bits of 16.16 phase for lookup index
  uint16_t idx1 = (g_phase_acc >> 8) & 0xFF;
  uint16_t idx2 = ((g_phase_acc + g_phase_offset) >> 8) & 0xFF;

  uint8_t v1 = g_sine_lut[idx1];
  uint8_t v2 = g_sine_lut[idx2];

  dac_output_voltage(DAC_CH1, v1);
  dac_output_voltage(DAC_CH2, v2);

  g_phase_acc += g_phase_step;
}

// ------------- Helpers -------------

void initSineLUT() {
  for (int i = 0; i < 256; ++i) {
    float theta = 2.0f * PI * (float)i / 256.0f;
    float s = sinf(theta);                      // -1..1
    uint8_t v = (uint8_t)((s + 1.0f) * 127.5f); // map [-1,1] -> [0,255]
    g_sine_lut[i] = v;
  }
}

void setPhaseDegrees(float deg) {
  // wrap to [0, 360)
  while (deg < 0.0f)    deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;

  g_current_phase_deg = deg;

  // convert degrees -> 16.16 phase offset
  float frac = deg / 360.0f;
  uint32_t offset = (uint32_t)(frac * 65536.0f + 0.5f);
  // we only use upper 16 bits in ISR, but store in 32-bit to match g_phase_acc
  g_phase_offset = offset;
}

// ------------- HTTP handlers -------------

void handleRoot() {
  String html =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ONDA</title></head><body>"
    "<h1>ONDA ESP32 Backend</h1>"
    "<p>Use <code>/set_phase?phase=DEGREES</code> to adjust phase.</p>"
    "<p>Use <code>/status</code> for JSON status.</p>"
    "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetPhase() {
  if (!server.hasArg("phase")) {
    server.send(400, "application/json", "{\"success\":false,\"error\":\"missing phase param\"}");
    return;
  }
  float deg = server.arg("phase").toFloat();
  setPhaseDegrees(deg);

  String json = "{\"success\":true,\"phase\":";
  json += String(g_current_phase_deg, 1);
  json += "}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  String json = "{";
  json += "\"phase\":"          + String(g_current_phase_deg, 1) + ",";
  json += "\"frequency_hz\":"   + String(g_actual_frequency, 1)   + ",";
  json += "\"sample_rate_hz\":" + String(g_actual_sample_rate,1);
  json += "}";
  server.send(200, "application/json", json);
}

// ------------- Setup timer + DAC -------------

void initDACAndTimer() {
  // Enable DAC outputs
  dac_output_enable(DAC_CH1);
  dac_output_enable(DAC_CH2);

  // Compute timer/sample params
  float desired_sample_rate = BASE_FREQUENCY_HZ * SAMPLES_PER_CYCLE;
  // Timer ticks per sample (1 MHz / desired_sample_rate)
  uint32_t ticks_per_sample = (uint32_t)((float)TIMER_TICKS_PER_S / desired_sample_rate + 0.5f);
  if (ticks_per_sample == 0) ticks_per_sample = 1;

  g_actual_sample_rate = (float)TIMER_TICKS_PER_S / (float)ticks_per_sample;

  // Compute phase step so that output frequency ≈ BASE_FREQUENCY_HZ
  // freq = (phase_step / 2^16) * sample_rate  => phase_step = freq * 2^16 / sample_rate
  g_phase_step = (uint32_t)((BASE_FREQUENCY_HZ * 65536.0f / g_actual_sample_rate) + 0.5f);
  g_actual_frequency = (g_phase_step / 65536.0f) * g_actual_sample_rate;

  Serial.println("=== DAC / Timer Config ===");
  Serial.print("Desired frequency: "); Serial.print(BASE_FREQUENCY_HZ); Serial.println(" Hz");
  Serial.print("Actual sample rate: "); Serial.print(g_actual_sample_rate); Serial.println(" Hz");
  Serial.print("Actual output frequency: "); Serial.print(g_actual_frequency); Serial.println(" Hz");
  Serial.print("Ticks per sample: "); Serial.println(ticks_per_sample);
  Serial.print("Phase step (16.16): "); Serial.println(g_phase_step);

  // Create timer
  g_timer = timerBegin(0, TIMER_DIVIDER, true);  // timer 0, prescaler, count up
  timerAttachInterrupt(g_timer, &onTimerISR, true);
  timerAlarmWrite(g_timer, ticks_per_sample, true);
  timerAlarmEnable(g_timer);
}

// ------------- WiFi / HTTP init -------------

void initWiFiAndServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  IPAddress ip = WiFi.softAPIP();
  Serial.println("=== WiFi AP Started ===");
  Serial.print("SSID: ");     Serial.println(AP_SSID);
  Serial.print("Password: "); Serial.println(AP_PASSWORD);
  Serial.print("AP IP: ");    Serial.println(ip);

  server.on("/",          handleRoot);
  server.on("/set_phase", handleSetPhase);
  server.on("/status",    handleStatus);
  server.begin();
  Serial.println("HTTP server started on port 80");
}

// ------------- Arduino setup/loop -------------

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ONDA ESP32 Backend ===");

  initSineLUT();
  setPhaseDegrees(0.0f);  // start in-phase
  initDACAndTimer();
  initWiFiAndServer();
}

void loop() {
  server.handleClient();
  // DAC runs via timer ISR
}
