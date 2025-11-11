#ifndef LEVITATION_CONTROL_H
#define LEVITATION_CONTROL_H

#include <Arduino.h>

/**
 * Initialize acoustic levitation system
 * Sets up both DAC channels:
 * - Channel 1 (GPIO25): Hardware cosine generator (reference)
 * - Channel 2 (GPIO26): Phase-shifted sine wave (controlled)
 * 
 * @param frequency Ultrasonic frequency in Hz (typically 40000)
 * @param initial_phase Initial phase shift in degrees (0-360)
 * @return true if successful, false otherwise
 */
bool levitation_init(float frequency, float initial_phase = 0.0f);

/**
 * Set phase shift between the two channels
 * This controls the position of pressure nodes in the standing wave
 * 
 * @param phase_shift Phase shift in degrees (0-360)
 */
void levitation_set_phase(float phase_shift);

/**
 * Get current phase shift
 * @return Current phase shift in degrees
 */
float levitation_get_phase();

/**
 * Set frequency (both channels will use this frequency)
 * @param frequency Frequency in Hz
 */
void levitation_set_frequency(float frequency);

/**
 * Get current frequency
 * @return Current frequency in Hz
 */
float levitation_get_frequency();

/**
 * Start levitation system
 */
void levitation_start();

/**
 * Stop levitation system
 */
void levitation_stop();

/**
 * Move the levitated object up or down by adjusting phase
 * @param direction Positive = up, negative = down
 * @param step_size Phase change per step in degrees
 */
void levitation_move(float direction, float step_size = 1.0f);

#endif // LEVITATION_CONTROL_H

