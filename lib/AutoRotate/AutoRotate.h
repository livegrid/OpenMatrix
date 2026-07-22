#pragma once

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <GeneralSettings.h>
#include "Matrix.h"

class AutoRotate {
 private:
  Matrix* matrix;
  Adafruit_ADXL345_Unified accel;
  bool sensorWorking;
  float x, y, z;
  uint8_t sampleRotation;
  uint8_t stableRotation;
  uint8_t pendingRotation;
  uint8_t pendingCount;
  bool hasStableRotation;

  void tryInitSensor(const char* context);
  void updateStableRotation(uint8_t sample);

 public:
  AutoRotate(Matrix* matrix);
  void init();
  /** Latest accelerometer sample mapped to 0–3, or 5 if between orientations. */
  uint8_t getCurrentRotation() const { return sampleRotation; }
  /** Debounced orientation for aquarium / display (holds last side when sample is 5). */
  uint8_t getStableRotation() const;
  bool isSensorWorking() const { return sensorWorking; }
  void displaySensorDetails();
  void setRange(int range);
  /** When applyToMatrix is false, only updates sensor reading (aquarium draws at rotation 0). */
  void updateSensorValues(bool applyToMatrix = true);
  void applyRotationToMatrix();
  float getX();
  float getY();
  float getZ();
  uint8_t calculateRotation();
};
