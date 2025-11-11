#ifndef PHASE_SHIFTED_DAC_H
#define PHASE_SHIFTED_DAC_H

#include <Arduino.h>
#include "driver/dac.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize DMA-based phase-shifted sine wave generator on DAC channel 2
 * Channel 1 should use hardware cosine generator as reference
 * 
 * @param frequency Frequency in Hz (typically 40000 for ultrasonic)
 * @param sample_rate Sample rate for DMA (typically 80000 or higher)
 * @param phase_shift Phase shift in degrees (0-360)
 * @return true if successful, false otherwise
 */
bool phase_shifted_dac_init(float frequency, uint32_t sample_rate, float phase_shift);

/**
 * Update the phase shift of the DMA-generated sine wave
 * @param phase_shift Phase shift in degrees (0-360)
 */
void phase_shifted_dac_set_phase(float phase_shift);

/**
 * Update the frequency of the DMA-generated sine wave
 * @param frequency Frequency in Hz
 */
void phase_shifted_dac_set_frequency(float frequency);

/**
 * Start the DMA-based sine wave output
 */
void phase_shifted_dac_start();

/**
 * Stop the DMA-based sine wave output
 */
void phase_shifted_dac_stop();

/**
 * Clean up DMA resources
 */
void phase_shifted_dac_deinit();

#ifdef __cplusplus
}
#endif

#endif // PHASE_SHIFTED_DAC_H

