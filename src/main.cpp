#include <Arduino.h>
#include "levitation_control.h"

// ========================================
// CONFIGURATION: Choose your mode
// ========================================
#define TEST_MODE_OSCILLOSCOPE true  // Set to true for oscilloscope testing, false for production

#if TEST_MODE_OSCILLOSCOPE
  // Test mode: Lower frequency for oscilloscope viewing
  const float ULTRASONIC_FREQUENCY = 1000.0f;  // 1 kHz - easy to see on scope
  const float INITIAL_PHASE = 0.0f;
#else
  // Production mode: Ultrasonic frequency for levitation
  const float ULTRASONIC_FREQUENCY = 40000.0f;  // 40 kHz ultrasonic frequency
  const float INITIAL_PHASE = 0.0f;
#endif

// Phase shift test configuration
const float PHASE_SHIFT_INTERVAL = 5000;  // Change phase every 5 seconds (5000ms)
// Phase values to cycle through (in degrees)
const float PHASE_VALUES[] = {0.0f, 90.0f, 180.0f, 270.0f, 360.0f};
const int NUM_PHASE_VALUES = sizeof(PHASE_VALUES) / sizeof(PHASE_VALUES[0]);

// Phase control variables
int phase_index = 0;
unsigned long last_phase_change = 0;

void setup() {
  Serial.begin(921600);
  delay(1000);
  
  #if TEST_MODE_OSCILLOSCOPE
    Serial.println("========================================");
    Serial.println("ESP32 Acoustic Levitation System");
    Serial.println("AUTOMATIC PHASE SHIFT TEST MODE");
    Serial.println("========================================");
    Serial.println();
    Serial.println("HARDWARE CONNECTIONS:");
    Serial.println("  Oscilloscope Ch1 -> GPIO25 (DAC1) - Reference wave");
    Serial.println("  Oscilloscope Ch2 -> GPIO26 (DAC2) - Phase-shifted wave");
    Serial.println("  Oscilloscope GND -> GND");
    Serial.println();
    Serial.println("RECOMMENDED SCOPE SETTINGS:");
    Serial.println("  Timebase: 200us/div to 1ms/div");
    Serial.println("  Voltage: 500mV/div or 1V/div");
    Serial.println("  Coupling: DC");
    Serial.println("  Trigger: Ch1, Rising Edge");
    Serial.println("  Try X-Y mode for phase visualization!");
    Serial.println();
  #else
    Serial.println("========================================");
    Serial.println("ESP32 Acoustic Levitation System");
    Serial.println("AUTOMATIC PHASE SHIFT MODE");
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
    Serial.println("Channel 1 (GPIO25): Hardware cosine generator (reference - no phase shift)");
    Serial.println("Channel 2 (GPIO26): Phase-shifted sine wave (automatically shifting every 5 seconds)");
    Serial.println();
    
    // Start the system
    levitation_start();
    last_phase_change = millis();
    
    Serial.println("✓ Levitation system started!");
    Serial.println();
    Serial.println("AUTOMATIC PHASE SHIFT TEST:");
    Serial.println("  Phase will cycle through: 0°, 90°, 180°, 270°, 360°");
    Serial.println("  Each phase lasts 5 seconds");
    Serial.println();
    Serial.println("WHAT TO CHECK ON OSCILLOSCOPE:");
    Serial.println("  - At 0°: Both waves should overlap perfectly");
    Serial.println("  - At 90°: Waves should be 1/4 cycle apart");
    Serial.println("  - At 180°: Waves should be inverted");
    Serial.println("  - At 270°: Waves should be 3/4 cycle apart");
    Serial.println("  - At 360°: Same as 0° (waves overlap)");
    Serial.println("  - Use X-Y mode to see Lissajous patterns!");
    Serial.println();
    Serial.println("Starting phase test in 2 seconds...");
    Serial.println();
    delay(2000);
    
    // Set initial phase and print
    levitation_set_phase(PHASE_VALUES[0]);
    Serial.print(">>> Phase set to: ");
    Serial.print(PHASE_VALUES[0]);
    Serial.println("°");
  } else {
    Serial.println("✗ Failed to initialize system!");
    Serial.println("Check your hardware connections.");
  }
}

void loop() {
  // Check if it's time to change phase
  unsigned long current_time = millis();
  
  if (current_time - last_phase_change >= PHASE_SHIFT_INTERVAL) {
    // Move to next phase value
    phase_index = (phase_index + 1) % NUM_PHASE_VALUES;
    float new_phase = PHASE_VALUES[phase_index];
    
    // Apply the new phase
    levitation_set_phase(new_phase);
    
    // Print status
    Serial.print(">>> Phase changed to: ");
    Serial.print(new_phase);
    Serial.print("° (");
    
    // Describe what should be visible
    if (new_phase == 0.0f || new_phase == 360.0f) {
      Serial.print("waves should overlap");
    } else if (new_phase == 90.0f) {
      Serial.print("1/4 cycle shift");
    } else if (new_phase == 180.0f) {
      Serial.print("waves inverted");
    } else if (new_phase == 270.0f) {
      Serial.print("3/4 cycle shift");
    }
    Serial.println(")");
    
    // Update timer
    last_phase_change = current_time;
  }
}