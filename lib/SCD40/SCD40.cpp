#include "SCD40.h"
#include "SCD40Settings.h"

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

void SCD40::updateRunningAverage(State* state) {
  float tempAvg = tempSum / readingCount;
  float humidityAvg = humiditySum / readingCount;
  float co2Avg = co2Sum / readingCount;
  
  // Update the last element with current running average
  state->environment.temperature.history_24h[23] = tempAvg;
  state->environment.humidity.history_24h[23] = round(humidityAvg);
  state->environment.co2.history_24h[23] = round(co2Avg);
}

void SCD40::shiftHistoryAndResetAverage(State* state) {
  
  // Shift history values
  for (int i = 0; i < 23; i++) {
    state->environment.temperature.history_24h[i] = state->environment.temperature.history_24h[i+1];
    state->environment.humidity.history_24h[i] = state->environment.humidity.history_24h[i+1];
    state->environment.co2.history_24h[i] = state->environment.co2.history_24h[i+1];
  }
  
  // Reset sums and count for the new hour
  tempSum = humiditySum = co2Sum = 0;
  readingCount = 0;
}

void SCD40::runMeasurementTask() {
  Wire.begin();
  uint16_t error;
  char errorMessage[256];
  scd4x.begin(Wire);

  error = scd4x.stopPeriodicMeasurement();
  if (error) {
      log_e("Error trying to execute stopPeriodicMeasurement(): ");
      errorToString(error, errorMessage, 256);
      log_e("%s", errorMessage);
  }

  uint16_t sensorStatus;
  error = scd4x.performSelfTest(sensorStatus);
  log_i("Sensor status: %d", sensorStatus);
  if (error) {
    log_e("Error trying to execute performSelfTest(): ");
    errorToString(error, errorMessage, 256);
    log_e("%s", errorMessage);
  }

  error = scd4x.startPeriodicMeasurement();
  if(error) {
    log_e("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    log_e("%s", errorMessage);
  }
  else {
    log_i("SCD40 connected");
    sensorAvailable = true;  // Update sensor availability status

    error = scd4x.setAutomaticSelfCalibration(true);
    if (error) {
      log_e("Error trying to execute setAutomaticSelfCalibration(): ");
      errorToString(error, errorMessage, 256);
      log_e("%s", errorMessage);
    }

    error = scd4x.startPeriodicMeasurement();

    const TickType_t xFrequency = pdMS_TO_TICKS(5000);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    // char tempBuffer[4];
    extern StateManager stateManager; 

    for (;;) {
    bool isDataReady = false;
      error = scd4x.getDataReadyFlag(isDataReady);
      if (isDataReady) {
        error = scd4x.readMeasurement(co2, temperature, humidity);
        if(error) {
          log_e("Error trying to execute readMeasurement(): ");
          errorToString(error, errorMessage, 256);
          log_e("%s", errorMessage);
          log_e("SCD40 not connected");
          sensorAvailable = false;
          vTaskDelete(NULL);  // Terminate this task as the sensor is not connected
        }
        else {
          uint8_t humidityValue = static_cast<uint8_t>(constrain(humidity, 0.0f, 255.0f));
          State* state = stateManager.getState();
          state->environment.temperature.value = temperature;
          state->environment.temperature.diff.type = DiffType::DISABLE;
          state->environment.humidity.value = humidityValue;
          state->environment.humidity.diff.type = DiffType::DISABLE;
          state->environment.co2.value = co2;
          state->environment.co2.diff.type = DiffType::DISABLE;

          // Accumulate readings
          tempSum += temperature;
          humiditySum += humidityValue;
          co2Sum += co2;
          readingCount++;

          // Update running average
          updateRunningAverage(state);

          // Check if we have collected an hour's worth of readings
          if (readingCount >= READINGS_PER_HOUR) {
            shiftHistoryAndResetAverage(state);
          }

          firstReadingReceived = true;  // Set the first reading flag
        }
        sensorAvailable = true;
      }
      vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
  }
}

void SCD40::init() {
  xTaskCreatePinnedToCore(measurementTaskFunction, "MeasurementTask", 2048, this, 0, &measurementTaskHandle, 0);
}

bool SCD40::isFirstReadingReceived() {
  return firstReadingReceived;
}

bool SCD40::isConnected() {
  return sensorAvailable;
}

float SCD40::getTemperature() {
  return temperature;
}

float SCD40::getHumidity() {
  return humidity;
}

uint16_t SCD40::getCO2() {
  return co2;
}
