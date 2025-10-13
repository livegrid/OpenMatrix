#include "StateManager.h"
#include "esp_log.h"

StateManager::StateManager(unsigned long saveIntervalinMinutes) {
    SAVE_INTERVAL = saveIntervalinMinutes * 60000;
}

State* StateManager::getState() {
  return &_state;
}

void StateManager::serialize(String& buffer, bool settings_only) {
  JsonDocument json;

  // Convert State to JSON
  json["power"] = _state.power;
  json["autobrightness"] = _state.autobrightness;
  json["brightness"] = _state.brightness;
  json["mode"] = _state.mode;
  json["firstBoot"] = _state.firstBoot;
  json["temperatureUnit"] = _state.temperatureUnit;

  if (!settings_only) {
    JsonObject environment = json["environment"].to<JsonObject>();

    // Temperature (Celsius)
    JsonObject temperature = environment["temperature"].to<JsonObject>();
    temperature["value"] = _state.environment.temperature.value;
    temperature["diff"]["type"] = _state.environment.temperature.diff.type;
    temperature["diff"]["value"] = _state.environment.temperature.diff.value;
    temperature["diff"]["inverse"] = _state.environment.temperature.diff.inverse;
    JsonArray temperature_history = temperature["history_24h"].to<JsonArray>();
    for (int i = 0; i < 24; i++) {
      temperature_history.add(_state.environment.temperature.history_24h[i]);
    }

    // Temperature (Fahrenheit)
    JsonObject temperature_f = environment["temperature_fahrenheit"].to<JsonObject>();
    temperature_f["value"] = _state.environment.temperature_fahrenheit.value;
    temperature_f["diff"]["type"] = _state.environment.temperature_fahrenheit.diff.type;
    temperature_f["diff"]["value"] = _state.environment.temperature_fahrenheit.diff.value;
    temperature_f["diff"]["inverse"] = _state.environment.temperature_fahrenheit.diff.inverse;
    JsonArray temperature_f_history = temperature_f["history_24h"].to<JsonArray>();
    for (int i = 0; i < 24; i++) {
      temperature_f_history.add(_state.environment.temperature_fahrenheit.history_24h[i]);
    }

    // Humidity
    JsonObject humidity = environment["humidity"].to<JsonObject>();
    humidity["value"] = _state.environment.humidity.value;
    humidity["diff"]["type"] = _state.environment.humidity.diff.type;
    humidity["diff"]["value"] = _state.environment.humidity.diff.value;
    humidity["diff"]["inverse"] = _state.environment.humidity.diff.inverse;
    JsonArray humidity_history = humidity["history_24h"].to<JsonArray>();
    for (int i = 0; i < 24; i++) {
      humidity_history.add(_state.environment.humidity.history_24h[i]);
    }

    // CO2
    JsonObject co2 = environment["co2"].to<JsonObject>();
    co2["value"] = _state.environment.co2.value;
    co2["diff"]["type"] = _state.environment.co2.diff.type;
    co2["diff"]["value"] = _state.environment.co2.diff.value;
    co2["diff"]["inverse"] = _state.environment.co2.diff.inverse;
    JsonArray co2_history = co2["history_24h"].to<JsonArray>();
    for (int i = 0; i < 24; i++) {
      co2_history.add(_state.environment.co2.history_24h[i]);
    }
  }

  // Effects
  json["effects"]["selected"] = _state.effects.selected;

  // Image
  json["image"]["selected"] = _state.image.selected;
  json["image"]["width"] = _state.image.width;
  json["image"]["height"] = _state.image.height;

  // Text
  json["text"]["payload"] = _state.text.payload;
  json["text"]["size"] = _state.text.size;

  // Settings
  JsonObject settings = json["settings"].to<JsonObject>();
  // Calibration
  settings["calibration"]["temperatureOffsetC"] = _state.settings.calibration.temperatureOffsetC;
  settings["calibration"]["humidityOffsetPct"] = _state.settings.calibration.humidityOffsetPct;
  settings["calibration"]["co2OffsetPpm"] = _state.settings.calibration.co2OffsetPpm;
  // MQTT
  settings["mqtt"]["status"] = _state.settings.mqtt.status;
  settings["mqtt"]["host"] = _state.settings.mqtt.host;
  settings["mqtt"]["port"] = _state.settings.mqtt.port;
  settings["mqtt"]["client_id"] = _state.settings.mqtt.client_id;
  settings["mqtt"]["username"] = _state.settings.mqtt.username;
  settings["mqtt"]["password"] = _state.settings.mqtt.password;
  settings["mqtt"]["co2_topic"] = _state.settings.mqtt.co2_topic;
  settings["mqtt"]["matrix_text_topic"] = _state.settings.mqtt.matrix_text_topic;
  settings["mqtt"]["show_text"] = _state.settings.mqtt.show_text;
  // Home Assistant
  settings["hass"]["status"] = _state.settings.home_assistant.status;
  settings["hass"]["show_text"] = _state.settings.home_assistant.show_text;
  // eDMX
  settings["edmx"]["protocol"] = _state.settings.edmx.protocol;
  settings["edmx"]["multicast"] = _state.settings.edmx.multicast;
  settings["edmx"]["start_universe"] = _state.settings.edmx.start_universe;
  settings["edmx"]["start_address"] = _state.settings.edmx.start_address;
  settings["edmx"]["mode"] = _state.settings.edmx.mode;
  settings["edmx"]["timeout"] = _state.settings.edmx.timeout;

  // Scheduler
  settings["scheduler"]["enableDarkAutoPower"] = _state.settings.scheduler.enableDarkAutoPower;
  settings["scheduler"]["darkThresholdLux"] = _state.settings.scheduler.darkThresholdLux;
  settings["scheduler"]["darkHysteresisLux"] = _state.settings.scheduler.darkHysteresisLux;
  settings["scheduler"]["darkStabilitySeconds"] = _state.settings.scheduler.darkStabilitySeconds;

  serializeJson(json, buffer);
  // Clear JSON
  json.clear();
}

