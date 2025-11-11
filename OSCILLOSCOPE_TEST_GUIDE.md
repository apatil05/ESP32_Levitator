# Oscilloscope Testing Guide

This guide explains how to test your ESP32 DAC outputs using an oscilloscope before connecting ultrasonic transducers.

## Hardware Connections

### Oscilloscope Connections
1. **Channel 1** → Connect to **GPIO25** (DAC1 output)
2. **Channel 2** → Connect to **GPIO26** (DAC2 output)
3. **Ground** → Connect to **GND** on ESP32

**Important:** 
- Use proper oscilloscope probes with ground clips
- Ensure good connections - loose connections can cause noise
- Keep probe leads short to minimize interference

## Software Setup

### Option 1: Use Test Mode (Recommended)

1. **Backup your current main.cpp:**
   ```bash
   mv src/main.cpp src/main_production.cpp
   ```

2. **Use the test main file:**
   ```bash
   mv src/test_main.cpp src/main.cpp
   ```

3. **Upload to ESP32**

4. **Open Serial Monitor** at 921600 baud

### Option 2: Modify Existing Code

Simply change the frequency in `main.cpp`:
```cpp
const float ULTRASONIC_FREQUENCY = 1000.0f;  // Change from 40000 to 1000 for testing
```

## Oscilloscope Settings

### Recommended Settings for 1 kHz Test Frequency

| Setting | Value | Notes |
|---------|-------|-------|
| Timebase | 200 µs/div to 1 ms/div | Adjust to see 1-2 complete cycles |
| Voltage Scale | 500 mV/div or 1 V/div | DAC outputs 0-3.3V (typically 1.65V center) |
| Coupling | DC | Shows the full waveform including DC offset |
| Trigger | Channel 1, Rising Edge | Stabilizes the display |
| Trigger Level | ~1.65V | Middle of waveform |
| Acquisition | Normal | Standard mode |

### For 40 kHz Testing (Production Frequency)

| Setting | Value | Notes |
|---------|-------|-------|
| Timebase | 5 µs/div to 10 µs/div | One cycle = 25 µs at 40 kHz |
| Voltage Scale | 500 mV/div | Same as above |
| Coupling | DC | Same as above |
| Trigger | Channel 1, Rising Edge | Same as above |
| Bandwidth Limit | OFF | Need full bandwidth for 40 kHz |

## What to Look For

### 1. Basic Waveform Verification

**At 0° Phase Shift:**
- Both channels should show identical sine waves
- Waves should be in perfect synchronization (overlapping)
- Amplitude should be approximately 0-3.3V (or centered around 1.65V if DC-coupled)

**At 90° Phase Shift:**
- Channel 2 should be shifted by 1/4 cycle relative to Channel 1
- When Channel 1 is at its peak, Channel 2 should be at zero crossing

**At 180° Phase Shift:**
- Channels should be perfectly inverted (180° out of phase)
- When Channel 1 is at maximum, Channel 2 should be at minimum

### 2. Using X-Y Mode (Lissajous Patterns)

**How to Enable:**
- Most oscilloscopes have an X-Y mode button
- Channel 1 → X-axis
- Channel 2 → Y-axis

**What You Should See:**
- **0° phase:** A diagonal line at 45° (or -45°)
- **90° phase:** A perfect circle or ellipse
- **180° phase:** A diagonal line at -45° (or 45°)
- **Other phases:** Ellipses with different orientations

This is the **best way to verify phase relationships!**

### 3. Signal Quality Checks

✅ **Good Signals:**
- Clean, smooth sine waves
- No visible noise or distortion
- Stable frequency (use frequency measurement function)
- Consistent amplitude

❌ **Problems to Watch For:**
- Excessive noise or distortion
- Frequency instability (jitter)
- Amplitude variations
- Missing samples or glitches

## Test Procedure

### Step 1: Initial Verification
1. Start the test mode
2. Set phase to 0°
3. Verify both channels show identical sine waves
4. Measure frequency (should match your test frequency)

### Step 2: Phase Relationship Testing
1. Set phase to 0° - verify waves overlap
2. Set phase to 90° - verify 1/4 cycle shift
3. Set phase to 180° - verify waves are inverted
4. Set phase to 270° - verify 3/4 cycle shift

### Step 3: Phase Sweep Test
1. Use the 'w' command for automatic phase sweep
2. Observe the phase relationship changing smoothly
3. In X-Y mode, you should see the pattern rotating

### Step 4: Frequency Testing
1. Test at 1 kHz (easy to see)
2. Test at 10 kHz (intermediate)
3. Test at 40 kHz (production frequency)

## Serial Commands (Test Mode)

| Command | Function |
|---------|----------|
| `s` | Start waveforms |
| `x` | Stop waveforms |
| `p` | Set phase (will prompt for value) |
| `f` | Set frequency (will prompt for value) |
| `w` | Sweep phase 0° to 360° |
| `0` | Set phase to 0° |
| `9` | Set phase to 90° |
| `1` | Set phase to 180° |
| `2` | Set phase to 270° |
| `i` | Show current info |

## Troubleshooting

### No Signal on Oscilloscope
- Check connections (probe, ground, GPIO pins)
- Verify DAC is enabled in code
- Check if system is running (send 's' command)
- Verify oscilloscope is not in AC coupling (use DC)

### Distorted Waveform
- Check power supply (ESP32 needs stable 3.3V)
- Reduce probe loading (use 10X probe if available)
- Check for ground loops
- Verify frequency is within DAC capabilities

### Phase Not Changing
- Verify Channel 2 is using timer interrupt (not hardware generator)
- Check serial commands are being received
- Restart the system

### Frequency Mismatch
- Hardware generator frequency calculation may need adjustment
- Timer interrupt sample rate may need tuning
- Check RTC clock frequency settings

## Expected Results

### At 1 kHz Test Frequency:
- **Waveform:** Clean sine wave
- **Frequency:** 1000 Hz ± 1%
- **Amplitude:** ~0-3.3V peak-to-peak
- **Phase Accuracy:** ±2° or better

### At 40 kHz Production Frequency:
- **Waveform:** Clean sine wave (may show slight quantization)
- **Frequency:** 40000 Hz ± 0.5%
- **Amplitude:** ~0-3.3V peak-to-peak
- **Phase Accuracy:** ±5° (timer interrupt limitations)

## Advanced: Measuring Phase with Oscilloscope

### Method 1: Time Difference
1. Trigger on Channel 1 rising edge
2. Measure time difference to Channel 2 rising edge
3. Calculate: Phase = (Time Difference / Period) × 360°

### Method 2: X-Y Mode (Most Accurate)
1. Enable X-Y mode
2. Measure ellipse parameters
3. Calculate phase from ellipse shape

### Method 3: Math Function
1. Use oscilloscope's math function (Ch1 - Ch2)
2. Measure time between zero crossings
3. Calculate phase difference

## Next Steps

Once you've verified:
1. ✅ Both channels produce clean sine waves
2. ✅ Frequency is accurate
3. ✅ Phase shifting works correctly
4. ✅ No excessive noise or distortion

You're ready to connect ultrasonic transducers and test acoustic levitation!

## Safety Notes

- **Never** connect oscilloscope probes directly to ultrasonic transducers
- Always test with oscilloscope first
- Use appropriate coupling capacitors if needed
- Be aware of high voltages in some transducer circuits
- Follow your oscilloscope's safety guidelines

## Additional Resources

- ESP32 DAC Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html
- Oscilloscope Basics: Check your oscilloscope manual
- Lissajous Patterns: Great for phase measurement visualization

