#pragma once

#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 2

// #define PANEL_UPCYCLED 1

// #define RUN_DEMO 1

#define AQUARIUM_ENABLED 1
// #define SCD40_ENABLED 1  // Temperature/Humidity/CO2 sensor (disable to save ~2KB RAM + reduce I2C contention)
// #define ADXL345_ENABLED 1   //AUTOROTATE
// #define BH1750_ENABLED 1
#define TOUCH_ENABLED 1
#define VL53L8CX_ENABLED 1  // Time-of-Flight sensor

// #define WIFI_ENABLED 1  // Comment out to disable WiFi, web server, and ServerTask (saves RAM)

#define STATE_SAVE_INTERVAL 30  //in minutes
#define MATRIX_REFRESH_INTERVAL 300000  //in ms

// Debug profiling (enable temporarily)
// #define PROFILE_UPDATEFISH 1
