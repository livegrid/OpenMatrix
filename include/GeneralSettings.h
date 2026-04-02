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

// #define TOF_DEBUG_ENABLED 1

// WiFi Buffer Configuration for ESP32-S3 RAM constraints
// Reduces WiFi buffers to minimum viable for connection (saves ~10KB RAM during init)
#ifdef WIFI_ENABLED
  #define WIFI_STATIC_RX_BUFFER_NUM  4   // Default: 10 (reduce to 4)
  #define WIFI_DYNAMIC_RX_BUFFER_NUM 8   // Default: 32 (reduce to 8)
  #define WIFI_DYNAMIC_TX_BUFFER_NUM 8   // Default: 32 (reduce to 8)
  #define WIFI_RX_MAX_SINGLE_PKT_LEN 1600  // Default: 1600 (keep)
  #define WIFI_TX_PKT_NUM_MIN       4    // Default: 6 (reduce to 4)
#endif

#define STATE_SAVE_INTERVAL 30  //in minutes
#define MATRIX_REFRESH_INTERVAL 300000  //in ms

// Debug profiling (enable temporarily)
// #define PROFILE_UPDATEFISH 1
