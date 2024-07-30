#include "SCD40.h"

String SCD40::uint16ToHex(uint16_t value) {
  String hexString = "";
  if (value < 4096) hexString += "0";
  if (value < 256) hexString += "0";
  if (value < 16) hexString += "0";
  hexString += String(value, HEX);
  return hexString;
}

String SCD40::getSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  String serialString = "Serial: 0x";
  serialString += uint16ToHex(serial0);
  serialString += uint16ToHex(serial1);
  serialString += uint16ToHex(serial2);
  return serialString;
}

void SCD40::measurementTaskFunction(void* parameter) {
  SCD40* instance = static_cast<SCD40*>(parameter);
  instance->runMeasurementTask();
}

void SCD40::runMeasurementTask() {
  Wire.begin();
  scd4x.begin(Wire);

  if(scd4x.isConnected()) {
    log_i("SCD40 connected");
    sensorAvailable = true;  // Update sensor availability status

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
          firstReadingReceived = true;  // Set the first reading flag
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

void SCD40::init() {
  xTaskCreate(measurementTaskFunction, "MeasurementTask", 2048, this, 1, NULL);
}

bool SCD40::isFirstReadingReceived() {
  return firstReadingReceived;
}

bool SCD40::isConnected() {
  return sensorAvailable;
}

long SCD40::getTemperature() {
  return temperature;
}

long SCD40::getHumidity() {
  return humidity;
}

long SCD40::getCO2() {
  return co2;
}
