#include "test_mode.h"
#include "levitation_control.h"

static float g_test_frequency = 1000.0f;
static float g_test_phase = 0.0f;
static bool g_test_running = false;

void test_mode_init(float test_frequency, float phase_shift) {
    g_test_frequency = test_frequency;
    g_test_phase = phase_shift;
    
    Serial.println("========================================");
    Serial.println("OSCILLOSCOPE TEST MODE");
    Serial.println("========================================");
    Serial.println();
    Serial.println("HARDWARE CONNECTIONS:");
    Serial.println("  Oscilloscope Channel 1 -> GPIO25 (DAC1)");
    Serial.println("  Oscilloscope Channel 2 -> GPIO26 (DAC2)");
    Serial.println("  Oscilloscope Ground -> GND on ESP32");
    Serial.println();
    Serial.println("RECOMMENDED SCOPE SETTINGS:");
    Serial.print("  Timebase: ");
    if (test_frequency <= 5000) {
        Serial.println("200us/div to 1ms/div");
    } else {
        Serial.println("5us/div to 10us/div");
    }
    Serial.println("  Voltage: 500mV/div or 1V/div");
    Serial.println("  Coupling: DC");
    Serial.println("  Trigger: Channel 1, Rising Edge");
    Serial.println("  Try X-Y mode for phase visualization!");
    Serial.println();
    Serial.print("Test Frequency: ");
    Serial.print(test_frequency);
    Serial.println(" Hz");
    Serial.print("Initial Phase Shift: ");
    Serial.print(phase_shift);
    Serial.println("°");
    Serial.println();
    Serial.println("EXPECTED ON OSCILLOSCOPE:");
    Serial.println("  - Two sine waves at the same frequency");
    Serial.println("  - Phase difference matches the set phase shift");
    Serial.println("  - Both waves should be clean and stable");
    Serial.println("  - At 0°: waves should overlap perfectly");
    Serial.println("  - At 90°: waves should be 1/4 cycle apart");
    Serial.println("  - At 180°: waves should be inverted");
    Serial.println();
    
    // Initialize levitation system with test frequency
    if (levitation_init(test_frequency, phase_shift)) {
        Serial.println("✓ Test mode initialized successfully!");
        Serial.println();
    } else {
        Serial.println("✗ Failed to initialize test mode!");
        Serial.println("Check your connections and try again.");
    }
}

void test_mode_phase_sweep(uint32_t sweep_speed_ms) {
    if (!g_test_running) {
        test_mode_start();
    }
    
    Serial.println("Starting phase sweep test...");
    Serial.println("Observe the phase relationship on your oscilloscope.");
    Serial.println("Press any key to stop.");
    Serial.println();
    
    float phase = 0.0f;
    unsigned long last_update = millis();
    
    while (true) {
        // Check for serial input to stop
        if (Serial.available()) {
            Serial.read();  // Clear the buffer
            break;
        }
        
        // Update phase
        if (millis() - last_update >= sweep_speed_ms) {
            phase += 1.0f;  // 1 degree per update
            if (phase >= 360.0f) {
                phase = 0.0f;
            }
            
            test_mode_set_phase(phase);
            last_update = millis();
            
            // Print every 45 degrees
            if ((int)phase % 45 == 0) {
                Serial.print("Phase: ");
                Serial.print(phase);
                Serial.println("°");
            }
        }
        
        delay(1);  // Small delay to allow serial processing
    }
    
    Serial.println("Phase sweep stopped.");
}

void test_mode_set_phase(float phase_shift) {
    g_test_phase = phase_shift;
    levitation_set_phase(phase_shift);
}

float test_mode_get_phase() {
    return g_test_phase;
}

void test_mode_start() {
    levitation_start();
    g_test_running = true;
    Serial.println("Test mode started - waveforms are now active");
}

void test_mode_stop() {
    levitation_stop();
    g_test_running = false;
    Serial.println("Test mode stopped");
}

void test_mode_run() {
    Serial.println("========================================");
    Serial.println("OSCILLOSCOPE TEST MODE - INTERACTIVE");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  's' - Start waveforms");
    Serial.println("  'x' - Stop waveforms");
    Serial.println("  'p' - Set phase (will prompt for value)");
    Serial.println("  'f' - Set frequency (will prompt for value)");
    Serial.println("  'w' - Sweep phase 0° to 360°");
    Serial.println("  '0' - Set phase to 0°");
    Serial.println("  '9' - Set phase to 90°");
    Serial.println("  '1' - Set phase to 180°");
    Serial.println("  '2' - Set phase to 270°");
    Serial.println("  'i' - Show current info");
    Serial.println();
    
    bool running = false;
    
    while (true) {
        if (Serial.available()) {
            char cmd = Serial.read();
            
            switch (cmd) {
                case 's':
                case 'S':
                    test_mode_start();
                    running = true;
                    break;
                    
                case 'x':
                case 'X':
                    test_mode_stop();
                    running = false;
                    break;
                    
                case 'p':
                case 'P':
                    {
                        Serial.println("Enter phase shift in degrees (0-360):");
                        while (!Serial.available()) delay(10);
                        float phase = Serial.parseFloat();
                        Serial.readStringUntil('\n');  // Clear buffer
                        test_mode_set_phase(phase);
                        Serial.print("Phase set to: ");
                        Serial.print(phase);
                        Serial.println("°");
                    }
                    break;
                    
                case 'f':
                case 'F':
                    {
                        Serial.println("Enter frequency in Hz:");
                        while (!Serial.available()) delay(10);
                        float freq = Serial.parseFloat();
                        Serial.readStringUntil('\n');  // Clear buffer
                        levitation_set_frequency(freq);
                        g_test_frequency = freq;
                        Serial.print("Frequency set to: ");
                        Serial.print(freq);
                        Serial.println(" Hz");
                    }
                    break;
                    
                case 'w':
                case 'W':
                    test_mode_phase_sweep(50);  // 50ms per degree
                    break;
                    
                case '0':
                    test_mode_set_phase(0.0f);
                    Serial.println("Phase set to 0°");
                    break;
                    
                case '9':
                    test_mode_set_phase(90.0f);
                    Serial.println("Phase set to 90°");
                    break;
                    
                case '1':
                    test_mode_set_phase(180.0f);
                    Serial.println("Phase set to 180°");
                    break;
                    
                case '2':
                    test_mode_set_phase(270.0f);
                    Serial.println("Phase set to 270°");
                    break;
                    
                case 'i':
                case 'I':
                    Serial.println("--- Current Settings ---");
                    Serial.print("Frequency: ");
                    Serial.print(levitation_get_frequency());
                    Serial.println(" Hz");
                    Serial.print("Phase Shift: ");
                    Serial.print(levitation_get_phase());
                    Serial.println("°");
                    Serial.print("Status: ");
                    Serial.println(running ? "Running" : "Stopped");
                    break;
                    
                default:
                    if (cmd != '\n' && cmd != '\r') {
                        Serial.print("Unknown command: '");
                        Serial.print(cmd);
                        Serial.println("'");
                    }
                    break;
            }
        }
        
        delay(10);
    }
}

