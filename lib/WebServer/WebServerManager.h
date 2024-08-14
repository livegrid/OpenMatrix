#pragma once

#include <WebServer.h>
#include <NetWizard.h>
#include "StateManager.h"
#include "UI.h"
#include "Matrix.h"
#include "EffectManager.h"
#include <ElegantOTA.h>

class WebServerManager {
public:
    WebServerManager(Matrix* matrix, EffectManager* effectManager, StateManager* stateManager);
    void begin();
    void handleClient();

private:
    WebServer server;
    NetWizard nw;
    UI interface;
    StateManager* stateManager;
    Matrix* matrix;
    EffectManager* effectManager;

    void setupNetWizard();
    void setupInterface();
    void startServer();
};