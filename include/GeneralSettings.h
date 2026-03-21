#pragma once

#define FIRMWARE_VERSION_MAJOR 2
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0

#define PANEL_UPCYCLED 1

#define RUN_DEMO 1

#define AQUARIUM_ENABLED 1
#define SCD40_ENABLED 1
#define ADXL345_ENABLED 1
#define BH1750_ENABLED 1
#define TOUCH_ENABLED 1
// #define VL53L8CX_ENABLED 1  // Time-of-Flight depth sensor (optional)

#define WIFI_ENABLED 1

#ifdef VL53L8CX_ENABLED
#define TOF_PWREN_PIN_1 2          // Power enable pin for TOF sensor
#define TOF_SENSOR_1_ADDRESS 0x29  // I2C address
#endif

#define STATE_SAVE_INTERVAL 30  //in minutes
#define MATRIX_REFRESH_INTERVAL 300000  //in ms