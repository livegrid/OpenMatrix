#include "StateManager.h"
#include "esp_log.h"

StateManager::StateManager() {
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

  if (!settings_only) {
    JsonObject environment = json["environment"].to<JsonObject>();

    // Temperature
    JsonObject temperature = environment["temperature"].to<JsonObject>();
    temperature["value"] = _state.environment.temperature.value;
    temperature["diff"]["type"] = _state.environment.temperature.diff.type;
    temperature["diff"]["value"] = _state.environment.temperature.diff.value;
    temperature["diff"]["inverse"] = _state.environment.temperature.diff.inverse;
    JsonArray temperature_history = temperature.createNestedArray("history_24h");
    for (int i = 0; i < 24; i++) {
      temperature_history.add(_state.environment.temperature.history_24h[i]);
    }
    // Humidity
    JsonObject humidity = environment["humidity"].to<JsonObject>();
    humidity["value"] = _state.environment.humidity.value;
    humidity["diff"]["type"] = _state.environment.humidity.diff.type;
    humidity["diff"]["value"] = _state.environment.humidity.diff.value;
    humidity["diff"]["inverse"] = _state.environment.humidity.diff.inverse;
    JsonArray humidity_history = humidity.createNestedArray("history_24h");
    for (int i = 0; i < 24; i++) {
      humidity_history.add(_state.environment.humidity.history_24h[i]);
    }
    // CO2
    JsonObject co2 = environment["co2"].to<JsonObject>();
    co2["value"] = _state.environment.co2.value;
    co2["diff"]["type"] = _state.environment.co2.diff.type;
    co2["diff"]["value"] = _state.environment.co2.diff.value;
    co2["diff"]["inverse"] = _state.environment.co2.diff.inverse;
    JsonArray co2_history = co2.createNestedArray("history_24h");
    for (int i = 0; i < 24; i++) {
      co2_history.add(_state.environment.co2.history_24h[i]);
    }
  }

  // Effects
  json["effects"]["selected"] = _state.effects.selected;

  // Image
  json["image"]["selected"] = _state.image.selected;

  // Text
  json["text"]["payload"] = _state.text.payload;
  json["text"]["size"] = _state.text.size;

  // Settings
  JsonObject settings = json["settings"].to<JsonObject>();
  // MQTT
  settings["mqtt"]["host"] = _state.settings.mqtt.host;
  settings["mqtt"]["port"] = _state.settings.mqtt.port;
  settings["mqtt"]["client_id"] = _state.settings.mqtt.client_id;
  settings["mqtt"]["username"] = _state.settings.mqtt.username;
  settings["mqtt"]["password"] = _state.settings.mqtt.password;
  settings["mqtt"]["co2_topic"] = _state.settings.mqtt.co2_topic;
  settings["mqtt"]["matrix_text_topic"] = _state.settings.mqtt.matrix_text_topic;
  settings["mqtt"]["show_text"] = _state.settings.mqtt.show_text;
  // Home Assistant
  settings["hass"]["show_text"] = _state.settings.home_assistant.show_text;
  // eDMX
  settings["edmx"]["protocol"] = _state.settings.edmx.protocol;
  settings["edmx"]["multicast"] = _state.settings.edmx.multicast;
  settings["edmx"]["start_universe"] = _state.settings.edmx.start_universe;
  settings["edmx"]["start_address"] = _state.settings.edmx.start_address;
  settings["edmx"]["mode"] = _state.settings.edmx.mode;
  settings["edmx"]["timeout"] = _state.settings.edmx.timeout;

  serializeJson(json, buffer);
  // Clear JSON
  json.clear();
}

