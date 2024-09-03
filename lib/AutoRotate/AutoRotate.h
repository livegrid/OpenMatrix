#pragma once

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "Matrix.h"

class AutoRotate {
 private:
  Matrix* matrix;
  Adafruit_ADXL345_Unified accel;
  bool sensorWorking;
  float x, y, z;
  uint8_t currentRotation;

 public:
  AutoRotate(Matrix* matrix);
  void init();
  uint8_t getCurrentRotation();
  void displaySensorDetails();
  void setRange(int range);
  void updateSensorValues();
  float getX();
  float getY();
  float getZ();
  uint8_t calculateRotation();
};
