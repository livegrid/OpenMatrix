#include "AutoRotate.h"

#include <math.h>

// Dominant axis must exceed this (m/s²) to claim a side; cross-axis must stay below cross max.
static constexpr float kTiltMinG = 7.0f;
static constexpr float kCrossAxisMaxG = 2.5f;
static constexpr uint8_t kStableSamples = 2;  // ~1 s at 500 ms sensor poll

AutoRotate::AutoRotate(Matrix* matrix)
    : matrix(matrix),
      accel(12345),
      sensorWorking(false),
      x(0),
      y(0),
      z(0),
      sampleRotation(5),
      stableRotation(0),
      pendingRotation(5),
      pendingCount(0),
      hasStableRotation(false) {}

void AutoRotate::init() {
  Wire.begin();
  Wire.setClock(400000);
  delay(10);
  tryInitSensor("setup");
}

void AutoRotate::tryInitSensor(const char* context) {
  if (accel.begin()) {
    if (!sensorWorking) {
      sensorWorking = true;
      accel.setRange(ADXL345_RANGE_16_G);
      log_i("AutoRotate: ADXL345 initialized (%s)", context);
    }
    return;
  }
  if (sensorWorking) {
    log_w("AutoRotate: ADXL345 lost on I2C (%s)", context);
    sensorWorking = false;
  } else if (strcmp(context, "setup") == 0) {
    log_e("AutoRotate: no ADXL345 on I2C (%s) — will retry every 5s", context);
  }
}

uint8_t AutoRotate::getStableRotation() const {
  if (hasStableRotation) return stableRotation;
  if (pendingRotation < 4) return pendingRotation;
  return 0;
}

void AutoRotate::updateStableRotation(uint8_t sample) {
  sampleRotation = sample;
  if (sample >= 4) {
    return;
  }
  if (sample == pendingRotation) {
    if (pendingCount < 255) pendingCount++;
  } else {
    pendingRotation = sample;
    pendingCount = 1;
  }
  if (pendingCount >= kStableSamples &&
      (!hasStableRotation || sample != stableRotation)) {
    stableRotation = sample;
    hasStableRotation = true;
  }
}

void AutoRotate::displaySensorDetails() {
  sensor_t sensor;
  accel.getSensor(&sensor);
  log_i("------------------------------------");
  log_i("Sensor:       %s", sensor.name);
  log_i("Driver Ver:   %d", sensor.version);
  log_i("Unique ID:    %d", sensor.sensor_id);
  log_i("Max Value:    %f m/s^2", sensor.max_value);
  log_i("Min Value:    %f m/s^2", sensor.min_value);
  log_i("Resolution:   %f m/s^2", sensor.resolution);
  log_i("------------------------------------");
}

void AutoRotate::setRange(int range) {
  switch (range) {
    case 0:
      accel.setRange(ADXL345_RANGE_2_G);
      break;
    case 1:
      accel.setRange(ADXL345_RANGE_4_G);
      break;
    case 2:
      accel.setRange(ADXL345_RANGE_8_G);
      break;
    case 3:
      accel.setRange(ADXL345_RANGE_16_G);
      break;
    default:
      log_e("Invalid range setting. Please use a value between 0 and 3.");
      break;
  }
}

void AutoRotate::updateSensorValues(bool applyToMatrix) {
  if (!sensorWorking) {
    static unsigned long lastRetryMs = 0;
    unsigned long now = millis();
    if (now - lastRetryMs >= 5000) {
      lastRetryMs = now;
      tryInitSensor("retry");
    }
    return;
  }

  sensors_event_t event;
  accel.getEvent(&event);
  x = event.acceleration.x;
  y = event.acceleration.y;
  z = event.acceleration.z;
  updateStableRotation(calculateRotation());

  if (!applyToMatrix) return;

  uint8_t rot = getStableRotation();
  if (rot >= 4) return;

  if (matrix->getRotation() != rot) {
    matrix->setRotation(rot);
  }
}

void AutoRotate::applyRotationToMatrix() {
  if (!sensorWorking) return;
  uint8_t rot = getStableRotation();
  if (rot >= 4) return;
  matrix->setRotation(rot);
}

float AutoRotate::getX() {
  return x;
}

float AutoRotate::getY() {
  return y;
}

float AutoRotate::getZ() {
  return z;
}

uint8_t AutoRotate::calculateRotation() {
  const float ax = fabsf(x);
  const float ay = fabsf(y);

  if (ax < kTiltMinG && ay < kTiltMinG) {
    return 5;
  }

  if (ax > ay) {
    if (ay > kCrossAxisMaxG) return 5;
#ifdef PANEL_UPCYCLED
    return x > 0 ? 1 : 3;
#else
    return x > 0 ? 3 : 1;
#endif
  }

  if (ax > kCrossAxisMaxG) return 5;
  return y > 0 ? 0 : 2;
}