void StateManager::save() {
  stateChanged = true;
}

void StateManager::saveState() {
  stateChanged = false;
  log_i("[SAVE] Starting save operation...");

  String buffer;
  this->serialize(buffer, false);
  
  if (buffer.length() == 0) {
    log_e("[SAVE] Failed to serialize state");
    return;
  }
  log_i("[SAVE] Serialization successful. Buffer length: %d", buffer.length());

  File file = LittleFS.open("/state.json", "w");
  
  if (!file) {
    log_e("[SAVE] Failed to open file for writing. Aborting save operation.");
    return;
  }

  size_t bytesWritten = file.print(buffer);
  if (bytesWritten != buffer.length()) {
    log_e("[SAVE] Failed to write entire buffer to file. Bytes written: %d, Buffer length: %d", bytesWritten, buffer.length());
  }

  file.close();
}

void StateManager::restore() {
  // Restore state from FS
  File file = LittleFS.open("/state.json", "r");
  if (!file) {
    log_e("[!] state.json not found. Creating default state.");
    setDefaultState();
    save();
    return;
  }

  setDefaultState();
  
  JsonDocument json;

  // Parse JSON
  DeserializationError error = deserializeJson(json, file);
  file.close();

  if (error) {
    log_e("[!] Failed to parse state.json");
    save();
    return;
  }

  _state.power = json["power"] | DEFAULT_POWER;
  _state.autobrightness = json["autobrightness"] | DEFAULT_AUTOBRIGHTNESS;
  _state.brightness = json["brightness"] | DEFAULT_BRIGHTNESS;
  _state.mode = json["mode"] | DEFAULT_MODE;
  _state.firstBoot = json["firstBoot"] | DEFAULT_FIRST_BOOT;
  _state.temperatureUnit = json["temperatureUnit"] | DEFAULT_TEMPERATURE_UNIT;

  log_i("Restored power: %d", _state.power);
  log_i("Restored autobrightness: %d", _state.autobrightness);
  log_i("Restored brightness: %d", _state.brightness);
  log_i("Restored mode: %d", static_cast<int>(_state.mode));
  log_i("Restored firstBoot: %d", _state.firstBoot);
  log_i("Restored temperatureUnit: %d", static_cast<int>(_state.temperatureUnit));

  // Environment
  JsonObject environment = json["environment"];
  if (environment) {
    JsonArray temp_history = environment["temperature"]["history_24h"];
    JsonArray temp_f_history = environment["temperature_fahrenheit"]["history_24h"];
    JsonArray humidity_history = environment["humidity"]["history_24h"];
    JsonArray co2_history = environment["co2"]["history_24h"];

    log_i("Restoring environment history:");
    for (int i = 0; i < 24; i++) {
      _state.environment.temperature.history_24h[i] = temp_history[i] | DEFAULT_TEMPERATURE_VALUE;
      _state.environment.temperature_fahrenheit.history_24h[i] = temp_f_history[i] | (DEFAULT_TEMPERATURE_VALUE * 9.0/5.0 + 32.0);
      _state.environment.humidity.history_24h[i] = humidity_history[i] | DEFAULT_HUMIDITY_VALUE;
      _state.environment.co2.history_24h[i] = co2_history[i] | DEFAULT_CO2_VALUE;
      log_i("Hour %d - Temp C: %.2f, Temp F: %.2f, Humidity: %d, CO2: %d", 
        i, 
        _state.environment.temperature.history_24h[i],
        _state.environment.temperature_fahrenheit.history_24h[i],
        _state.environment.humidity.history_24h[i],
        _state.environment.co2.history_24h[i]
      );
    }
  }

  // Effects
  _state.effects.selected = json["effects"]["selected"] | DEFAULT_EFFECTS_SELECTED;
  log_i("Restored selected effect: %d", static_cast<int>(_state.effects.selected));

  // Image
  _state.image.selected = json["image"]["selected"] | DEFAULT_IMAGE_SELECTED;
  _state.image.width = json["image"]["width"] | _state.image.width;  // Use current default from setDefaultState
  _state.image.height = json["image"]["height"] | _state.image.height;  // Use current default from setDefaultState
  log_i("Restored selected image: %s", _state.image.selected);

  // Text
  _state.text.payload = json["text"]["payload"] | DEFAULT_TEXT_PAYLOAD;
  _state.text.size = json["text"]["size"] | DEFAULT_TEXT_SIZE;
  log_i("Restored text payload: %s", _state.text.payload);
  log_i("Restored text size: %d", static_cast<int>(_state.text.size));

  // Settings
  JsonObject settings = json["settings"];
  // MQTT
  _state.settings.mqtt.status = ConnectionStatus::DISCONNECTED;  // Always start disconnected
  _state.settings.mqtt.host = settings["mqtt"]["host"] | DEFAULT_MQTT_HOST;
  _state.settings.mqtt.port = settings["mqtt"]["port"] | DEFAULT_MQTT_PORT;
  _state.settings.mqtt.client_id = settings["mqtt"]["client_id"] | DEFAULT_MQTT_CLIENT_ID;
  _state.settings.mqtt.username = settings["mqtt"]["username"] | DEFAULT_MQTT_USERNAME;
  _state.settings.mqtt.password = settings["mqtt"]["password"] | DEFAULT_MQTT_PASSWORD;
  _state.settings.mqtt.co2_topic = settings["mqtt"]["co2_topic"] | DEFAULT_MQTT_CO2_TOPIC;
  _state.settings.mqtt.matrix_text_topic = settings["mqtt"]["matrix_text_topic"] | DEFAULT_MQTT_MATRIX_TEXT_TOPIC;
  _state.settings.mqtt.show_text = settings["mqtt"]["show_text"] | DEFAULT_MQTT_SHOW_TEXT;

  // Home Assistant
  _state.settings.home_assistant.status = ConnectionStatus::DISCONNECTED;  // Always start disconnected
  _state.settings.home_assistant.show_text = settings["hass"]["show_text"] | DEFAULT_HA_SHOW_TEXT;

  // eDMX
  _state.settings.edmx.protocol = settings["edmx"]["protocol"] | DEFAULT_EDMX_PROTOCOL;
  _state.settings.edmx.multicast = settings["edmx"]["multicast"] | DEFAULT_EDMX_MULTICAST;
  _state.settings.edmx.start_universe = settings["edmx"]["start_universe"] | DEFAULT_EDMX_START_UNIVERSE;
  _state.settings.edmx.start_address = settings["edmx"]["start_address"] | DEFAULT_EDMX_START_ADDRESS;
  _state.settings.edmx.mode = settings["edmx"]["mode"] | DEFAULT_EDMX_MODE;
  _state.settings.edmx.timeout = settings["edmx"]["timeout"] | DEFAULT_EDMX_TIMEOUT;

  // Calibration
  _state.settings.calibration.temperatureOffsetC = settings["calibration"]["temperatureOffsetC"] | DEFAULT_CAL_TEMP_OFFSET_C;
  _state.settings.calibration.humidityOffsetPct = settings["calibration"]["humidityOffsetPct"] | DEFAULT_CAL_HUMIDITY_OFFSET_PCT;
  _state.settings.calibration.co2OffsetPpm = settings["calibration"]["co2OffsetPpm"] | DEFAULT_CAL_CO2_OFFSET_PPM;

  // Scheduler
  _state.settings.scheduler.enableDarkAutoPower = settings["scheduler"]["enableDarkAutoPower"] | DEFAULT_SCHED_ENABLE_DARK;
  _state.settings.scheduler.darkThresholdLux = settings["scheduler"]["darkThresholdLux"] | DEFAULT_SCHED_DARK_THRESHOLD_LUX;
  _state.settings.scheduler.darkHysteresisLux = settings["scheduler"]["darkHysteresisLux"] | DEFAULT_SCHED_DARK_HYSTERESIS_LUX;
  _state.settings.scheduler.darkStabilitySeconds = settings["scheduler"]["darkStabilitySeconds"] | DEFAULT_SCHED_DARK_STABILITY_SECONDS;

  // Free memory
  json.clear();

  log_i("State restoration complete");
}

