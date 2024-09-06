#include "AquariumStateManager.h"

const char* AquariumStateManager::FILENAME = "/aquarium_state.json";

AquariumStateManager::AquariumStateManager() {
    if (!LittleFS.begin(true)) {
        log_e("An error occurred while mounting LittleFS");
    }
}

void AquariumStateManager::saveState(const std::vector<std::unique_ptr<Fish>>& fishArray) {
    log_i("[SAVE] Starting save operation...");

    String buffer;
    
    DynamicJsonDocument doc(16384);
    JsonArray fishesJson = doc.createNestedArray("fishes");

    for (const auto& fish : fishArray) {
        JsonObject fishJson = fishesJson.createNestedObject();
        fishJson["age"] = fish->getAge();
        fishJson["health"] = fish->getHealth();
        fishJson["bodyType"] = fish->getBodyType();
        fishJson["headType"] = fish->getHeadType();
        fishJson["tailType"] = fish->getTailType();
        fishJson["finType"] = fish->getFinType();
        fishJson["motionType"] = fish->getMotionType();
        
        JsonArray colorsJson = fishJson.createNestedArray("colors");
        for (const auto& color : fish->getColorsHSV()) {
            JsonObject colorJson = colorsJson.createNestedObject();
            colorJson["h"] = color.hue;
            colorJson["s"] = color.sat;
            colorJson["v"] = color.val;
        }
    }

    serializeJson(doc, buffer);
    
    if (buffer.length() == 0) {
        log_e("[SAVE] Failed to serialize state");
        return;
    }
    log_i("[SAVE] Serialization successful. Buffer length: %d", buffer.length());

    File file = LittleFS.open(FILENAME, "w");
    
    if (!file) {
        log_e("[SAVE] Failed to open file for writing. Aborting save operation.");
        return;
    }

    size_t bytesWritten = file.print(buffer);
    if (bytesWritten != buffer.length()) {
        log_e("[SAVE] Failed to write entire buffer to file. Bytes written: %d, Buffer length: %d", bytesWritten, buffer.length());
    }
    
    // String jsonString;
    // serializeJsonPretty(doc, jsonString);
    // log_i("[SAVE] Aquarium state:\n%s", jsonString.c_str());

    file.close();
    log_i("[SAVE] Save operation completed");
}

bool AquariumStateManager::loadState(std::vector<std::unique_ptr<Fish>>& fishArray, Matrix* matrix) {
    File file = LittleFS.open(FILENAME, "r");
    if (!file) {
        log_e("[LOAD] state.json not found. Returning False.");
        return false;
    }
    
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, file);
    
    // String jsonString;
    // serializeJsonPretty(doc, jsonString);
    // log_i("[LOAD] Aquarium state:\n%s", jsonString.c_str());

    file.close();

    if (error) {
        log_e("[LOAD] Failed to parse state.json");
        return false;
    }

    fishArray.clear();
    JsonArray fishesJson = doc["fishes"];
    for (JsonObject fishJson : fishesJson) {
        Fish::FishDefinition fishDef;
        fishDef.age = fishJson["age"];
        fishDef.health = fishJson["health"];
        fishDef.bodyType = fishJson["bodyType"].as<String>();
        fishDef.headType = fishJson["headType"].as<String>();
        fishDef.tailType = fishJson["tailType"].as<String>();
        fishDef.finType = fishJson["finType"].as<String>();
        fishDef.motionType = fishJson["motionType"].as<String>();
        
        JsonArray colorsJson = fishJson["colors"];
        for (JsonObject colorJson : colorsJson) {
            CHSV color;
            color.hue = colorJson["h"];
            color.sat = colorJson["s"];
            color.val = colorJson["v"];
            fishDef.colors.push_back(color);
        }

        fishArray.emplace_back(std::make_unique<Fish>(matrix, fishDef));
    }

    log_i("[LOAD] State loaded successfully");
    return true;
}