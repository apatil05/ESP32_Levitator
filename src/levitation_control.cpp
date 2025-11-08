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
    // For precise 40 kHz: use sample rate that gives exact phase increment
    // Calculate optimal sample rate for maximum precision
    uint32_t sample_rate;
    if (frequency == 40000.0f) {
        // For exactly 40 kHz, use 200 kHz sample rate for high precision
        // This gives phase_increment = 40000 * 65536 / 200000 = 13107.2
        // With rounding, this is very precise
        sample_rate = 200000;  // 200 kHz - 5x oversampling for 40 kHz
    } else {
        // For other frequencies, use 2.5x oversampling
        sample_rate = (uint32_t)(frequency * 2.5f);
        if (sample_rate < 80000) sample_rate = 80000;
        if (sample_rate > 200000) sample_rate = 200000;
    }
    
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

