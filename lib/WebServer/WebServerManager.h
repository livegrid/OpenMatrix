#pragma once

#include <WebServer.h>
#include <NetWizard.h>
#include "StateManager.h"
#include "UI.h"
#include "Matrix.h"
#include "EffectManager.h"
#include "ImageDraw.h"
#include "TaskManager.h"
#include <ElegantOTA.h>
#include "MQTTManager.h"
#include <ESPmDNS.h>
#include "Edmx.h"

class WebServerManager {
public:
    WebServerManager(Matrix* matrix, EffectManager* effectManager,
                     ImageDraw* imageDraw, StateManager* stateManager,
                     TaskManager* taskManager);
    void begin();
    void initWiFi();  // Call from ServerTask after DisplayTask has entered its render loop
    void setupAfterWiFi();  // Call from ServerTask after initWiFi()
    void handleClient();
    void setupUniqueHostname();
    NetWizard nw;
    void setupNetWizard();

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