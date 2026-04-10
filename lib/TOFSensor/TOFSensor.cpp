#include "TOFSensor.h"

namespace {

// Forward: native sample grid (x,y) -> display-aligned cell (matches former TOFVisualizer mapping).
void forwardRotate8x8(uint8_t x, uint8_t y, uint16_t rotDeg, uint8_t& outX, uint8_t& outY) {
    switch (rotDeg % 360) {
        case 90:
            outX = 7 - y;
            outY = x;
            break;
        case 180:
            outX = 7 - x;
            outY = 7 - y;
            break;
        case 270:
            outX = y;
            outY = 7 - x;
            break;
        default:
            outX = x;
            outY = y;
            break;
    }
}

// Inverse: display-aligned (lx,ly) -> native sample indices for zone lookup.
void inverseRotate8x8(uint8_t lx, uint8_t ly, uint16_t rotDeg, uint8_t& sx, uint8_t& sy) {
    switch (rotDeg % 360) {
        case 90:
            sx = ly;
            sy = 7 - lx;
            break;
        case 180:
            sx = 7 - lx;
            sy = 7 - ly;
            break;
        case 270:
            sx = 7 - ly;
            sy = lx;
            break;
        default:
            sx = lx;
            sy = ly;
            break;
    }
}

}  // namespace

TOFSensor::TOFSensor(uint8_t pwrenPin, uint8_t address) 
    : pwren_pin(pwrenPin), sensor_address(address), is_active(false), rotation(TOF_DEFAULT_ROTATION),
      ranging_frame_id(0) {
    // VL53L8CX object must be in internal RAM (contains hardware pointers for I2C)
    sensor = new VL53L8CX(&Wire, TOF_LPN_PIN);
    resolution = VL53L8CX_RESOLUTION_8X8;
    
    // Results buffer in internal RAM - VL53L8CX library writes via I2C/DMA
    // which may not work properly with PSRAM
    log_i("TOF: Using internal RAM for results buffer (%d bytes)", sizeof(VL53L8CX_ResultsData));
    memset(&results, 0, sizeof(VL53L8CX_ResultsData));
}

TOFSensor::~TOFSensor() {
    if (sensor && is_active) {
        sensor->stop_ranging();
    }
    delete sensor;
}

void TOFSensor::powerCycleSensor() {
    log_i("TOF: Power cycling sensor on pin %d", pwren_pin);
    digitalWrite(pwren_pin, LOW);
    delay(100);  // Keep power off longer to ensure full reset
    digitalWrite(pwren_pin, HIGH);
    delay(200);  // Wait for sensor to boot (needs ~200ms)
}

bool TOFSensor::initSensorWithRetry() {
    uint8_t status;
    
    for (uint8_t attempt = 1; attempt <= TOF_MAX_INIT_RETRIES; attempt++) {
        log_i("TOF: Init attempt %d/%d", attempt, TOF_MAX_INIT_RETRIES);
        
        // If retry, power cycle the sensor
        if (attempt > 1) {
            log_i("TOF: Retrying... power cycling sensor");
            powerCycleSensor();
            delay(200);  // Extra time after power cycle for sensor to stabilize
        }
        
        // Begin communication
        sensor->begin();
        
        // Initialize sensor
        status = sensor->init();
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Init failed with status: %d", status);
            continue;
        }
        log_i("TOF: Init OK");
        
        // Set resolution to 8x8
        status = sensor->set_resolution(resolution);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to set resolution");
            continue;
        }
        log_i("TOF: Resolution set to 8x8");
        
        // Set ranging frequency
        status = sensor->set_ranging_frequency_hz(TOF_RANGING_FREQUENCY);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to set ranging frequency");
            continue;
        }
        log_i("TOF: Ranging frequency set to %d Hz", TOF_RANGING_FREQUENCY);
        
        // Start ranging
        status = sensor->start_ranging();
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to start ranging");
            continue;
        }
        log_i("TOF: Ranging started successfully!");
        
        return true;  // Success!
    }
    
    // All retries failed
    log_e("TOF: Sensor FAILED after all retries");
    digitalWrite(pwren_pin, LOW);  // Power off failed sensor
    return false;
}

