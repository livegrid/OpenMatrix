#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "Fish.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class AquariumStateManager {
private:
    static const char* FILENAME;
    static const uint32_t SAVE_INTERVAL = 600000; // 10 minutes in milliseconds

public:
    AquariumStateManager();
    void saveState(const std::vector<std::unique_ptr<Fish>>& fishArray);
    bool loadState(std::vector<std::unique_ptr<Fish>>& fishArray, Matrix* matrix);
};