void StateManager::setDefaultState() {
    // Basic state
    _state.power = DEFAULT_POWER;
    _state.autobrightness = DEFAULT_AUTOBRIGHTNESS;
    _state.brightness = DEFAULT_BRIGHTNESS;
    _state.mode = DEFAULT_MODE;
    _state.firstBoot = DEFAULT_FIRST_BOOT;
    _state.temperatureUnit = DEFAULT_TEMPERATURE_UNIT;

    // Environment
    _state.environment.temperature.value = DEFAULT_TEMPERATURE_VALUE;
    _state.environment.temperature.diff = {DiffType::DISABLE, "", false};
    _state.environment.temperature_fahrenheit.value = DEFAULT_TEMPERATURE_VALUE * 9.0/5.0 + 32.0;
    _state.environment.temperature_fahrenheit.diff = {DiffType::DISABLE, "", false};
    _state.environment.humidity.value = DEFAULT_HUMIDITY_VALUE;
    _state.environment.humidity.diff = {DiffType::DISABLE, "", false};
    _state.environment.co2.value = DEFAULT_CO2_VALUE;
    _state.environment.co2.diff = {DiffType::DISABLE, "", false};

    // Initialize history arrays with zeros
    for (int i = 0; i < 24; i++) {
        _state.environment.temperature.history_24h[i] = DEFAULT_TEMPERATURE_VALUE;
        _state.environment.temperature_fahrenheit.history_24h[i] = DEFAULT_TEMPERATURE_VALUE * 9.0/5.0 + 32.0;
        _state.environment.humidity.history_24h[i] = DEFAULT_HUMIDITY_VALUE;
        _state.environment.co2.history_24h[i] = DEFAULT_CO2_VALUE;
    }

    // Effects
    _state.effects.selected = DEFAULT_EFFECTS_SELECTED;
    _state.image.selected = DEFAULT_IMAGE_SELECTED;
    #ifdef PANEL_UPCYCLED
    _state.image.width = 78;
    _state.image.height = 78;
    #else
    _state.image.width = 64;
    _state.image.height = 64;
    #endif
    _state.text.payload = DEFAULT_TEXT_PAYLOAD;
    _state.text.size = DEFAULT_TEXT_SIZE;

    // Settings
    // MQTT
    _state.settings.mqtt.status = DEFAULT_MQTT_STATUS;
    _state.settings.mqtt.host = DEFAULT_MQTT_HOST;
    _state.settings.mqtt.port = DEFAULT_MQTT_PORT;
    _state.settings.mqtt.client_id = DEFAULT_MQTT_CLIENT_ID;
    _state.settings.mqtt.username = DEFAULT_MQTT_USERNAME;
    _state.settings.mqtt.password = DEFAULT_MQTT_PASSWORD;
    _state.settings.mqtt.co2_topic = DEFAULT_MQTT_CO2_TOPIC;
    _state.settings.mqtt.matrix_text_topic = DEFAULT_MQTT_MATRIX_TEXT_TOPIC;
    _state.settings.mqtt.show_text = DEFAULT_MQTT_SHOW_TEXT;

    // Home Assistant
    _state.settings.home_assistant.status = DEFAULT_HA_STATUS;
    _state.settings.home_assistant.show_text = DEFAULT_HA_SHOW_TEXT;

    // eDMX
    _state.settings.edmx.protocol = DEFAULT_EDMX_PROTOCOL;
    _state.settings.edmx.multicast = DEFAULT_EDMX_MULTICAST;
    _state.settings.edmx.start_universe = DEFAULT_EDMX_START_UNIVERSE;
    _state.settings.edmx.start_address = DEFAULT_EDMX_START_ADDRESS;
    _state.settings.edmx.mode = DEFAULT_EDMX_MODE;
    _state.settings.edmx.timeout = DEFAULT_EDMX_TIMEOUT;

    // Calibration
    _state.settings.calibration.temperatureOffsetC = DEFAULT_CAL_TEMP_OFFSET_C;
    _state.settings.calibration.humidityOffsetPct = DEFAULT_CAL_HUMIDITY_OFFSET_PCT;
    _state.settings.calibration.co2OffsetPpm = DEFAULT_CAL_CO2_OFFSET_PPM;

    // Scheduler
    _state.settings.scheduler.enableDarkAutoPower = DEFAULT_SCHED_ENABLE_DARK;
    _state.settings.scheduler.darkThresholdLux = DEFAULT_SCHED_DARK_THRESHOLD_LUX;
    _state.settings.scheduler.darkHysteresisLux = DEFAULT_SCHED_DARK_HYSTERESIS_LUX;
    _state.settings.scheduler.darkStabilitySeconds = DEFAULT_SCHED_DARK_STABILITY_SECONDS;
}

void StateManager::startPeriodicSave() {
  log_i("Starting periodic save task");
  BaseType_t result = xTaskCreate(
    this->saveTask,
    "SaveTask",
    4096,
    this,
    1,
    &_saveTaskHandle
  );
  if (result == pdPASS) {
    log_i("Periodic save task created successfully");
  } else {
    log_e("Failed to create periodic save task");
  }
}

void StateManager::saveTask(void* parameter) {
    StateManager* stateManager = static_cast<StateManager*>(parameter);
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(stateManager->SAVE_INTERVAL);
    log_i("Save task started");
    // Wait for 10 seconds before starting the periodic save task
    vTaskDelay(pdMS_TO_TICKS(10000));

    for (;;) {   
        if (stateManager->stateChanged) {
            log_i("State changed, saving immediately");
            stateManager->saveState();
            xLastWakeTime = xTaskGetTickCount();
        } else if (xTaskGetTickCount() - xLastWakeTime >= xFrequency) {
            log_i("Saving state periodically");
            stateManager->saveState();
            xLastWakeTime = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Check every second
    }
}


StateManager::~StateManager() {
    if (_saveTaskHandle != NULL) {
        vTaskDelete(_saveTaskHandle);
    }
}