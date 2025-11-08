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
    g_phase_shift = 0.0f;  // Ignored for identical waves test
    
    // Use timer interrupt for BOTH channels - this ensures IDENTICAL waves
    // Both channels use the same timer, same phase accumulator, same lookup table
    // Calculate sample rate that's at least 2x the frequency (Nyquist)
    uint32_t sample_rate = (uint32_t)(frequency * 2.5f);  // 2.5x for good quality
    if (sample_rate < 80000) sample_rate = 80000;  // Minimum sample rate
    if (sample_rate > 200000) sample_rate = 200000;  // Maximum reasonable rate
    
    // Initialize both channels with timer interrupt - they will be IDENTICAL
    if (!phase_shifted_dac_init(frequency, sample_rate, 0.0f)) {
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
        // Update timer-based DAC frequency (affects both channels identically)
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

