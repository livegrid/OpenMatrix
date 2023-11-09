#include "ConfigManager.h"

ConfigManager::ConfigManager() {
}

Config& ConfigManager::getConfig() {
    return _config;
}

void ConfigManager::serialize(String& buffer) {
    JsonDocument json;
    json["wifi"]["ssid"] = _config.wifi.ssid;
    json["wifi"]["password"] = _config.wifi.password;

    serializeJson(json, buffer);
    // Clear JSON
    json.clear();
}

void ConfigManager::save() {
    File file = LittleFS.open("/state.json", "w");
    if (!file) {
        Serial.println("[!] Failed to open state.json for writing");
        return;
    }

    // Serialize config to file
    String buffer;
    this->serialize(buffer);
    
    // Write to file
    file.print(buffer);

    // Close file
    file.close();
}

void ConfigManager::reset() {
    _config.wifi.ssid = "";
    _config.wifi.password = "";
}

ConfigManager::~ConfigManager() {
    // Clear state
    _config.wifi.ssid.clear();
    _config.wifi.password.clear();
}
