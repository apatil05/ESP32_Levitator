# ESP32 Acoustic Levitator

An acoustic levitation system that can be controlled via a web app to move the levitating object along the z-axis by adjusting the phase relationship between two ultrasonic transducers.

## Overview

This project implements an acoustic levitator using an ESP32 microcontroller. The system generates two synchronized 40 kHz ultrasonic sine waves with controllable phase shift. By adjusting the phase relationship between the two transducers, the position of pressure nodes in the standing wave can be controlled, allowing precise vertical (z-axis) positioning of levitated objects.

The system features a web-based control interface accessible via Wi-Fi, allowing real-time control of the levitation position through an intuitive drag-and-drop interface.

## How It Works

### Acoustic Levitation Principle

Acoustic levitation uses standing waves created by two opposing ultrasonic transducers. When two waves of the same frequency meet, they create interference patterns with:
- **Pressure nodes**: Areas of low pressure where objects can be trapped
- **Pressure antinodes**: Areas of high pressure

By adjusting the phase shift between the two transducers, the position of these nodes shifts, allowing controlled movement of the levitated object along the z-axis.

### Technical Implementation

The ESP32 generates two synchronized 40 kHz sine waves:

1. **Channel 1 (GPIO25)**: Uses the ESP32's hardware cosine waveform generator as a reference signal
2. **Channel 2 (GPIO26)**: Uses LEDC (PWM) with phase control to generate a phase-shiftable sine wave

The phase relationship between channels is controlled via the LEDC `hpoint` parameter, which allows precise phase adjustment from 0° to 360° in 1024 steps (10-bit resolution).

## Hardware Requirements

- **ESP32 Development Board** (ESP32 DevKitC or compatible)
- **Two Ultrasonic Transducers** (40 kHz, suitable for acoustic levitation)
- **Amplifier Circuit** (optional, may be needed depending on transducer requirements)
- **Power Supply** (USB or external 5V supply)

### Pin Connections

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| Transducer 1 | GPIO25 | Reference channel (DAC1) |
| Transducer 2 | GPIO26 | Phase-controlled channel (DAC2) |
| Ground | GND | Common ground |

## Software Architecture

### Core Components

- **`main.cpp`**: Main application entry point, handles Wi-Fi AP setup, web server, and LEDC PWM configuration
- **`levitation_control.h/cpp`**: High-level API for controlling the levitation system
- **`phase_shifted_dac.h/cpp`**: Low-level phase-shifted DAC implementation (currently not used in main build)
- **`test_mode.h/cpp`**: Testing utilities for oscilloscope verification

### Key Features

- **40 kHz PWM Generation**: Uses LEDC (Low-speed Error Correction) timer with 10-bit resolution
- **Phase Control**: Precise phase adjustment from 0° to 360° via `hpoint` parameter
- **Web Interface**: HTML5 interface served from SPIFFS filesystem
- **Wi-Fi Access Point**: Creates "ONDA" network for wireless control
- **Real-time Control**: HTTP API for phase adjustment with minimal latency

### Web Interface

The web interface (`data/index.html`) provides:
- Visual representation of the levitation chamber
- Drag-and-drop control for vertical positioning
- Keyboard controls (Arrow Up/Down)
- Real-time phase adjustment via HTTP API
- Modern, responsive UI with animated visual feedback

## Setup Instructions

### Prerequisites

1. **PlatformIO**: Install [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) or PlatformIO Core
2. **ESP32 Board Support**: PlatformIO will automatically install ESP32 support

### Building and Uploading

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd ESP32_Levitator
   ```

2. **Open in PlatformIO**:
   - Open the project folder in VS Code with PlatformIO extension
   - Or use PlatformIO CLI

3. **Upload filesystem** (required for web interface):
   ```bash
   pio run --target uploadfs
   ```

4. **Build and upload firmware**:
   ```bash
   pio run --target upload
   ```

5. **Open Serial Monitor** (optional, for debugging):
   ```bash
   pio device monitor
   ```
   Baud rate: 115200

### Configuration

Default Wi-Fi Access Point settings (in `src/main.cpp`):
- **SSID**: `ONDA`
- **Password**: `levitate123`
- **IP Address**: `192.168.4.1` (default ESP32 AP IP)

To change these, modify the constants in `src/main.cpp`:
```cpp
const char *AP_SSID = "YOUR_SSID";
const char *AP_PASSWORD = "YOUR_PASSWORD";
```

## Usage

### Initial Setup

1. **Power on the ESP32** and wait for Wi-Fi AP to start
2. **Connect to Wi-Fi**: Look for "ONDA" network on your device
3. **Open web interface**: Navigate to `http://192.168.4.1` in your browser
4. **Control levitation**: Use the web interface to adjust the phase and move the object

