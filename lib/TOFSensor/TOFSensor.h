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

class TOFSensor {
private:
    VL53L8CX* sensor;
    uint8_t pwren_pin;
    uint8_t sensor_address;
    bool is_active;
    uint8_t resolution;
    VL53L8CX_ResultsData results;  // Back to direct member for now (I2C/DMA may need internal RAM)
    uint16_t rotation;  // Display rotation in degrees (0, 90, 180, 270)
    
    void powerCycleSensor();
    bool initSensorWithRetry();
    
public:
    TOFSensor(uint8_t pwrenPin, uint8_t address);
    ~TOFSensor();
    
    bool begin();
    bool update();
    bool isActive() { return is_active; }
    
    // Coordinate rotation (0, 90, 180, 270 degrees) - used by visualizer and interaction manager
    uint16_t getRotation() const { return rotation; }
    void setRotation(uint16_t rot) { rotation = rot; }
    
    // Get distance data for visualization
    int16_t getDistance(uint8_t x, uint8_t y);
    uint8_t getNumZones();
    uint8_t getZonesPerLine();
    
    // Get raw results
    VL53L8CX_ResultsData* getResults() { return &results; }
};
