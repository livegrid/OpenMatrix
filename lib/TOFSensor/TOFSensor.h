#pragma once

#include <Arduino.h>
#include <vl53l8cx.h>
#include <Wire.h>

// Pin definitions for sensor 1
#define TOF_LPN_PIN -1
#define TOF_PWREN_PIN_1 12
#define TOF_PWREN_PIN_2 13
#define TOF_PWREN_PIN_3 11
#define TOF_SENSOR_1_ADDRESS 0x52

// Sensor configuration
#define TOF_MAX_INIT_RETRIES 3
#define TOF_RANGING_FREQUENCY 15  // 15 Hz

// Shared detection range configuration (mm)
#define TOF_MIN_DETECTION_DIST 800
#define TOF_MAX_DETECTION_DIST 1800

// Shared TOF coordinate rotation (degrees)
#define TOF_DEFAULT_ROTATION 90

class TOFSensor {
private:
    VL53L8CX* sensor;
    uint8_t pwren_pin;
    uint8_t sensor_address;
    bool is_active;
    uint8_t resolution;
    VL53L8CX_ResultsData results;  // Back to direct member for now (I2C/DMA may need internal RAM)
    uint16_t rotation;  // Physical mounting rotation in degrees (0, 90, 180, 270)
    uint32_t ranging_frame_id;  // Increments on each new ranging sample (for analysis sync)
    
    void powerCycleSensor();
    bool initSensorWithRetry();
    
public:
    TOFSensor(uint8_t pwrenPin, uint8_t address);
    ~TOFSensor();
    
    bool begin();
    bool restart() { return begin(); }
    bool update();
    bool isActive() { return is_active; }

    /** Monotonic id; only changes when update() returns true (new frame). */
    uint32_t getRangingFrameId() const { return ranging_frame_id; }
    
    // Physical mounting rotation (0, 90, 180, 270). Applied inside getDistance().
    uint16_t getRotation() const { return rotation; }
    void setRotation(uint16_t rot) { rotation = rot; }
    
    // Display-aligned grid (0..7): same layout as TOFVisualizer / panel. Rotation is applied here.
    int16_t getDistance(uint8_t x, uint8_t y);
    // Legacy/native 8x8 indices (pre-rotation convention) -> pass through toDisplayAligned()
    // before getDistance() when composing effect-specific offsets written for the old API.
    void toDisplayAligned(uint8_t nativeX, uint8_t nativeY, uint8_t& outX, uint8_t& outY) const;
    /** Inverse of toDisplayAligned — display indices → native grid before physical rotation. */
    void fromDisplayAligned(uint8_t dispX, uint8_t dispY, uint8_t& nativeX, uint8_t& nativeY) const;
    // Generic 8x8 coordinate rotation helper for effects (rotDeg: 0/90/180/270).
    static void rotateGrid8x8(uint8_t x, uint8_t y, int16_t rotDeg, uint8_t& outX, uint8_t& outY);
    /** Inverse of rotateGrid8x8 — undo an effect-local rotation. */
    static void inverseRotateGrid8x8(uint8_t x, uint8_t y, int16_t rotDeg, uint8_t& outX, uint8_t& outY);
    uint8_t getNumZones();
    uint8_t getZonesPerLine();
    
    // Get raw results
    VL53L8CX_ResultsData* getResults() { return &results; }
};
