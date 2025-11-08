#include "phase_shifted_dac.h"
#include "driver/dac.h"
#include "soc/sens_reg.h"
#include <math.h>

// Timer configuration for sample output
#define TIMER_DIVIDER 16
// Use APB clock frequency (typically 80 MHz) for timer calculations
// In Arduino ESP32 framework, APB frequency is 80 MHz
#define APB_FREQ 80000000
#define TIMER_SCALE (APB_FREQ / TIMER_DIVIDER)

static float g_frequency = 40000.0f;
static float g_phase_shift = 0.0f;
static uint32_t g_sample_rate = 80000;
static bool g_initialized = false;
static bool g_running = false;
static hw_timer_t* g_timer = nullptr;

// Use fixed-point arithmetic for ISR (avoid floating point in ISR)
static volatile uint32_t g_phase_accumulator = 0;  // Fixed point: 0-65535 represents 0-2π
static volatile uint32_t g_phase_increment = 0;    // Fixed point phase increment
static uint8_t* g_sine_lut = nullptr;              // Sine lookup table
static const uint16_t LUT_SIZE = 256;              // Lookup table size

/**
 * Generate sine lookup table (0-255 for 0-2π)
 * Centered around midpoint (127.5 = 1.65V) with full amplitude
 */
void generate_sine_lut(uint8_t* lut, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        float angle = 2.0f * M_PI * i / size;
        float value = sinf(angle);
        // Convert from [-1, 1] to [0, 255] for 8-bit DAC
        // This creates a sine wave from 0V to 3.3V (centered at 1.65V)
        lut[i] = (uint8_t)((value + 1.0f) * 127.5f);
    }
}

/**
 * Timer interrupt handler to output samples
 * Uses fixed-point arithmetic and lookup table to avoid floating point in ISR
 * Outputs IDENTICAL sine wave to BOTH channels
 */
void IRAM_ATTR timer_isr() {
    if (!g_running || !g_sine_lut) return;
    
    // Get sample from lookup table using fixed-point phase
    uint8_t lut_index = (g_phase_accumulator >> 8) & 0xFF;  // Use upper 8 bits for LUT index
    uint8_t dac_value = g_sine_lut[lut_index];
    
    // Output IDENTICAL value to BOTH DAC channels simultaneously
    // This ensures perfect synchronization, identical frequency, and identical amplitude
    dacWrite(25, dac_value);  // Channel 1 (GPIO25)
    dacWrite(26, dac_value);  // Channel 2 (GPIO26)
    
    // Update phase accumulator (fixed point)
    g_phase_accumulator += g_phase_increment;
    // Phase accumulator wraps at 65536 (2π in fixed point)
}

bool phase_shifted_dac_init(float frequency, uint32_t sample_rate, float phase_shift) {
    if (g_initialized) {
        phase_shifted_dac_deinit();
    }
    
    g_frequency = frequency;
    g_phase_shift = phase_shift;
    g_sample_rate = sample_rate;
    
    // Allocate and generate sine lookup table
    g_sine_lut = (uint8_t*)malloc(LUT_SIZE);
    if (!g_sine_lut) {
        return false;
    }
    generate_sine_lut(g_sine_lut, LUT_SIZE);
    
    // Disable hardware cosine generator on BOTH channels (we'll use timer interrupt for both)
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
    
    // Enable BOTH DAC channels - they will receive IDENTICAL output
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_2);
    
    // Initialize phase accumulator (start at 0 for identical waves)
    g_phase_accumulator = 0;
    
    // Calculate phase increment (fixed point) with high precision
    // phase_increment = (2π * frequency / sample_rate) * (65536 / 2π) = frequency * 65536 / sample_rate
    // Use rounding for better precision
    double phase_inc = ((double)frequency * 65536.0) / (double)sample_rate;
    g_phase_increment = (uint32_t)(phase_inc + 0.5);  // Round to nearest integer for precision
    
    // Configure timer for sample output
    g_timer = timerBegin(0, TIMER_DIVIDER, true);
    if (!g_timer) {
        free(g_sine_lut);
        g_sine_lut = nullptr;
        return false;
    }
    
    // Calculate timer period (in timer counts)
    // Timer runs at TIMER_SCALE Hz, we need sample_rate interrupts per second
    uint64_t timer_period = TIMER_SCALE / sample_rate;
    timerAlarmWrite(g_timer, timer_period, true);  // true = auto-reload
    timerAttachInterrupt(g_timer, &timer_isr, true);  // true = edge trigger
    timerWrite(g_timer, 0);
    
    g_initialized = true;
    return true;
}

void phase_shifted_dac_set_phase(float phase_shift) {
    // For testing identical waves, phase is ignored - both channels stay identical
    g_phase_shift = 0.0f;
    // Both channels always output the same value, so phase shifting is disabled
}

void phase_shifted_dac_set_frequency(float frequency) {
    g_frequency = frequency;
    
    if (g_initialized) {
        // Recalculate phase increment (fixed point) with high precision
        double phase_inc = ((double)frequency * 65536.0) / (double)g_sample_rate;
        g_phase_increment = (uint32_t)(phase_inc + 0.5);  // Round to nearest integer for precision
    }
}

void phase_shifted_dac_start() {
    if (g_initialized && g_timer && !g_running) {
        g_running = true;
        timerAlarmEnable(g_timer);
    }
}

void phase_shifted_dac_stop() {
    if (g_timer && g_running) {
        g_running = false;
        timerAlarmDisable(g_timer);
    }
}

void phase_shifted_dac_deinit() {
    phase_shifted_dac_stop();
    
    if (g_timer) {
        timerDetachInterrupt(g_timer);
        timerEnd(g_timer);
        g_timer = nullptr;
    }
    
    if (g_sine_lut) {
        free(g_sine_lut);
        g_sine_lut = nullptr;
    }
    
    g_initialized = false;
    g_running = false;
}