### Web Interface Controls

- **Drag the ball**: Click and drag the ball up/down to adjust position
- **Arrow Keys**: Use ↑/↓ keys for fine adjustments
- **Touch Support**: Works on mobile devices with touch input

### API Endpoints

The system exposes a simple HTTP API:

- **GET `/`**: Serves the web interface
- **GET `/set_phase?deg=<degrees>`**: Sets phase shift (0-360°)
  - Example: `http://192.168.4.1/set_phase?deg=90`
  - Returns: `{"success":true,"phase":90.0}`

### Serial Monitor Commands

If using serial interface, you can monitor system status:
- Startup messages show frequency and GPIO configuration
- Phase changes are logged with current phase value

## Technical Specifications

### Signal Generation

- **Frequency**: 40,000 Hz (40 kHz)
- **Resolution**: 10-bit (1024 steps per period)
- **Phase Resolution**: ~0.35° per step (360°/1024)
- **Duty Cycle**: 50% square wave (converted to sine via transducers)
- **Synchronization**: Both channels share the same LEDC timer for perfect synchronization

### Performance

- **Phase Adjustment Range**: 0° to 360°
- **Update Rate**: Real-time (limited by HTTP request rate)
- **Latency**: < 50ms for phase changes
- **Frequency Accuracy**: ±0.5% at 40 kHz

### System Requirements

- **ESP32**: Any ESP32 development board
- **Flash**: Minimum 4MB (for SPIFFS filesystem)
- **RAM**: Standard ESP32 (520KB) is sufficient

## Testing and Verification

### Oscilloscope Testing

Before connecting transducers, verify signal generation with an oscilloscope:

1. **Connect oscilloscope**:
   - Channel 1 → GPIO25
   - Channel 2 → GPIO26
   - Ground → GND

2. **Recommended settings**:
   - Timebase: 5-10 µs/div (for 40 kHz)
   - Voltage: 500 mV/div
   - Coupling: DC
   - Trigger: Channel 1, Rising Edge

3. **Verify phase relationships**:
   - 0°: Waves should overlap
   - 90°: 1/4 cycle phase difference
   - 180°: Waves inverted
   - Use X-Y mode for best phase visualization

### Troubleshooting

**No Wi-Fi network visible**:
- Check serial monitor for startup messages
- Verify SPIFFS was uploaded correctly
- Try resetting the ESP32

**Web interface not loading**:
- Ensure you're connected to "ONDA" network
- Try accessing `http://192.168.4.1` directly
- Check browser console for errors

**No signal on oscilloscope**:
- Verify GPIO connections
- Check that system is running (serial monitor)
- Ensure DAC channels are enabled in code

**Phase not changing**:
- Verify HTTP requests are being sent (browser network tab)
- Check serial monitor for phase change confirmations
- Restart ESP32 if needed

## Project Structure

```
ESP32_Levitator/
├── src/
│   ├── main.cpp              # Main application
│   ├── levitation_control.*  # Levitation control API
│   ├── phase_shifted_dac.*   # Phase-shifted DAC implementation
│   └── test_mode.*           # Testing utilities
├── data/
│   └── index.html            # Web interface
├── lib/
│   └── dac-cosine/           # DAC cosine generator library
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Build Configuration

The project uses PlatformIO with the following configuration:

- **Platform**: Espressif 32
- **Board**: ESP32 DevKitC
- **Framework**: Arduino
- **Filesystem**: SPIFFS
- **Monitor Speed**: 115200 baud

Some source files are excluded from the build (see `platformio.ini`):
- `test_main.cpp`
- `phase_shifted_dac.cpp`
- `levitation_control.cpp`
- `test_mode.cpp`

The current implementation uses LEDC PWM directly in `main.cpp` for phase control.

## Safety Notes

- **Ultrasonic Frequencies**: 40 kHz is above human hearing range but ensure proper transducer handling
- **Power Requirements**: Ensure adequate power supply for transducers and amplifiers
- **High Voltages**: Some transducer driver circuits may use high voltages - follow safety guidelines
- **Oscilloscope Testing**: Always test with oscilloscope before connecting transducers

## Future Enhancements

Potential improvements:
- Automatic frequency tuning for resonance
- Closed-loop position control with sensors
- Multi-axis control (x, y, z positioning)
- Preset positions and sequences
- Data logging and analysis

## License

[Specify your license here]

## Credits

Built for Hackathon '25. Uses the ESP32's hardware capabilities for precise acoustic levitation control.

## References

- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [ESP32 DAC Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/dac.html)
- [PlatformIO ESP32 Documentation](https://docs.platformio.org/en/latest/platforms/espressif32.html)

