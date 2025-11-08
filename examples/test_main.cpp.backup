#include <Arduino.h>
#include "test_mode.h"

/**
 * OSCILLOSCOPE TEST MODE
 * 
 * This file is optimized for testing the DAC outputs with an oscilloscope.
 * Use this to verify waveforms before connecting ultrasonic transducers.
 * 
 * To use this instead of main.cpp:
 * 1. Rename main.cpp to main_production.cpp (or backup it)
 * 2. Rename this file (test_main.cpp) to main.cpp
 * 3. Upload and test with oscilloscope
 * 
 * Oscilloscope Connections:
 * - Channel 1 probe -> GPIO25 (DAC1 output)
 * - Channel 2 probe -> GPIO26 (DAC2 output)  
 * - Ground clip -> GND on ESP32
 * 
 * Recommended Oscilloscope Settings:
 * - Timebase: 200us/div to 1ms/div (for 1-5 kHz test frequency)
 * - Voltage: 1V/div or 500mV/div
 * - Coupling: DC
 * - Trigger: Channel 1, Rising edge
 * - Use X-Y mode to see Lissajous patterns for phase verification
 */

// Test configuration - adjust these for your oscilloscope
const float TEST_FREQUENCY = 1000.0f;      // 1 kHz - easy to see on scope
const float INITIAL_PHASE = 0.0f;          // Start with 0° phase shift

void setup() {
  Serial.begin(921600);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("OSCILLOSCOPE TEST MODE");
  Serial.println("========================================");
  Serial.println();
  Serial.println("This mode is designed for oscilloscope verification.");
  Serial.println("It uses lower frequencies for easier viewing.");
  Serial.println();
  
  // Initialize test mode
  test_mode_init(TEST_FREQUENCY, INITIAL_PHASE);
  
  // Start the test mode interactive menu
  test_mode_run();
}

void loop() {
  // Everything is handled in test_mode_run()
  // This should never be reached, but just in case...
  delay(1000);
}

