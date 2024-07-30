#pragma once

#include <Arduino.h>
#include <scd4x.h>
#include <Wire.h>
#include "SCD40Settings.h"

class SCD40 {
 private:
  double temperature, humidity, co2;
  uint16_t error;
  char errorMessage[256];
  SCD4X scd4x;
  bool sensorAvailable = false;  // Track if the sensor is operational
  bool firstReadingReceived = false;  // Flag to indicate if the first reading has been received

  String uint16ToHex(uint16_t value);
  String getSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2);
  static void measurementTaskFunction(void* parameter);
  void runMeasurementTask();

 public:
  void init();
  bool isFirstReadingReceived();
  bool isConnected();
  long getTemperature();
  long getHumidity();
  long getCO2();
};
