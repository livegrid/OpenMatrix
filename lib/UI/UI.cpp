#include "UI.h"
#include "interface.h"

UI::UI(WebServer* server, StateManager* stateManager) {
    _server = server;
    _state_manager = stateManager;
}

void UI::begin() {
    // For STA clients
    _server->on("/", HTTP_GET, [&]() {
        _server->sendHeader("Content-Encoding", "gzip");
        _server->send_P(200, "text/html", (const char*)OPEN_MATRIX_HTML, sizeof(OPEN_MATRIX_HTML));
    }).setFilter(_onSTAFilter);

    // For AP clients
    _server->on("/openmatrix", HTTP_GET, [&]() {
        _server->sendHeader("Content-Encoding", "gzip");
        _server->send_P(200, "text/html", (const char*)OPEN_MATRIX_HTML, sizeof(OPEN_MATRIX_HTML));
    }).setFilter(_onAPFilter);

    // OpenMatrix State
    _server->on("/openmatrix/state", HTTP_GET, [&]() {
        String state;
        _state_manager->serialize(state);
        return _server->send(200, "application/json", state);
    });

    // OpenMatrix Switch Modes
    _server->on("/openmatrix/mode", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) { 
            if (_on_mode_cb) {
                _on_mode_cb(json["mode"].as<OpenMatrixMode>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on Power Change
    _server->on("/openmatrix/power", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));

        if (err == DeserializationError::Ok) {
            if (_on_power_cb) {
                _on_power_cb(json["power"].as<bool>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on Power Change
    _server->on("/openmatrix/autobrightness", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));

        if (err == DeserializationError::Ok) {
            if (_on_autobrightness_cb) {
                _on_autobrightness_cb(json["autobrightness"].as<bool>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on Brightness Change
    _server->on("/openmatrix/brightness", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));

        if (err == DeserializationError::Ok) {
            uint8_t brightness = json["brightness"].as<uint8_t>();
            if (_on_brightness_cb) {
                _on_brightness_cb(brightness);
            }

            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on effect selection
    _server->on("/openmatrix/effect", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (_on_effect_cb) {
                _on_effect_cb(json["effect"].as<Effects>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    _server->on("/openmatrix/image", HTTP_GET, [&]() {
        JsonDocument doc;
        JsonArray json = doc.to<JsonArray>();
        // Go through LitteFS and scan all files in 'img' directory
        File root = LittleFS.open("/img", "r");

        if (!root) {
            return _server->send(200, "application/json", "[]");
        }

        // Loop over all files, adding them to the root array without (file extension)
        File file = root.openNextFile();
        while (file) {
            // Skip directories
            if (file.isDirectory()) {
                continue;
            }

            String name = file.name();
            // Remove special characters
            name.replace("(", "");
            name.replace(")", "");
            name.replace("[", "");
            name.replace("]", "");
            name.replace("{", "");
            name.replace("}", "");
            name.replace("|", "");
            name.replace(":", "");
            name.replace("?", "");
            name.replace("\"", "");
            name.replace("<", "");
            name.replace(">", "");
            name.replace("/", "");
            name.replace("\\", "");
            name.replace("*", "");
            name.replace("\r", "");
            name.replace("\n", "");

            JsonDocument image;
            image["name"] = name;
            image["size"] = file.size();
            json.add(image);

            // Hard limiter for number of images to be listed
            if (json.size() >= 30) {
                log_i("Reached limit of 30 images in 'img' directory.");
                break;
            }

            // Get next file
            file.close();
            file = root.openNextFile();
        }
        
        root.close();

        // If overflowed, delete elements from JsonArray until it doesn't overflow
        if (doc.overflowed()) {
            while (doc.overflowed() && json.size() > 0) {
                json.remove(json.size() - 1);
            }
        }

        // Serialize JSON
        String payload;
        serializeJson(doc, payload);
        doc.clear();

        return _server->send(200, "application/json", payload);
    });

    // on image selection
    _server->on("/openmatrix/image", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (imageExists(json["name"].as<const char*>())) {
                if (_on_image_cb) {
                    _on_image_cb(json["name"].as<const char*>());
                }
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on image preview
    _server->on("/openmatrix/image", HTTP_PATCH, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (imageExists(json["name"].as<const char*>())) {
                if (_on_image_preview_cb) {
                    _on_image_preview_cb(json["name"].as<const char*>());
                }
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on text change
    _server->on("/openmatrix/text", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (_on_text_cb) {
                _on_text_cb(json["payload"].as<const char*>(), json["size"].as<TextSize>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on mqtt settings
    _server->on("/openmatrix/settings/mqtt", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (_on_mqtt_settings_cb) {
                _on_mqtt_settings_cb(json["host"].as<const char*>(), json["port"].as<uint16_t>(), json["client_id"].as<const char*>(), json["username"].as<const char*>(), json["password"].as<const char*>(), json["co2_topic"].as<const char*>(), json["matrix_text_topic"].as<const char*>(), json["show_text"].as<bool>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on dmx settings
    _server->on("/openmatrix/settings/dmx", HTTP_POST, [&]() {
        JsonDocument json;
        DeserializationError err = deserializeJson(json, _server->arg("plain"));
        if (err == DeserializationError::Ok) {
            if (_on_dmx_settings_cb) {
                _on_dmx_settings_cb(json["protocol"].as<eDmxProtocol>(), json["mode"].as<eDmxMode>(), json["multicast"].as<bool>(), json["start_universe"].as<bool>(), json["start_address"].as<uint16_t>(), json["timeout"].as<uint16_t>());
            }
            return _server->send(200, "application/json", "{\"message\":\"OK\"}");
        } else {
            return _server->send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
        }
    });

    // on network reset
    _server->on("/openmatrix/network/reset", HTTP_POST, [&]() {
        if (_on_network_reset_cb) {
            _on_network_reset_cb();
        } 
        return _server->send(200, "application/json", "{\"message\":\"OK\"}");
    });
}

void UI::onPower(onPowerCallback cb) {
    _on_power_cb = cb;
}

void UI::onAutoBrightness(onAutoBrightnessCallback cb) {
    _on_autobrightness_cb = cb;
}

void UI::onBrightness(onBrightnessCallback cb) {
    _on_brightness_cb = cb;
}

void UI::onMode(onModeChangeCallback cb) {
    _on_mode_cb = cb;
}

void UI::onEffect(onEffectChangeCallback cb) {
    _on_effect_cb = cb;
}

void UI::onImage(onImageChangeCallback cb) {
    _on_image_cb = cb;
}

void UI::onImagePreview(onImagePreviewCallback cb) {
    _on_image_preview_cb = cb;
}

void UI::onText(onTextChangeCallback cb) {
    _on_text_cb = cb;
}

void UI::onMqttSettings(onMqttSettingsCallback cb) {
    _on_mqtt_settings_cb = cb;
}

void UI::onDmxSettings(onDmxSettingsCallback cb) {
    _on_dmx_settings_cb = cb;
}

void UI::onHomeAssistantSettings(onHomeAssistantSettingsCallback cb) {
    _on_home_assistant_settings_cb = cb;
}

void UI::onNetworkReset(onNetworkResetCallback cb) {
    _on_network_reset_cb = cb;
}

bool UI::_onAPFilter(WebServer &server) {
    return WiFi.AP.hasIP() && WiFi.AP.localIP() == server.client().localIP();
}

bool UI::_onSTAFilter(WebServer &server) {
    return WiFi.STA.hasIP() && WiFi.STA.localIP() == server.client().localIP();
}

bool UI::imageExists(const char* name) {
    File file = LittleFS.open("/img/" + String(name), "r");
    if (!file) {
        return false;
    }
    file.close();
    return true;
}

UI::~UI() {
}