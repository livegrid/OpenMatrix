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
        OpenMatrixMode mode;

        if (err == DeserializationError::Ok) { 
            if (strncmp(json["mode"].as<const char*>(), "aquarium", 8) == 0) {
                mode = OpenMatrixMode::AQUARIUM;
            } else if (strncmp(json["mode"].as<const char*>(), "effect", 6) == 0) {
                mode = OpenMatrixMode::EFFECT;
            } else if (strncmp(json["mode"].as<const char*>(), "image", 5) == 0) {
                mode = OpenMatrixMode::IMAGE;
            } else if (strncmp(json["mode"].as<const char*>(), "text", 4) == 0) {
                mode = OpenMatrixMode::TEXT;
            } else {
                mode = OpenMatrixMode::AQUARIUM;
            }

            if (_on_mode_cb) {
                _on_mode_cb(mode);
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
}

void UI::onPower(onPowerCallback cb) {
    _on_power_cb = cb;
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

bool UI::_onAPFilter(WebServer &server) {
    return WiFi.AP.hasIP() && WiFi.AP.localIP() == server.client().localIP();
}

bool UI::_onSTAFilter(WebServer &server) {
    return WiFi.STA.hasIP() && WiFi.STA.localIP() == server.client().localIP();
}

UI::~UI() {
}