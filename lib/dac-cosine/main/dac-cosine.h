#ifndef DAC_COSINE_H
#define DAC_COSINE_H

#include "driver/dac.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enable cosine waveform generator on a DAC channel
 * @param channel DAC channel (DAC_CHANNEL_1 or DAC_CHANNEL_2)
 */
void dac_cosine_enable(dac_channel_t channel);

/**
 * Set frequency of internal CW generator common to both DAC channels
 * @param clk_8m_div RTC 8M clock divider (0b000 - 0b111)
 * @param frequency_step Frequency step (0x0001 - 0xFFFF)
 */
void dac_frequency_set(int clk_8m_div, int frequency_step);

/**
 * Scale output of a DAC channel
 * @param channel DAC channel
 * @param scale 00: no scale, 01: 1/2, 10: 1/4, 11: 1/8
 */
void dac_scale_set(dac_channel_t channel, int scale);

/**
 * Offset output of a DAC channel
 * @param channel DAC channel
 * @param offset DC offset (0x00 - 0xFF)
 */
void dac_offset_set(dac_channel_t channel, int offset);

/**
 * Invert output pattern of a DAC channel
 * @param channel DAC channel
 * @param invert 00: no invert, 01: invert all, 10: invert MSB, 11: invert all except MSB
 */
void dac_invert_set(dac_channel_t channel, int invert);

/**
 * Calculate frequency from clock divider and frequency step
 * @param clk_8m_div Clock divider
 * @param frequency_step Frequency step
 * @return Frequency in Hz
 */
float dac_calculate_frequency(int clk_8m_div, int frequency_step);

#ifdef __cplusplus
}
#endif

#endif // DAC_COSINE_H

