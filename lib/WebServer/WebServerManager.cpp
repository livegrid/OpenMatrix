#include "WebServerManager.h"


WebServerManager::WebServerManager(Matrix* matrix, EffectManager* effectManager, StateManager* stateManager)
    : matrix(matrix), effectManager(effectManager), server(80), nw(&server), interface(&server, stateManager), stateManager(stateManager) {}

void WebServerManager::begin() {
    setupNetWizard();
    setupInterface();
    startServer();
}

void WebServerManager::handleClient() {
    server.handleClient();
    ElegantOTA.loop();
    nw.loop();
}

void WebServerManager::setupNetWizard() {
    nw.setStrategy(NetWizardStrategy::NON_BLOCKING);

    nw.onConnectionStatus([this](NetWizardConnectionStatus status) {
        String status_str = "";
        switch (status) {
            case NetWizardConnectionStatus::DISCONNECTED: status_str = "Disconnected"; break;
            case NetWizardConnectionStatus::CONNECTING: status_str = "Connecting"; break;
            case NetWizardConnectionStatus::CONNECTED: status_str = "Connected"; break;
            case NetWizardConnectionStatus::CONNECTION_FAILED: status_str = "Connection Failed"; break;
            case NetWizardConnectionStatus::CONNECTION_LOST: status_str = "Connection Lost"; break;
            case NetWizardConnectionStatus::NOT_FOUND: status_str = "Not Found"; break;
            default: status_str = "Unknown";
        }
        Serial.printf("[NW] Connection status changed to: %s\n", status_str.c_str());
        if (status == NetWizardConnectionStatus::CONNECTED) {
            Serial.printf("[NW] Local IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[NW] Gateway IP: %s\n", WiFi.gatewayIP().toString().c_str());
            Serial.printf("[NW] Subnet mask: %s\n", WiFi.subnetMask().toString().c_str());
        }
    });

    nw.onPortalState([](NetWizardPortalState state) {
        String state_str = "";
        switch (state) {
            case NetWizardPortalState::IDLE: state_str = "Idle"; break;
            case NetWizardPortalState::CONNECTING_WIFI: state_str = "Connecting to WiFi"; break;
            case NetWizardPortalState::WAITING_FOR_CONNECTION: state_str = "Waiting for Connection"; break;
            case NetWizardPortalState::SUCCESS: state_str = "Success"; break;
            case NetWizardPortalState::FAILED: state_str = "Failed"; break;
            case NetWizardPortalState::TIMEOUT: state_str = "Timeout"; break;
            default: state_str = "Unknown";
        }
        Serial.printf("[NW] Portal state changed to: %s\n", state_str.c_str());
    });

    Serial.println("[*] Starting NetWizard");
    nw.autoConnect("LiveGrid", "");
}

void WebServerManager::setupInterface() {
    interface.begin();
    interface.onPower([this](bool state) {
        stateManager->getState()->power = state;
        stateManager->save();
        // TODO: Change matrix power state
        Serial.printf("Power state changed to: %s\n", state ? "ON" : "OFF");
    });

    interface.onBrightness([this](uint8_t value) {
        stateManager->getState()->brightness = value;
        stateManager->save();
        matrix->setBrightness(value);
        Serial.printf("Brightness changed to: %d\n", value);
    });

    interface.onMode([this](OpenMatrixMode mode) {
        stateManager->getState()->mode = mode;
        stateManager->save();
        // TODO: Change matrix mode
        Serial.printf("Mode changed to: %d\n", static_cast<int>(mode));
    });

    interface.onEffect([this](Effects effect) {
        if(effectManager->getCurrentEffect() != (effect-1) && (effect-1) <= effectManager->getEffectCount()) {
          effectManager->setEffect(effect-1);
          stateManager->getState()->effects.selected = effect;
          stateManager->save();
          Serial.printf("Effect changed to: %d\n", static_cast<int>(effect));
        }
    });
}

void WebServerManager::startServer() {
    Serial.println("[*] Attaching ElegantOTA");
    ElegantOTA.begin(&server);

    server.begin();

    if (nw.isConfigured()) {
        Serial.println("OpenMatrix is configured!");
    } else {
        Serial.println("OpenMatrix is not configured yet! Please connect to LiveGrid AP and setup your device.");
    }
}