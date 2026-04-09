#pragma once

#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>  // For esp_wifi_set_ps() and low-level WiFi control
#include <nvs_flash.h> // For NVS initialization (required before WiFi on ESP32-S3/IDF5)
#include "StateManager.h"
#include "UI.h"
#include "Matrix.h"
#include "EffectManager.h"
#include "ImageDraw.h"
#include "TaskManager.h"
// #include <ElegantOTA.h>
#include "MQTTManager.h"
#include <ESPmDNS.h>
#include "Edmx.h"

// WiFi credentials
#define WIFI_SSID "Tardigrade"
#define WIFI_PASSWORD "chocolate-milk"
// #define WIFI_SSID "Pixel6"
// #define WIFI_PASSWORD "yellow22"
// #define WIFI_SSID "LivegridHotspot"
// #define WIFI_PASSWORD "livegrid22"
// #define WIFI_SSID "Hone Wifi 2.4Ghz"
// #define WIFI_PASSWORD "Findyouredge"
// #define WIFI_SSID "IRVINA_4G_EXT"
// #define WIFI_PASSWORD "raj230661"

class WebServerManager {
public:
    WebServerManager(Matrix* matrix, EffectManager* effectManager,
                     ImageDraw* imageDraw, StateManager* stateManager,
                     TaskManager* taskManager);
    void begin();
    void handleClient();
    void setupUniqueHostname();
    void connectToWiFi();
    void setupInterface();
    void startServer();
    
    // WiFi event handler for diagnostics (public for callback access)
    void onWiFiEvent(WiFiEvent_t event);
    
private:
    WebServer server;
    UI interface;
    StateManager* stateManager;
    Matrix* matrix;
    EffectManager* effectManager;
    ImageDraw* imageDraw;
    TaskManager* taskManager;
    
    void handleModeChange();
    void handleGetState();
    void handleEffectSettings();
};