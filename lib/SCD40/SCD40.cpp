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
  float tempFahrenheitAvg = (tempAvg * 9.0/5.0) + 32.0;  // Convert to Fahrenheit
  float humidityAvg = humiditySum / readingCount;
  float co2Avg = co2Sum / readingCount;
  
  // Update the last element with current running average
  state->environment.temperature.history_24h[23] = tempAvg;
  state->environment.temperature_fahrenheit.history_24h[23] = tempFahrenheitAvg;
  state->environment.humidity.history_24h[23] = round(humidityAvg);
  state->environment.co2.history_24h[23] = round(co2Avg);
}

void SCD40::shiftHistoryAndResetAverage(State* state) {
  
  // Shift history values
  for (int i = 0; i < 23; i++) {
    state->environment.temperature.history_24h[i] = state->environment.temperature.history_24h[i+1];
    state->environment.temperature_fahrenheit.history_24h[i] = state->environment.temperature_fahrenheit.history_24h[i+1];
    state->environment.humidity.history_24h[i] = state->environment.humidity.history_24h[i+1];
    state->environment.co2.history_24h[i] = state->environment.co2.history_24h[i+1];
  }
  
  // Reset sums and count for the new hour
  tempSum = humiditySum = co2Sum = 0;
  readingCount = 0;
}
void SCD40::runMeasurementTask() {
  const int MAX_RETRIES = 5;
  int retries = 0;
  uint16_t error;

  Wire.begin();
  
  while (retries < MAX_RETRIES) {
    log_i("Attempting to initialize SCD40 sensor (attempt %d of %d)", retries + 1, MAX_RETRIES);

    scd4x.begin(Wire);
    
    // Stop any ongoing measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
      log_e("Error stopping periodic measurement: %u", error);
      retries++;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // Perform self-test
    uint16_t sensorStatus;
    error = scd4x.performSelfTest(sensorStatus);
    if (error) {
      log_e("Error performing self-test: %u", error);
      retries++;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    log_i("Sensor status: %d", sensorStatus);

    // Start periodic measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
      log_e("Error starting periodic measurement: %u", error);
      retries++;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    // If we've reached here, initialization was successful
    log_i("SCD40 sensor initialized successfully");
    sensorAvailable = true;
    break;
  }

  if (retries == MAX_RETRIES) {
    log_e("Failed to initialize SCD40 sensor after %d attempts", MAX_RETRIES);
    sensorAvailable = false;
    vTaskDelete(NULL);  // Terminate the task
    return;
  }

  // Set automatic self-calibration
  error = scd4x.setAutomaticSelfCalibration(true);
  if (error) {
    log_w("Error setting automatic self-calibration: %u", error);
  }

  const TickType_t xFrequency = pdMS_TO_TICKS(5000);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  extern StateManager stateManager;

  // Main measurement loop
  for (;;) {
    bool isDataReady = false;
    error = scd4x.getDataReadyFlag(isDataReady);
    if (error) {
      log_e("Error checking data ready flag: %u", error);
      sensorAvailable = false;
      vTaskDelay(xFrequency);
      continue;
    }

    if (isDataReady) {
      error = scd4x.readMeasurement(co2, temperature, humidity);
      if (error) {
        log_e("Error reading measurement: %u", error);
        sensorAvailable = false;
        vTaskDelay(xFrequency);
        continue;
      }

      sensorAvailable = true;
      uint8_t humidityValue = static_cast<uint8_t>(constrain(humidity, 0.0f, 255.0f));
      
      State* state = stateManager.getState();
      state->environment.temperature.value = temperature;
      state->environment.temperature.diff.type = DiffType::DISABLE;
      state->environment.temperature_fahrenheit.value = (temperature * 9.0/5.0) + 32.0;  // Convert to Fahrenheit
      state->environment.temperature_fahrenheit.diff.type = DiffType::DISABLE;
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

      firstReadingReceived = true;
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
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

float SCD40::getTemperatureFahrenheit() {
  return (temperature * 9.0/5.0) + 32.0;
}

float SCD40::getHumidity() {
  return humidity;
}

uint16_t SCD40::getCO2() {
  return co2;
}
