#include <WebServer.h>
#include <NetWizard.h>
#include <ElegantOTA.h>
#include "StateManager.h"
#include "UI.h"
#include "Matrix.h"
#include "Effects/EffectManager.h"

#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 0

StateManager stateManager;

WebServer server(80);
NetWizard NW(&server);
UI ui(&server, &stateManager);

Matrix matrix;
EffectManager effectManager(&matrix);

void setup(void) {
  Serial.begin(115200);
  Serial.println("");

  Serial.println(R""""(
 _     _            ____      _     _ 
| |   (_)_   _____ / ___|_ __(_) __| |
| |   | \ \ / / _ \ |  _| '__| |/ _` |
| |___| |\ V /  __/ |_| | |  | | (_| |
|_____|_| \_/ \___|\____|_|  |_|\__,_|

)"""");
  Serial.println("----------------------------");
  Serial.printf("Firmware Version: %u.%u.%u\n", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
  Serial.println();

  // Start LittleFS
  LittleFS.begin();

  // Restore State
  stateManager.restore();

  // NetWizard configured to NON_BLOCKING mode
  NW.setStrategy(NetWizardStrategy::NON_BLOCKING);

  // Listen for connection status changes
  NW.onConnectionStatus([](NetWizardConnectionStatus status) {
    String status_str = "";

    switch (status) {
      case NetWizardConnectionStatus::DISCONNECTED:
        status_str = "Disconnected";
        break;
      case NetWizardConnectionStatus::CONNECTING:
        status_str = "Connecting";
        break;
      case NetWizardConnectionStatus::CONNECTED:
        status_str = "Connected";
        break;
      case NetWizardConnectionStatus::CONNECTION_FAILED:
        status_str = "Connection Failed";
        break;
      case NetWizardConnectionStatus::CONNECTION_LOST:
        status_str = "Connection Lost";
        break;
      case NetWizardConnectionStatus::NOT_FOUND:
        status_str = "Not Found";
        break;
      default:
        status_str = "Unknown";
    }

    Serial.printf("[NW] Connection status changed to: %s\n", status_str.c_str());
    if (status == NetWizardConnectionStatus::CONNECTED) {
      // Local IP
      Serial.printf("[NW] Local IP: %s\n", NW.localIP().toString().c_str());
      // Gateway IP
      Serial.printf("[NW] Gateway IP: %s\n", NW.gatewayIP().toString().c_str());
      // Subnet mask
      Serial.printf("[NW] Subnet mask: %s\n", NW.subnetMask().toString().c_str());
    }
  });

  // Listen for portal state changes
  NW.onPortalState([](NetWizardPortalState state) {
    String state_str = "";

    switch (state) {
      case NetWizardPortalState::IDLE:
        state_str = "Idle";
        break;
      case NetWizardPortalState::CONNECTING_WIFI:
        state_str = "Connecting to WiFi";
        break;
      case NetWizardPortalState::WAITING_FOR_CONNECTION:
        state_str = "Waiting for Connection";
        break;
      case NetWizardPortalState::SUCCESS:
        state_str = "Success";
        break;
      case NetWizardPortalState::FAILED:
        state_str = "Failed";
        break;
      case NetWizardPortalState::TIMEOUT:
        state_str = "Timeout";
        break;
      default:
        state_str = "Unknown";
    }

    Serial.printf("[NW] Portal state changed to: %s\n", state_str.c_str());
  });

  // Start OpenMatrix UI
  ui.begin();

  // Start ElegantOTA
  Serial.println("[*] Attaching ElegantOTA");
  ElegantOTA.begin(&server);

  // Start NetWizard with "LiveGrid" AP
  Serial.println("[*] Starting NetWizard");
  NW.autoConnect("LiveGrid", "");

  server.begin();
  
  // Check if configured
  if (NW.isConfigured()) {
    Serial.println("OpenMatrix is configured!");
  } else {
    Serial.println("OpenMatrix is not configured yet! Please connect to LiveGrid AP and setup your device.");
  }
}

void loop(void) {
  // Handle WebServer
  server.handleClient();

  static unsigned long lastTime = 0;  
  static uint16_t x = 0;
  static uint16_t y = 0;  
  unsigned long currentTime = millis();

  if (millis() - lastTime >= 40) {
    effectManager.updateCurrentEffect();
    matrix.update();
    lastTime = currentTime;
  }

  ElegantOTA.loop();
  NW.loop();

  // static unsigned long lastFpsTime = 0;
  // static int frameCount = 0;
  // frameCount++;

  // if (currentTime - lastFpsTime >= 5000) {
  //   float fps = frameCount / 5.0f;
  //   Serial.printf("FPS: %.2f\n", fps);
  //   frameCount = 0;
  //   lastFpsTime = currentTime;
  // }
}