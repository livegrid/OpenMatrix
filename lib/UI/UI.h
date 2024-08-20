#pragma once

#include <functional>
#include "LittleFS.h"
#include "Arduino.h"
#include "WiFi.h"
#include "WebServer.h"
#include "StateManager.h"
#include "ArduinoJson.h"

class UI {
    public:
        typedef std::function<void(bool value)> onPowerCallback;
        typedef std::function<void(uint16_t value)> onBrightnessCallback;
        typedef std::function<void(OpenMatrixMode mode)> onModeChangeCallback;
        typedef std::function<void(Effects effect)> onEffectChangeCallback;
        typedef std::function<void(const char* path)> onImageChangeCallback;
        typedef std::function<void(const char* path)> onImagePreviewCallback;
        typedef std::function<void(const char* payload, TextSize size)> onTextChangeCallback;

        UI(WebServer* server, StateManager* stateManager);
        void begin();
        void onPower(onPowerCallback cb);
        void onBrightness(onBrightnessCallback cb);
        void onMode(onModeChangeCallback cb);
        void onEffect(onEffectChangeCallback cb);
        void onImage(onImageChangeCallback cb);
        void onText(onTextChangeCallback cb);
        ~UI();
    
    private:
        WebServer* _server;
        StateManager* _state_manager;

        onPowerCallback _on_power_cb;
        onBrightnessCallback _on_brightness_cb;
        onModeChangeCallback _on_mode_cb;
        onEffectChangeCallback _on_effect_cb;
        onImageChangeCallback _on_image_cb;
        onTextChangeCallback _on_text_cb;

    protected:
        static bool _onAPFilter(WebServer &server);
        static bool _onSTAFilter(WebServer &server);
};
