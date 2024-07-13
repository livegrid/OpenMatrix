#pragma once

#include <functional>
#include "Arduino.h"
#include "WiFi.h"
#include "WebServer.h"
#include "StateManager.h"
#include "ArduinoJson.h"

class UI {
    public:
        typedef std::function<void(JsonObject& obj)> onDataCallback;

        UI(WebServer* server, StateManager* stateManager);
        void begin();
        void onModeChange(onDataCallback cb);
        void onSettingsChange(onDataCallback cb);
        ~UI();
    
    private:
        WebServer* _server;
        StateManager* _state_manager;

        onDataCallback _on_mode_change_cb;
        onDataCallback _on_settings_change_cb;

    protected:
        static bool _onAPFilter(WebServer &server);
        static bool _onSTAFilter(WebServer &server);
};
