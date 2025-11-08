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

// Phase control variables
float current_phase = INITIAL_PHASE;
bool system_running = false;

void setup() {
  Serial.begin(921600);
  delay(1000);
  
  #if TEST_MODE_OSCILLOSCOPE
    Serial.println("========================================");
    Serial.println("ESP32 Acoustic Levitation System");
    Serial.println("OSCILLOSCOPE TEST MODE");
    Serial.println("========================================");
    Serial.println();
    Serial.println("HARDWARE CONNECTIONS:");
    Serial.println("  Oscilloscope Ch1 -> GPIO25 (DAC1)");
    Serial.println("  Oscilloscope Ch2 -> GPIO26 (DAC2)");
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
    Serial.println("PRODUCTION MODE");
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
    Serial.println("Channel 1 (GPIO25): Hardware cosine generator (reference)");
    Serial.println("Channel 2 (GPIO26): Phase-shifted sine wave (controlled)");
    Serial.println();
    
    // Start the system
    levitation_start();
    system_running = true;
    current_phase = INITIAL_PHASE;
    
    Serial.println("✓ Levitation system started!");
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  'u' - Move object up (increase phase)");
    Serial.println("  'd' - Move object down (decrease phase)");
    Serial.println("  'r' - Reset phase to 0°");
    Serial.println("  's' - Show current phase");
    Serial.println("  't' - Toggle system on/off");
    #if TEST_MODE_OSCILLOSCOPE
    Serial.println();
    Serial.println("TEST MODE COMMANDS:");
    Serial.println("  '0' - Set phase to 0° (waves should overlap)");
    Serial.println("  '9' - Set phase to 90° (1/4 cycle shift)");
    Serial.println("  '1' - Set phase to 180° (waves inverted)");
    Serial.println("  '2' - Set phase to 270° (3/4 cycle shift)");
    Serial.println();
    Serial.println("WHAT TO CHECK ON OSCILLOSCOPE:");
    Serial.println("  - At 0°: Both waves should overlap perfectly");
    Serial.println("  - At 90°: Waves should be 1/4 cycle apart");
    Serial.println("  - At 180°: Waves should be inverted");
    Serial.println("  - Use X-Y mode to see Lissajous patterns");
    #endif
    Serial.println();
  } else {
    Serial.println("✗ Failed to initialize system!");
    Serial.println("Check your hardware connections.");
  }
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'u':
      case 'U':
        // Move up: increase phase
        current_phase += 5.0f;  // 5° steps
        levitation_set_phase(current_phase);
        Serial.print("Phase: ");
        Serial.print(current_phase);
        Serial.println("° (moved up)");
        break;
        
      case 'd':
      case 'D':
        // Move down: decrease phase
        current_phase -= 5.0f;  // 5° steps
        levitation_set_phase(current_phase);
        Serial.print("Phase: ");
        Serial.print(current_phase);
        Serial.println("° (moved down)");
        break;
        
      case 'r':
      case 'R':
        // Reset phase
        current_phase = 0.0f;
        levitation_set_phase(current_phase);
        Serial.println("Phase reset to 0°");
        break;
        
      case 's':
      case 'S':
        // Show status
        Serial.println("--- Status ---");
        Serial.print("Frequency: ");
        Serial.print(levitation_get_frequency());
        Serial.println(" Hz");
        Serial.print("Phase shift: ");
        Serial.print(levitation_get_phase());
        Serial.println("°");
        Serial.print("System: ");
        Serial.println(system_running ? "Running" : "Stopped");
        break;
        
      case 't':
      case 'T':
        // Toggle system
        if (system_running) {
          levitation_stop();
          system_running = false;
          Serial.println("System stopped");
        } else {
          levitation_start();
          system_running = true;
          Serial.println("System started");
        }
        break;
        
      #if TEST_MODE_OSCILLOSCOPE
      case '0':
        // Set phase to 0° for testing
        current_phase = 0.0f;
        levitation_set_phase(current_phase);
        Serial.println("Phase set to 0° - waves should overlap on scope");
        break;
        
      case '9':
        // Set phase to 90° for testing
        current_phase = 90.0f;
        levitation_set_phase(current_phase);
        Serial.println("Phase set to 90° - waves should be 1/4 cycle apart");
        break;
        
      case '1':
        // Set phase to 180° for testing
        current_phase = 180.0f;
        levitation_set_phase(current_phase);
        Serial.println("Phase set to 180° - waves should be inverted");
        break;
        
      case '2':
        // Set phase to 270° for testing
        current_phase = 270.0f;
        levitation_set_phase(current_phase);
        Serial.println("Phase set to 270° - waves should be 3/4 cycle apart");
        break;
      #endif
        
      default:
        if (cmd != '\n' && cmd != '\r') {
          Serial.print("Unknown command: '");
          Serial.print(cmd);
          Serial.println("'");
        }
        break;
    }
  }
  
  // Optional: Automatic phase sweep for testing
  // Uncomment to automatically sweep phase from 0° to 360°
  /*
  static unsigned long last_update = 0;
  if (millis() - last_update > 100) {  // Update every 100ms
    current_phase += 1.0f;  // 1° per update
    if (current_phase >= 360.0f) {
      current_phase = 0.0f;
    }
    levitation_set_phase(current_phase);
    last_update = millis();
    
    if ((int)current_phase % 45 == 0) {  // Print every 45°
      Serial.print("Phase: ");
      Serial.print(current_phase);
      Serial.println("°");
    }
  }
  */
  
  // Phase shift iteration test - cycles through 0° to 360° in 45° steps
  #define ENABLE_PHASE_ITERATION true
  
  #if ENABLE_PHASE_ITERATION
  static unsigned long last_update = 0;
  static float test_phase = 0.0f;
  
  if (millis() - last_update > 2000) {  // Update every 2 seconds
    test_phase += 45.0f;
    if (test_phase >= 360.0f) test_phase = 0.0f;
    
    current_phase = test_phase;
    levitation_set_phase(current_phase);
    Serial.print("Phase: ");
    Serial.print(current_phase);
    Serial.println("°");
    
    last_update = millis();
  }
  #endif
}