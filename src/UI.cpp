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
        if (_state_manager) {
            _state_manager->serialize(state);
        } else {
            state = "{}";
        }
        _server->send(200, "application/json", state);
    });
}

void UI::onModeChange(onDataCallback cb) {
    _on_mode_change_cb = cb;
}

void UI::onSettingsChange(onDataCallback cb) {
    _on_settings_change_cb = cb;
}

bool UI::_onAPFilter(WebServer &server) {
    return WiFi.AP.hasIP() && WiFi.AP.localIP() == server.client().localIP();
}

bool UI::_onSTAFilter(WebServer &server) {
    return WiFi.STA.hasIP() && WiFi.STA.localIP() == server.client().localIP();
}

UI::~UI() {
}