bool TOFSensor::begin() {
    pinMode(TOF_PWREN_PIN_2, OUTPUT);
    digitalWrite(TOF_PWREN_PIN_2, LOW);
    pinMode(TOF_PWREN_PIN_3, OUTPUT);
    digitalWrite(TOF_PWREN_PIN_3, LOW);
    
    log_i("TOF: Initializing sensor...");
    
    // Set up power pin - POWER ON FIRST like the working example
    pinMode(pwren_pin, OUTPUT);
    digitalWrite(pwren_pin, HIGH);  // Power on immediately
    delay(100);  // Give sensor time to power up
    
    // Initialize I2C (use higher speed for faster data transfers)
    Wire.begin();
    Wire.setClock(400000);  // 400kHz: within datasheet fast-mode, reduces read latency
    log_i("TOF: I2C initialized at 400kHz");
    
    // Initialize sensor
    is_active = initSensorWithRetry();
    
    if (is_active) {
        log_i("TOF: Sensor initialized successfully");
    } else {
        log_e("TOF: Sensor initialization failed");
    }
    
    return is_active;
}

bool TOFSensor::update() {
    if (!is_active) {
        return false;
    }
    
    uint8_t NewDataReady = 0;
    uint8_t status = sensor->check_data_ready(&NewDataReady);
    
    if (status != VL53L8CX_STATUS_OK) {
        log_e("TOF: Check data ready failed");
        return false;
    }
    
    if (NewDataReady) {
        status = sensor->get_ranging_data(&results);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Get ranging data failed");
            return false;
        }
        ++ranging_frame_id;
        return true;  // New data available
    }
    
    return false;  // No new data yet
}

void TOFSensor::toDisplayAligned(uint8_t nativeX, uint8_t nativeY, uint8_t& outX, uint8_t& outY) const {
    forwardRotate8x8(nativeX, nativeY, rotation, outX, outY);
}

void TOFSensor::fromDisplayAligned(uint8_t dispX, uint8_t dispY, uint8_t& nativeX, uint8_t& nativeY) const {
    inverseRotate8x8(dispX, dispY, rotation, nativeX, nativeY);
}

void TOFSensor::rotateGrid8x8(uint8_t x, uint8_t y, int16_t rotDeg, uint8_t& outX, uint8_t& outY) {
    int16_t normalized = rotDeg % 360;
    if (normalized < 0) normalized += 360;
    forwardRotate8x8(x, y, (uint16_t)normalized, outX, outY);
}

void TOFSensor::inverseRotateGrid8x8(uint8_t x, uint8_t y, int16_t rotDeg, uint8_t& outX, uint8_t& outY) {
    int16_t normalized = rotDeg % 360;
    if (normalized < 0) normalized += 360;
    inverseRotate8x8(x, y, (uint16_t)normalized, outX, outY);
}

int16_t TOFSensor::getDistance(uint8_t x, uint8_t y) {
    if (!is_active || x >= 8 || y >= 8) {
        return -1;
    }
    
    uint8_t sx, sy;
    inverseRotate8x8(x, y, rotation, sx, sy);
    // Convert native x,y to zone index (VL53L8CX ordering + mirror)
    uint8_t zone = sy * 8 + (7 - sx);
    
    if (results.nb_target_detected[zone] > 0) {
        return results.distance_mm[zone * VL53L8CX_NB_TARGET_PER_ZONE];
    }
    
    return -1;  // No target detected
}

uint8_t TOFSensor::getNumZones() {
    return resolution == VL53L8CX_RESOLUTION_8X8 ? 64 : 16;
}

uint8_t TOFSensor::getZonesPerLine() {
    return resolution == VL53L8CX_RESOLUTION_8X8 ? 8 : 4;
}
