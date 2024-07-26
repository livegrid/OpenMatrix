#ifndef SCD40_H
#define SCD40_H

#include <Arduino.h>
#include <scd4x.h>
#include <Wire.h>

class SCD40 {
 private:
  double temperature, humidity, co2;
  uint16_t error;
  char errorMessage[256];
  SCD4X scd4x;
  bool sensorAvailable = true;  // Track if the sensor is operational

  String uint16ToHex(uint16_t value) {
    String hexString = "";
    if (value < 4096) hexString += "0";
    if (value < 256) hexString += "0";
    if (value < 16) hexString += "0";
    hexString += String(value, HEX);
    return hexString;
  }

  String getSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
    String serialString = "Serial: 0x";
    serialString += uint16ToHex(serial0);
    serialString += uint16ToHex(serial1);
    serialString += uint16ToHex(serial2);
    return serialString;
  }

  static void measurementTaskFunction(void* parameter) {
    SCD40* instance = static_cast<SCD40*>(parameter);
    instance->runMeasurementTask();
  }

  void runMeasurementTask() {
    Wire.begin();
    scd4x.begin(Wire);
    
    if(scd4x.isConnected()) {
      log_i("SCD40 connected");

    // Check if auto-calibration is enabled
    if (scd4x.getCalibrationMode()) {
      // Disable auto-calibration
      scd4x.setCalibrationMode(false);

      // Save the settings to EEPROM
      scd4x.saveSettings();
    }

    scd4x.startPeriodicMeasurement();

    while (true) {
      if (scd4x.isDataReady()) {
        if (scd4x.readMeasurement(co2, temperature, humidity) == 0) {
          Serial.printf("CO2: %u ppm, Temperature: %.1f °C, Humidity: %.1f %%RH\n", co2, temperature, humidity);
          vTaskDelay(pdMS_TO_TICKS(4750));  // New data available after approximately 5 seconds
        }
        sensorAvailable = true;
      }
        vTaskDelay(pdMS_TO_TICKS(250));  // Check every 250ms
      }
    }
    else {
      log_i("SCD40 not connected");
      sensorAvailable = false;
    }
  }
 public:
  void init() {
    xTaskCreate(measurementTaskFunction, "MeasurementTask", 2048, this, 1, NULL);
  }
  bool isConnected() {
    return sensorAvailable;
  }
  long getTemperature() {
    // return ((int)(temperature * 100 + 0.5) / 100.0);
    return temperature;
  }
  long getHumidity() {
    // return ((int)(humidity * 100 + 0.5) / 100.0);
    return humidity;
  }
  long getCO2() {
    return (co2);
  }
};

#endif