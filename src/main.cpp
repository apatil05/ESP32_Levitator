#include <Arduino.h>
#include "levitation_control.h"

// ========================================
// CONFIGURATION: Choose your mode
// ========================================
#define TEST_MODE_OSCILLOSCOPE true  // Set to true for oscilloscope testing, false for production

#if TEST_MODE_OSCILLOSCOPE
  // Test mode: Precise 40 kHz frequency
  const float ULTRASONIC_FREQUENCY = 40000.0f;  // Exactly 40 kHz
  const float INITIAL_PHASE = 0.0f;
#else
  // Production mode: Ultrasonic frequency for levitation
  const float ULTRASONIC_FREQUENCY = 40000.0f;  // 40 kHz ultrasonic frequency
  const float INITIAL_PHASE = 0.0f;
#endif

// Test mode: Output identical sine waves on both channels

void setup() {
  Serial.begin(921600);
  delay(1000);
  
  #if TEST_MODE_OSCILLOSCOPE
    Serial.println("========================================");
    Serial.println("ESP32 Acoustic Levitation System");
    Serial.println("IDENTICAL SINE WAVES TEST MODE");
    Serial.println("========================================");
    Serial.println();
    Serial.println("HARDWARE CONNECTIONS:");
    Serial.println("  Oscilloscope Ch1 -> GPIO25 (DAC1)");
    Serial.println("  Oscilloscope Ch2 -> GPIO26 (DAC2)");
    Serial.println("  Oscilloscope GND -> GND");
    Serial.println();
    Serial.println("RECOMMENDED SCOPE SETTINGS:");
    Serial.println("  Timebase: 2us/div to 5us/div (40 kHz = 25us period)");
    Serial.println("  Voltage: 500mV/div or 1V/div");
    Serial.println("  Coupling: DC");
    Serial.println("  Trigger: Ch1, Rising Edge");
    Serial.println("  Try X-Y mode: Should show diagonal line!");
    Serial.println();
  #else
    Serial.println("========================================");
    Serial.println("ESP32 Acoustic Levitation System");
    Serial.println("IDENTICAL SINE WAVES MODE");
    Serial.println("========================================");
    Serial.println();
  #endif
  
  // Initialize the levitation system
  Serial.print("Initializing levitation system at ");
  Serial.print(ULTRASONIC_FREQUENCY);
  Serial.println(" Hz...");
  
  if (levitation_init(ULTRASONIC_FREQUENCY, INITIAL_PHASE)) {
    Serial.println("✓ System initialized successfully!");
    Serial.println();
    Serial.println("Channel 1 (GPIO25): Sine wave");
    Serial.println("Channel 2 (GPIO26): Sine wave");
    Serial.println();
    Serial.println("BOTH CHANNELS OUTPUT IDENTICAL SINE WAVES");
    Serial.println("  - Same frequency");
    Serial.println("  - Same amplitude");
    Serial.println("  - Same phase");
    Serial.println("  - Perfect synchronization");
    Serial.println();
    
    // Start the system
    levitation_start();
    
    Serial.println("✓ System started!");
    Serial.println();
    Serial.println("TEST MODE: IDENTICAL SINE WAVES AT 40 kHz");
    Serial.println("  Both channels output the EXACT same signal");
    Serial.println("  Frequency: Precisely 40,000 Hz (40 kHz)");
    Serial.println("  Sample rate: 200,000 Hz (5x oversampling)");
    Serial.println();
    Serial.println("WHAT TO CHECK ON OSCILLOSCOPE:");
    Serial.println("  - Both waves should overlap perfectly");
    Serial.println("  - Frequency should measure exactly 40 kHz");
    Serial.println("  - Period should be exactly 25 microseconds");
    Serial.println("  - Same amplitude (same voltage range)");
    Serial.println("  - Same phase (no offset between waves)");
    Serial.println("  - In X-Y mode: Should show a diagonal line");
    Serial.println();
    Serial.println("System running at 40 kHz...");
    Serial.println();
  } else {
    Serial.println("✗ Failed to initialize system!");
    Serial.println("Check your hardware connections.");
  }
}

void loop() {
  // For identical waves test, do nothing - both channels output identical signals continuously
  // No phase shifting - just identical sine waves
  delay(1000);
}