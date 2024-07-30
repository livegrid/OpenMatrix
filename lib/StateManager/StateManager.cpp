#include "StateManager.h"

StateManager::StateManager() {
}

State* StateManager::getState() {
    return &_state;
}

void StateManager::serialize(String& buffer, bool settings_only) {
    JsonDocument json;
    
    // Convert State to JSON
    json["power"] = _state.power;
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
        // Humidity
        JsonObject humidity = environment["humidity"].to<JsonObject>();
        humidity["value"] = _state.environment.humidity.value;
        humidity["diff"]["type"] = _state.environment.humidity.diff.type;
        humidity["diff"]["value"] = _state.environment.humidity.diff.value;
        humidity["diff"]["inverse"] = _state.environment.humidity.diff.inverse;
        // CO2
        JsonObject co2 = environment["co2"].to<JsonObject>();
        co2["value"] = _state.environment.co2.value;
        co2["diff"]["type"] = _state.environment.co2.diff.type;
        co2["diff"]["value"] = _state.environment.co2.diff.value;
        co2["diff"]["inverse"] = _state.environment.co2.diff.inverse;
    }

    // Effects
    json["effects"]["selected"] = _state.effects.selected;
    
    // Image
    json["image"]["selected"] = _state.image.selected;

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
    settings["home_assistant"]["show_text"] = _state.settings.home_assistant.show_text;
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
    File file = LittleFS.open("/state.json", "w");
    if (!file) {
        Serial.println("[!] Failed to open state.json for writing");
        return;
    }

    // Serialize settings only to file
    String buffer;
    this->serialize(buffer, true);
    
    // Write to file
    file.print(buffer);

    // Close file
    file.close();
}

void StateManager::restore() {
    // Restore state from FS
    File file = LittleFS.open("/state.json", "r");
    if (file) {
        JsonDocument json;

        // Parse JSON
        DeserializationError error = deserializeJson(json, file);
        if (error) {
            Serial.println("[!] Failed to parse state.json");
            file.close();
            return;
        }

        // Convert JSON to State
        _state.power = json["power"].as<bool>();
        _state.brightness = json["brightness"].as<uint8_t>();
        _state.mode = json["mode"].as<OpenMatrixMode>();
        
        // Effects
        _state.effects.selected = json["effects"]["selected"].as<Effects>();

        // Image
        _state.image.selected = json["image"]["selected"].as<const char*>();

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
        // Home Assistant
        _state.settings.home_assistant.show_text = settings["home_assistant"]["show_text"].as<bool>();
        // eDMX
        _state.settings.edmx.protocol = settings["edmx"]["protocol"].as<eDmxProtocol>();
        _state.settings.edmx.multicast = settings["edmx"]["multicast"].as<bool>();
        _state.settings.edmx.start_universe = settings["edmx"]["start_universe"].as<bool>();
        _state.settings.edmx.start_address = settings["edmx"]["start_address"].as<uint16_t>();
        _state.settings.edmx.mode = settings["edmx"]["mode"].as<eDmxMode>();
        _state.settings.edmx.timeout = settings["edmx"]["timeout"].as<uint16_t>();

        // Free memory
        json.clear();
    }

    file.close();
}

StateManager::~StateManager() {
}
