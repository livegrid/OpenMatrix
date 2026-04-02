# VL53L8CX Time-of-Flight Sensor Integration

## Overview
This document describes the integration of the VL53L8CX Time-of-Flight (TOF) sensor into the OpenMatrix project with visual display on the 64x64 LED matrix.

## Files Created

### 1. `lib/TOFSensor/TOFSensor.h` & `TOFSensor.cpp`
- Main sensor driver class
- Handles sensor initialization with retry logic
- Manages I2C communication at 200kHz for stability
- Configured for 8x8 resolution (64 zones)
- Ranging frequency: 15Hz
- Power management with GPIO pin 2

### 2. `lib/TOFSensor/TOFVisualizer.h` & `TOFVisualizer.cpp`
- Visualizes sensor data on the 64x64 LED matrix
- Each of the 8x8 sensor zones is displayed as an 8x8 pixel block
- **Color mapping (Heat Map Style):**
  - **Red**: Close objects (100mm)
  - **Yellow/Green**: Medium distance (~1000mm)
  - **Blue**: Far objects (2000mm)
  - **Black**: No detection

### 3. Configuration Updates
- `include/GeneralSettings.h`: Added `VL53L8CX_ENABLED` flag
- `src/main.cpp`: Integrated sensor task and visualization

## Hardware Configuration

### Pin Assignments
- **PWREN_PIN_1**: GPIO 2 (Power enable for sensor 1)
- **LPN_PIN**: -1 (Not used)
- **I2C Address**: 0x52 (after initialization)

### I2C Configuration
- **Bus Speed**: 200kHz (reduced for stability)
- **Default Address**: 0x29 (changed to 0x52 during init)

## Features

### Sensor Settings
- **Resolution**: 8x8 (64 zones)
- **Integration Time**: 20ms
- **Ranging Frequency**: 15Hz
- **Update Rate**: 10Hz (100ms per update)
- **Distance Range**: Uses shared `TOF_MIN_DETECTION_DIST` to `TOF_MAX_DETECTION_DIST` defaults (configurable)

### Error Handling
- Automatic retry on initialization failure (3 attempts)
- Power cycling between retries
- Sensor health monitoring
- Task-safe operation with FreeRTOS

## Usage

### Enabling the Sensor
The sensor is automatically enabled when `VL53L8CX_ENABLED` is defined in `GeneralSettings.h`.

### Visual Display
When the sensor is active, the TOF visualization replaces the Aquarium mode display:
- The matrix shows a real-time heat map of distances
- Each 8x8 pixel block represents one sensor zone
- Move your hand over the sensor to see the colors change

### Distance Range Adjustment
To adjust the distance mapping range, modify the values in `main.cpp`:
```cpp
tofVisualizer->setDistanceRange(TOF_MIN_DETECTION_DIST, TOF_MAX_DETECTION_DIST);
```

## Task Management

### TOF Task
- **Name**: "TOFTask"
- **Stack Size**: 8192 bytes (8KB - VL53L8CX initialization requires significant stack)
- **Priority**: 1
- **Core**: 0
- **Function**: Reads sensor data at 10Hz

### Display Task Integration
The visualization is integrated into the existing display task:
- Checks if sensor is active
- Renders heat map visualization
- Falls back to Aquarium mode if sensor inactive

## Debugging

### Serial Output
Enable debug logging to monitor sensor status:
- Sensor initialization messages
- Data ready status
- Distance readings
- Error messages

### LED Indicator
- GPIO 2 controls both power and LED indicator
- LED off when system is running normally

## Future Enhancements

### Planned Features
1. Support for multiple sensors (3 total)
2. Sensor data overlay on existing modes
3. Gesture detection
4. Distance-based effects triggering
5. Web interface controls

### Multiple Sensor Setup
The architecture supports 3 sensors:
- Sensor 1: Pin 2, Address 0x52
- Sensor 2: Pin 11, Address 0x54
- Sensor 3: Pin 12, Address 0x56

## Testing

### Basic Test Procedure
1. Upload firmware with `VL53L8CX_ENABLED` defined
2. Monitor serial output for initialization messages
3. Observe the matrix display in Aquarium mode
4. Move your hand 10-200cm from the sensor
5. Verify color changes from red (close) to blue (far)

### Expected Behavior
- Matrix displays 8x8 grid of colored blocks
- Colors update smoothly as objects move
- No detection shows as black
- Smooth color transitions based on distance

## Troubleshooting

### Sensor Won't Initialize
- Check I2C connections (SDA, SCL)
- Verify power on GPIO 2
- Check serial output for error codes
- Try power cycling the ESP32

### No Display
- Verify `VL53L8CX_ENABLED` is defined
- Check that mode is set to AQUARIUM
- Monitor serial for "TOF Task" messages

### Incorrect Colors
- Adjust distance range with `setDistanceRange()`
- Check sensor is not covered or obstructed
- Verify power stability

## Technical Notes

### Memory Usage
- TOFSensor object: ~140 bytes
- TOFVisualizer object: ~32 bytes  
- Task stack: 8192 bytes (8KB)
- Sensor internal: ~16KB for firmware

### Performance
- Minimal impact on display FPS
- Sensor updates independent of display
- Thread-safe operation

## References
- VL53L8CX Datasheet: STMicroelectronics
- Example code: `src/main.sensortest`
- Three-sensor setup guide: `3_SENSOR_SETUP_README.md`
