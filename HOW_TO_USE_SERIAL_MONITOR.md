# How to Use Serial Monitor and Send Commands

## Quick Answer

**You type commands directly in the terminal where the serial monitor is running!**

The terminal at the bottom of VS Code IS the serial monitor. Just click on it and start typing!

---

## Step-by-Step Instructions

### Method 1: Using PlatformIO Serial Monitor (Recommended)

1. **Open Serial Monitor:**
   - Click the "plug" icon in the PlatformIO toolbar (bottom of VS Code)
   - OR press `Ctrl+Alt+S` (Mac: `Cmd+Option+S`)
   - OR use the terminal command: `pio device monitor`

2. **The terminal will show:**
   ```
   Terminal on /dev/cu.usbserial-XXXX | 921600 8-N-1
   ```
   This means it's connected and ready!

3. **To send commands:**
   - **Click on the terminal** (the bottom panel where you see the serial output)
   - **Just start typing!** Type `0` and press Enter
   - Type `9` and press Enter
   - Type `u` and press Enter
   - etc.

4. **Important:** 
   - Make sure the terminal is **focused** (click on it)
   - Type your command and press **Enter**
   - You should see the ESP32 respond with messages like "Phase set to 0°"

---

## Method 2: Using PlatformIO Serial Monitor with Filters (Alternative)

If the direct typing doesn't work, you can use PlatformIO's built-in filters:

1. Press `Ctrl+T` in the serial monitor
2. Then press `Ctrl+H` to see help
3. Use filters to send data

But **Method 1 should work** - just type directly in the terminal!

---

## Method 3: Use a Separate Serial Terminal (If Needed)

If you're having trouble with PlatformIO's serial monitor, you can use a separate program:

### On macOS:
- **Screen** (built-in):
  ```bash
  screen /dev/cu.usbserial-2130 921600
  ```
  (Replace `2130` with your actual port number)
  - To exit: Press `Ctrl+A` then `K`, then `Y`

- **CoolTerm** (download from http://freeware.the-meiers.org/)
- **Serial** (from the Mac App Store)

### On Windows:
- **PuTTY** (download from https://www.putty.org/)
- **Arduino IDE Serial Monitor**
- **Tera Term**

### On Linux:
- **Screen**:
  ```bash
  screen /dev/ttyUSB0 921600
  ```
- **Minicom**:
  ```bash
  minicom -D /dev/ttyUSB0 -b 921600
  ```

---

## Troubleshooting

### Problem: "I type but nothing happens"
**Solution:**
1. Make sure the terminal is focused (click on it)
2. Make sure you're in the serial monitor terminal (not a regular terminal)
3. Try pressing Enter after typing
4. Check that the ESP32 is actually running (you should see startup messages)

### Problem: "I see output but can't type"
**Solution:**
1. The terminal might be in "view-only" mode
2. Try stopping the monitor and starting it again
3. Make sure you're clicking directly on the terminal panel
4. Try using a separate serial terminal program (Method 3)

### Problem: "Commands don't do anything"
**Solution:**
1. Check that the ESP32 code is uploaded and running
2. Look for startup messages in the serial monitor
3. Make sure you're typing the correct commands (`0`, `1`, `2`, `9`, `u`, `d`, `s`, `t`)
4. Commands are case-sensitive - use lowercase

### Problem: "I see garbled text"
**Solution:**
1. Make sure the baud rate is set to 921600 (check platformio.ini)
2. Make sure both the ESP32 and serial monitor use the same baud rate
3. Try a lower baud rate like 115200 if 921600 doesn't work

---

## What You Should See

When everything is working, you should see:

1. **On startup:**
   ```
   ========================================
   ESP32 Acoustic Levitation System
   OSCILLOSCOPE TEST MODE
   ========================================
   
   HARDWARE CONNECTIONS:
     Oscilloscope Ch1 -> GPIO25 (DAC1)
     ...
   ```

2. **When you type `0`:**
   ```
   Phase set to 0° - waves should overlap on scope
   ```

3. **When you type `s`:**
   ```
   --- Status ---
   Frequency: 1000.0 Hz
   Phase shift: 0.0°
   System: Running
   ```

---

## Quick Command Reference

| Command | Action |
|---------|--------|
| `0` | Set phase to 0° (waves overlap) |
| `9` | Set phase to 90° (1/4 cycle apart) |
| `1` | Set phase to 180° (waves inverted) |
| `2` | Set phase to 270° (3/4 cycle apart) |
| `u` | Increase phase by 5° |
| `d` | Decrease phase by 5° |
| `r` | Reset phase to 0° |
| `s` | Show status |
| `t` | Toggle system on/off |

---

## Visual Guide

```
┌─────────────────────────────────────┐
│  VS Code Window                     │
│                                     │
│  [Code Editor Area]                 │
│                                     │
│  ┌───────────────────────────────┐ │
│  │ TERMINAL (Serial Monitor)     │ │ ← This is where you type!
│  │                               │ │
│  │ > ESP32 output here...        │ │
│  │ > More output...              │ │
│  │                               │ │
│  │ [Type here and press Enter]   │ │ ← Click here and type!
│  └───────────────────────────────┘ │
└─────────────────────────────────────┘
```

**Just click on the terminal at the bottom and start typing!**

---

## Still Having Issues?

If you're still having trouble:
1. Make sure the ESP32 is connected via USB
2. Make sure the code is uploaded
3. Try restarting the serial monitor
4. Try using a separate serial terminal program (Method 3)
5. Check that the baud rate matches (921600)

The key thing to remember: **The terminal IS the serial monitor - just click on it and type!**