void StateManager::save() {
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
  
  JsonDocument json;

  // Parse JSON
  DeserializationError error = deserializeJson(json, file);
  file.close();

  if (error) {
    log_e("[!] Failed to parse state.json");
    setDefaultState();
    save();
    return;
  }

  // Convert JSON to State
  _state.power = json["power"].as<bool>();
  _state.autobrightness = json["autobrightness"].as<bool>();
  _state.brightness = json["brightness"].as<uint8_t>();
  _state.mode = json["mode"].as<OpenMatrixMode>();

  log_i("Restored power: %d", _state.power);
  log_i("Restored autobrightness: %d", _state.autobrightness);
  log_i("Restored brightness: %d", _state.brightness);
  log_i("Restored mode: %d", static_cast<int>(_state.mode));

  JsonObject environment = json["environment"];
  if (environment) {
    JsonArray temp_history = environment["temperature"]["history_24h"];
    JsonArray humidity_history = environment["humidity"]["history_24h"];
    JsonArray co2_history = environment["co2"]["history_24h"];

    log_i("Restoring environment history:");
    for (int i = 0; i < 24; i++) {
      _state.environment.temperature.history_24h[i] = temp_history[i] | 0.0f;
      _state.environment.humidity.history_24h[i] = humidity_history[i] | 0;
      _state.environment.co2.history_24h[i] = co2_history[i] | 0;
      log_i("Hour %d - Temp: %.2f, Humidity: %d, CO2: %d", 
        i, 
        _state.environment.temperature.history_24h[i],
        _state.environment.humidity.history_24h[i],
        _state.environment.co2.history_24h[i]
      );
    }
  }
  else {
    log_e("[!] No environment data found in state.json");
  }

  // Effects
  _state.effects.selected = json["effects"]["selected"].as<Effects>();
  log_i("Restored selected effect: %d", static_cast<int>(_state.effects.selected));

  // Image
  _state.image.selected = json["image"]["selected"].as<const char*>();
  log_i("Restored selected image: %s", _state.image.selected);

  // Text
  _state.text.payload = json["text"]["payload"].as<const char*>();
  _state.text.size = json["text"]["size"].as<TextSize>();
  log_i("Restored text payload: %s", _state.text.payload);
  log_i("Restored text size: %d", static_cast<int>(_state.text.size));

  // Settings
  JsonObject settings = json["settings"];
  // MQTT
  _state.settings.mqtt.host = settings["mqtt"]["host"].as<const char*>();
  _state.settings.mqtt.port = settings["mqtt"]["port"].as<const char*>();
  _state.settings.mqtt.client_id = settings["mqtt"]["client_id"].as<const char*>();
  _state.settings.mqtt.username = settings["mqtt"]["username"].as<const char*>();
  _state.settings.mqtt.password = settings["mqtt"]["password"].as<const char*>();
  _state.settings.mqtt.co2_topic = settings["mqtt"]["co2_topic"].as<const char*>();
  _state.settings.mqtt.matrix_text_topic = settings["mqtt"]["matrix_text_topic"].as<const char*>();
  _state.settings.mqtt.show_text = settings["mqtt"]["show_text"].as<bool>();

  log_i("Restored MQTT settings:");
  log_i("  Host: %s", _state.settings.mqtt.host);
  log_i("  Port: %s", _state.settings.mqtt.port);
  log_i("  Client ID: %s", _state.settings.mqtt.client_id);
  log_i("  Username: %s", _state.settings.mqtt.username);
  log_i("  Password: %s", _state.settings.mqtt.password);
  log_i("  CO2 Topic: %s", _state.settings.mqtt.co2_topic);
  log_i("  Matrix Text Topic: %s", _state.settings.mqtt.matrix_text_topic);
  log_i("  Show Text: %d", _state.settings.mqtt.show_text);

  // Home Assistant
  _state.settings.home_assistant.show_text = settings["hass"]["show_text"].as<bool>();
  log_i("Restored Home Assistant show_text: %d", _state.settings.home_assistant.show_text);

  // eDMX
  _state.settings.edmx.protocol = settings["edmx"]["protocol"].as<eDmxProtocol>();
  _state.settings.edmx.multicast = settings["edmx"]["multicast"].as<bool>();
  _state.settings.edmx.start_universe = settings["edmx"]["start_universe"].as<bool>();
  _state.settings.edmx.start_address = settings["edmx"]["start_address"].as<uint16_t>();
  _state.settings.edmx.mode = settings["edmx"]["mode"].as<eDmxMode>();
  _state.settings.edmx.timeout = settings["edmx"]["timeout"].as<uint16_t>();

  log_i("Restored eDMX settings:");
  log_i("  Protocol: %d", static_cast<int>(_state.settings.edmx.protocol));
  log_i("  Multicast: %d", _state.settings.edmx.multicast);
  log_i("  Start Universe: %d", _state.settings.edmx.start_universe);
  log_i("  Start Address: %d", _state.settings.edmx.start_address);
  log_i("  Mode: %d", static_cast<int>(_state.settings.edmx.mode));
  log_i("  Timeout: %d", _state.settings.edmx.timeout);

  // Free memory
  json.clear();

  log_i("State restoration complete");
}

