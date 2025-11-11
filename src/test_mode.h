#ifndef TEST_MODE_H
#define TEST_MODE_H

#include <Arduino.h>

/**
 * Test mode for oscilloscope verification
 * This mode generates test waveforms to verify DAC outputs before
 * connecting to ultrasonic transducers
 */

/**
 * Initialize test mode with lower frequency for easier oscilloscope viewing
 * @param test_frequency Test frequency in Hz (recommended: 1000-10000 Hz for scope viewing)
 * @param phase_shift Phase shift between channels in degrees
 */
void test_mode_init(float test_frequency = 1000.0f, float phase_shift = 0.0f);

/**
 * Run a phase sweep test - automatically sweeps phase from 0° to 360°
 * Useful for observing phase relationship on oscilloscope
 * @param sweep_speed_ms Time per degree in milliseconds
 */
void test_mode_phase_sweep(uint32_t sweep_speed_ms = 50);

/**
 * Set fixed phase for testing
 * @param phase_shift Phase in degrees (0-360)
 */
void test_mode_set_phase(float phase_shift);

/**
 * Get current test phase
 * @return Current phase in degrees
 */
float test_mode_get_phase();

/**
 * Start test mode
 */
void test_mode_start();

/**
 * Stop test mode
 */
void test_mode_stop();

/**
 * Run continuous test with serial commands
 */
void test_mode_run();

#endif // TEST_MODE_H

