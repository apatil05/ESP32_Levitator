# Simple Guide: How to Send Commands to ESP32

## The Simple Answer

**Just click on the terminal at the bottom of VS Code and type your commands!**

---

## Step-by-Step:

1. **Look at the bottom of VS Code** - you should see a terminal panel
2. **Click on that terminal** (where you see the serial monitor output)
3. **Type a command** like `0` or `s`
4. **Press Enter**
5. **See the response** from your ESP32

That's it! The terminal IS your input/output interface.

---

## What to Type:

- `0` - Set phase to 0° (waves overlap)
- `9` - Set phase to 90°
- `1` - Set phase to 180° (waves inverted)
- `2` - Set phase to 270°
- `u` - Move up (increase phase)
- `d` - Move down (decrease phase)
- `s` - Show status
- `t` - Toggle on/off
- `r` - Reset to 0°

---

## If It's Not Working:

### Option 1: Make Sure Terminal is Focused
- Click directly on the terminal panel
- You should see a blinking cursor
- If no cursor, click again until you see it

### Option 2: Restart Serial Monitor
1. Stop the monitor (press `Ctrl+C` in the terminal)
2. Start it again (click the plug icon or press `Ctrl+Alt+S`)

### Option 3: Use a Separate Program
If typing in the terminal doesn't work, use **CoolTerm** (Mac) or **PuTTY** (Windows):

**For Mac (CoolTerm):**
1. Download from: http://freeware.the-meiers.org/
2. Open CoolTerm
3. Click "Options"
4. Set Port to: `/dev/cu.usbserial-2130` (or whatever port you see)
5. Set Baudrate to: `921600`
6. Click "OK"
7. Click "Connect"
8. Type commands in the window

**For Windows (PuTTY):**
1. Download from: https://www.putty.org/
2. Open PuTTY
3. Connection type: Serial
4. Serial line: `COM3` (or whatever port you see)
5. Speed: `921600`
6. Click "Open"
7. Type commands in the window

---

## Visual Example:

```
┌────────────────────────────────────┐
│  Your Code                         │
│                                    │
├────────────────────────────────────┤
│  TERMINAL (Serial Monitor)         │
│  ────────────────────────────────  │
│  > ESP32 output here...            │
│  > More messages...                │
│                                    │
│  0                                 │ ← You type here!
│  Phase set to 0°                   │ ← ESP32 responds
│                                    │
│  s                                 │ ← Type another command
│  Frequency: 1000.0 Hz              │ ← ESP32 responds
│  Phase shift: 0.0°                 │
│  System: Running                   │
│                                    │
│  [Cursor blinking here]            │ ← Click here and type!
└────────────────────────────────────┘
```

---

## Quick Test:

1. Click on terminal
2. Type: `s` and press Enter
3. You should see status information
4. If you see it, it's working! 🎉

---

## Still Confused?

The terminal at the bottom **IS** the serial monitor. There's no separate input box - you just type directly where you see the output!