void StateManager::setDefaultState() {
    // Basic state
    _state.power = false;
    _state.autobrightness = true;
    _state.brightness = 100;
    _state.mode = OpenMatrixMode::AQUARIUM;

    // Environment
    _state.environment.temperature.value = 0.0f;
    _state.environment.temperature.diff = {DiffType::DISABLE, "", false};
    _state.environment.humidity.value = 0.0f;
    _state.environment.humidity.diff = {DiffType::DISABLE, "", false};
    _state.environment.co2.value = 0.0f;
    _state.environment.co2.diff = {DiffType::DISABLE, "", false};

    // Initialize history arrays with zeros
    for (int i = 0; i < 24; i++) {
        _state.environment.temperature.history_24h[i] = 0.0f;
        _state.environment.humidity.history_24h[i] = 0.0f;
        _state.environment.co2.history_24h[i] = 0.0f;
    }

    // Effects
    _state.effects.selected = Effects::SIMPLEX_NOISE;
    _state.image.selected = "Airplane.gif";
    _state.text.payload = "Hello, World!";
    _state.text.size = TextSize::MEDIUM;

    // Image
    _state.image.selected = "";

    // Text
    _state.text.payload = "";
    _state.text.size = TextSize::SMALL;

    // Settings
    // MQTT
    _state.settings.mqtt.status = ConnectionStatus::DISCONNECTED;
    _state.settings.mqtt.host = "";
    _state.settings.mqtt.port = "1883";
    _state.settings.mqtt.client_id = "livegrid";
    _state.settings.mqtt.username = "";
    _state.settings.mqtt.password = "";
    _state.settings.mqtt.co2_topic = "livegrid/co2";
    _state.settings.mqtt.matrix_text_topic = "livegrid/text";
    _state.settings.mqtt.show_text = false;

    // Home Assistant
    _state.settings.home_assistant.status = ConnectionStatus::DISCONNECTED;
    _state.settings.home_assistant.show_text = false;

    // eDMX
    _state.settings.edmx.protocol = eDmxProtocol::S_ACN;
    _state.settings.edmx.multicast = true;
    _state.settings.edmx.start_universe = true;
    _state.settings.edmx.start_address = 1;
    _state.settings.edmx.mode = eDmxMode::DMX_MODE_RGB;
    _state.settings.edmx.timeout = 5000;
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
    const TickType_t xFrequency = pdMS_TO_TICKS(SAVE_INTERVAL);
    log_i("Save task started");
    // Wait for 10 seconds before starting the periodic save task
    vTaskDelay(pdMS_TO_TICKS(SAVE_INTERVAL));

    for (;;) {   
        log_i("Saving state periodically");
        stateManager->save();  // This will now save all state data
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

StateManager::~StateManager() {
    if (_saveTaskHandle != NULL) {
        vTaskDelete(_saveTaskHandle);
    }
}