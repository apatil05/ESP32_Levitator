# Quick Start: Oscilloscope Testing

## Step 1: Enable Test Mode

Open `src/main.cpp` and make sure this line is set:
```cpp
#define TEST_MODE_OSCILLOSCOPE true
```

This sets the frequency to 1 kHz (easy to see on oscilloscope) instead of 40 kHz.

## Step 2: Hardware Connections

Connect your oscilloscope:
- **Channel 1** → **GPIO25** (DAC1 output)
- **Channel 2** → **GPIO26** (DAC2 output)  
- **Ground** → **GND** on ESP32

**Important:** Use proper oscilloscope probes with ground clips attached!

## Step 3: Oscilloscope Settings

For 1 kHz test frequency:
- **Timebase:** 200 µs/div to 1 ms/div (adjust to see 1-2 complete cycles)
- **Voltage:** 500 mV/div or 1 V/div
- **Coupling:** DC
- **Trigger:** Channel 1, Rising Edge, ~1.65V
- **Mode:** Normal (or X-Y mode for phase visualization)

## Step 4: Upload and Test

1. Upload the code to your ESP32
2. Open Serial Monitor at 921600 baud
3. The system will automatically start

## Step 5: Verify Waveforms

### Test Phase Relationships

Send these commands via Serial Monitor:

- **`0`** - Set phase to 0°
  - **Expected:** Both waves should overlap perfectly
  
- **`9`** - Set phase to 90°  
  - **Expected:** Channel 2 should be 1/4 cycle behind Channel 1
  
- **`1`** - Set phase to 180°
  - **Expected:** Waves should be inverted (180° out of phase)
  
- **`2`** - Set phase to 270°
  - **Expected:** Channel 2 should be 3/4 cycle behind Channel 1

### Use X-Y Mode (Recommended!)

Enable X-Y mode on your oscilloscope:
- Channel 1 → X-axis
- Channel 2 → Y-axis

**What you should see:**
- **0° phase:** Diagonal line (45° or -45°)
- **90° phase:** Perfect circle or ellipse
- **180° phase:** Diagonal line (opposite direction)
- **Other phases:** Ellipses with different orientations

This is the **best way** to verify phase relationships!

## Step 6: Verify Signal Quality

Check for:
- ✅ Clean, smooth sine waves
- ✅ No visible noise or distortion  
- ✅ Stable frequency (use scope's frequency measurement)
- ✅ Consistent amplitude
- ✅ Proper phase relationships

## Step 7: Test at Production Frequency

Once verified at 1 kHz, test at 40 kHz:

1. Change `TEST_MODE_OSCILLOSCOPE` to `false` in `main.cpp`
2. Adjust oscilloscope timebase to 5-10 µs/div
3. Verify waveforms are still clean
4. Check phase relationships still work

## Troubleshooting

### No Signal
- Check connections (probe, ground, GPIO pins)
- Verify system is running (send 's' command to check status)
- Make sure oscilloscope is in DC coupling mode

### Distorted Waveform
- Check ESP32 power supply (needs stable 3.3V)
- Use 10X probe if available (reduces loading)
- Check for ground loops

### Phase Not Changing
- Verify Channel 2 is working (should see waveform)
- Check serial commands are being received
- Restart the system

## What Success Looks Like

✅ **At 0°:** Waves overlap perfectly  
✅ **At 90°:** Clear 1/4 cycle phase difference  
✅ **At 180°:** Waves are inverted  
✅ **Clean waveforms:** No distortion or excessive noise  
✅ **Stable frequency:** Matches set frequency  

Once you see these results, you're ready to connect ultrasonic transducers!

## Next Steps

After successful oscilloscope verification:
1. Set `TEST_MODE_OSCILLOSCOPE` to `false` for production
2. Connect ultrasonic transducers to GPIO25 and GPIO26
3. Add appropriate amplifier/driver circuits if needed
4. Test acoustic levitation!

## Additional Resources

See `OSCILLOSCOPE_TEST_GUIDE.md` for detailed information and advanced testing procedures.

