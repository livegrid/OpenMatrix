#pragma once

#include <WebServer.h>
#include <WiFi.h>
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
// #define WIFI_SSID "Pixel_9363"
// #define WIFI_PASSWORD "yellow22"
// #define WIFI_SSID "LivegridHotspot"
// #define WIFI_PASSWORD "livegrid22"
// #define WIFI_SSID "DHRUV 2244"
// #define WIFI_PASSWORD "226$Ff25"

class WebServerManager {
public:
    WebServerManager(Matrix* matrix, EffectManager* effectManager,
                     ImageDraw* imageDraw, StateManager* stateManager,
                     TaskManager* taskManager);
    void begin();
    void handleClient();
    void setupUniqueHostname();
    void connectToWiFi();
    
private:
    WebServer server;
    UI interface;
    StateManager* stateManager;
    Matrix* matrix;
    EffectManager* effectManager;
    ImageDraw* imageDraw;
    TaskManager* taskManager;
    
    void setupInterface();
    void startServer();
    void handleModeChange();
    void handleGetState();
    void handleEffectSettings();
};