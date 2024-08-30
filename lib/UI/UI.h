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
        typedef std::function<void(const char* host, uint16_t port, const char* client_id, const char* username, const char* password, const char* co2_topic, const char* matrix_text_topic, bool show_text)> onMqttSettingsCallback;
        typedef std::function<void(eDmxProtocol protocol, eDmxMode mode, bool multicast, bool start_universe, uint16_t start_address, uint16_t timeout)> onDmxSettingsCallback;
        typedef std::function<void(bool show_text)> onHomeAssistantSettingsCallback;
        typedef std::function<void()> onNetworkResetCallback;

        UI(WebServer* server, StateManager* stateManager);
        void begin();
        void onPower(onPowerCallback cb);
        void onBrightness(onBrightnessCallback cb);
        void onMode(onModeChangeCallback cb);
        void onEffect(onEffectChangeCallback cb);
        void onImage(onImageChangeCallback cb);
        void onImagePreview(onImagePreviewCallback cb);
        void onText(onTextChangeCallback cb);
        void onMqttSettings(onMqttSettingsCallback cb);
        void onDmxSettings(onDmxSettingsCallback cb);
        void onHomeAssistantSettings(onHomeAssistantSettingsCallback cb);
        void onNetworkReset(onNetworkResetCallback cb);
        ~UI();
    
    private:
        WebServer* _server;
        StateManager* _state_manager;

        onPowerCallback _on_power_cb;
        onBrightnessCallback _on_brightness_cb;
        onModeChangeCallback _on_mode_cb;
        onEffectChangeCallback _on_effect_cb;
        onImageChangeCallback _on_image_cb;
        onImagePreviewCallback _on_image_preview_cb;
        onTextChangeCallback _on_text_cb;
        onMqttSettingsCallback _on_mqtt_settings_cb;
        onDmxSettingsCallback _on_dmx_settings_cb;
        onHomeAssistantSettingsCallback _on_home_assistant_settings_cb;
        onNetworkResetCallback _on_network_reset_cb;

        bool imageExists(const char* name);

    protected:
        static bool _onAPFilter(WebServer &server);
        static bool _onSTAFilter(WebServer &server);
};
