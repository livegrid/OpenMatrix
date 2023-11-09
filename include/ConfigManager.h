#pragma once

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "ArduinoJson.h"

struct Config {
    struct {
        String ssid;
        String password;
    } wifi;
};

class ConfigManager {
    public:
        ConfigManager();
        Config& getConfig();
        void serialize(String& buffer);
        void save();
        void reset();
        ~ConfigManager();
    private:
        Config _config;
};
