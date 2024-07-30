#include "AutoRotate.h"

AutoRotate::AutoRotate(Matrix* matrix) : matrix(matrix), accel(12345), sensorWorking(false), x(0), y(0), z(0), currentRotation(5) { // Initialize with an invalid rotation
}

void AutoRotate::init() {
  xTaskCreate(rotationTask, "RotationTask", 2048, this, 1, &rotationTaskHandle);
}

void AutoRotate::rotationTask(void* param) {
  AutoRotate* self = static_cast<AutoRotate*>(param);
  self->initializeSensor();
  self->runRotationTask();
}

void AutoRotate::initializeSensor() {
  if (!accel.begin()) {
    log_e("Ooops, no ADXL345 detected ... Check your wiring!");
    sensorWorking = false;
    return;
  }
  sensorWorking = true;
  accel.setRange(ADXL345_RANGE_16_G);
}

void AutoRotate::runRotationTask() {
  while (true) {
    updateSensorValues();
    currentRotation = calculateRotation();
    matrix->setRotation(currentRotation);
    // log_i("Current rotation: %d", currentRotation);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Update every 1 second
  }
}

uint8_t AutoRotate::getCurrentRotation() {
  return currentRotation;
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
  switch(range) {
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

void AutoRotate::updateSensorValues() {
  sensors_event_t event; 
  accel.getEvent(&event);
  x = event.acceleration.x;
  y = event.acceleration.y;
  z = event.acceleration.z;
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
  uint8_t newRotation = 5;  //this will be assumed invalid
  float buffer = 2.02; // Adjust buffer value as needed

  if ((x > (10-buffer)) && (x < (10+buffer)) && (y > (-buffer)) && (y < buffer)) {
    newRotation = 1;
  } else if ((x > - buffer) && (x < buffer) && (y > (-10-buffer)) && (y < (-10+buffer))) {
    newRotation = 2;
  } else if ((x > (-10-buffer)) && (x < (-10+buffer)) && (y > -buffer) && (y < buffer)) {
    newRotation = 3;
  } else if ((x > -buffer) && (x < buffer) && (y > (10-buffer)) && (y < (10+buffer))) {
    newRotation = 0;
  }
  return newRotation;
}
