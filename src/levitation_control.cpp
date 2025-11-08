#include "levitation_control.h"
#include "phase_shifted_dac.h"
#include "soc/sens_reg.h"
#include "driver/dac.h"
#include "soc/rtc.h"

static float g_frequency = 40000.0f;
static float g_phase_shift = 0.0f;
static bool g_initialized = false;

bool levitation_init(float frequency, float initial_phase) {
    if (g_initialized) {
        levitation_stop();
    }
    
    g_frequency = frequency;
    g_phase_shift = initial_phase;
    
    // Calculate frequency step for hardware cosine generator
    // Formula: freq = dig_clk_rtc_freq × SENS_SAR_SW_FSTEP / 65536
    // dig_clk_rtc_freq is typically 8 MHz
    const float rtc_freq = RTC_FAST_CLK_FREQ_APPROX;
    uint16_t freq_step = (uint16_t)((frequency * 65536.0f) / rtc_freq);
    if (freq_step < 1) freq_step = 1;
    if (freq_step > 65535) freq_step = 65535;
    
    // Setup Channel 1: Hardware cosine generator configured as SINE wave (reference)
    // Both channels will output identical sine waves at the same frequency
    // 1. Enable cosine waveform generator
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    
    // 2. Connect generator to DAC channel 1 (GPIO25)
    SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    
    // 3. Set frequency step (ensures exact frequency match with Channel 2)
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, freq_step, SENS_SW_FSTEP_S);
    
    // 4. Invert MSB to convert cosine to sine wave (invert=2)
    // This ensures Channel 1 outputs a sine wave, matching Channel 2's sine lookup table
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
    
    // 5. Ensure full amplitude scale (0 = no scaling, full 0-255 range)
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, 0, SENS_DAC_SCALE1_S);
    
    // 6. Enable DAC1 output
    dac_output_enable(DAC_CHANNEL_1);
    
    // Setup Channel 2: Phase-shifted SINE wave using timer interrupt
    // Uses sine lookup table (sinf()) - same waveform type as Channel 1
    // Same frequency as Channel 1, full amplitude (0-255 range)
    // Use a sample rate that's at least 2x the frequency (Nyquist)
    uint32_t sample_rate = (uint32_t)(frequency * 2.5f);  // 2.5x for good quality
    if (sample_rate < 80000) sample_rate = 80000;  // Minimum sample rate
    if (sample_rate > 200000) sample_rate = 200000;  // Maximum reasonable rate
    
    if (!phase_shifted_dac_init(frequency, sample_rate, initial_phase)) {
        return false;
    }
    
    g_initialized = true;
    return true;
}

void levitation_set_phase(float phase_shift) {
    g_phase_shift = fmodf(phase_shift, 360.0f);
    if (g_phase_shift < 0) g_phase_shift += 360.0f;
    
    if (g_initialized) {
        phase_shifted_dac_set_phase(g_phase_shift);
    }
}

float levitation_get_phase() {
    return g_phase_shift;
}

void levitation_set_frequency(float frequency) {
    g_frequency = frequency;
    
    if (g_initialized) {
        // Update hardware cosine generator frequency
        const float rtc_freq = RTC_FAST_CLK_FREQ_APPROX;
        uint16_t freq_step = (uint16_t)((frequency * 65536.0f) / rtc_freq);
        if (freq_step < 1) freq_step = 1;
        if (freq_step > 65535) freq_step = 65535;
        SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, freq_step, SENS_SW_FSTEP_S);
        
        // Update phase-shifted DAC frequency
        phase_shifted_dac_set_frequency(frequency);
    }
}

float levitation_get_frequency() {
    return g_frequency;
}

void levitation_start() {
    if (g_initialized) {
        phase_shifted_dac_start();
    }
}

void levitation_stop() {
    if (g_initialized) {
        phase_shifted_dac_stop();
    }
}

void levitation_move(float direction, float step_size) {
    if (g_initialized) {
        float new_phase = g_phase_shift + (direction * step_size);
        levitation_set_phase(new_phase);
    }
}

