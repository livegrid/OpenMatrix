#include "TOFSensor.h"

TOFSensor::TOFSensor(uint8_t pwrenPin, uint8_t address) 
    : pwren_pin(pwrenPin), sensor_address(address), is_active(false) {
    sensor = new VL53L8CX(&Wire, TOF_LPN_PIN);
    resolution = VL53L8CX_RESOLUTION_8X8;
    pinMode(TOF_PWREN_PIN_2, OUTPUT);
    digitalWrite(TOF_PWREN_PIN_2, LOW);
    pinMode(TOF_PWREN_PIN_3, OUTPUT);
    digitalWrite(TOF_PWREN_PIN_3, LOW);
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
    delay(50);
    digitalWrite(pwren_pin, HIGH);
    delay(50);
}

bool TOFSensor::initSensorWithRetry() {
    uint8_t status;
    
    for (uint8_t attempt = 1; attempt <= TOF_MAX_INIT_RETRIES; attempt++) {
        log_i("TOF: Init attempt %d/%d", attempt, TOF_MAX_INIT_RETRIES);
        
        // Power on sensor
        digitalWrite(pwren_pin, HIGH);
        delay(100);
        
        // Begin communication
        sensor->begin();
        
        // Initialize sensor
        status = sensor->init();
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Init failed with status: %d", status);
            powerCycleSensor();
            continue;
        }
        log_i("TOF: Init OK");
        
        // Change I2C address to avoid conflicts
        status = sensor->set_i2c_address(sensor_address);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to change I2C address");
            powerCycleSensor();
            continue;
        }
        log_i("TOF: I2C address changed to 0x%02X", sensor_address);
        
        // Set resolution to 8x8
        status = sensor->set_resolution(resolution);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to set resolution");
            powerCycleSensor();
            continue;
        }
        log_i("TOF: Resolution set to 8x8");
        
        // Set integration time to 20ms
        status = sensor->set_integration_time_ms(20);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to set integration time");
            powerCycleSensor();
            continue;
        }
        
        // Set ranging frequency
        status = sensor->set_ranging_frequency_hz(TOF_RANGING_FREQUENCY);
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to set ranging frequency");
            powerCycleSensor();
            continue;
        }
        log_i("TOF: Ranging frequency set to %d Hz", TOF_RANGING_FREQUENCY);
        
        // Start ranging
        status = sensor->start_ranging();
        if (status != VL53L8CX_STATUS_OK) {
            log_e("TOF: Failed to start ranging");
            powerCycleSensor();
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
    log_i("TOF: Initializing sensor...");
    
    // Set up power pin
    pinMode(pwren_pin, OUTPUT);
    digitalWrite(pwren_pin, LOW);
    delay(100);
    
    // Initialize I2C at 200kHz for stability
    Wire.begin();
    Wire.setClock(200000);
    log_i("TOF: I2C initialized at 200 kHz");
    
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
        return true;  // New data available
    }
    
    return false;  // No new data yet
}

int16_t TOFSensor::getDistance(uint8_t x, uint8_t y) {
    if (!is_active || x >= 8 || y >= 8) {
        return -1;
    }
    
    // Convert x,y to zone index
    // The sensor data is organized in a specific way
    uint8_t zone = y * 8 + (7 - x);  // Mirror x axis
    
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
