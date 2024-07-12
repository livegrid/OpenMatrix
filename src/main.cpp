#include <WebServer.h>
#include <NetWizard.h>
#include <ElegantOTA.h>
#include "ConfigManager.h"

#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0

WebServer server(80);
NetWizard NW(&server);

ConfigManager cfgManager;

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

  // Start ElegantOTA
  Serial.println("[*] Attaching ElegantOTA");
  ElegantOTA.begin(&server);

  // Start NetWizard with "LiveGrid" AP
  Serial.println("[*] Starting NetWizard");
  NW.autoConnect("LiveGrid", "");
  
  // Check if configured
  if (NW.isConfigured()) {
    Serial.println("LiveGrid is configured!");
  } else {
    Serial.println("LiveGrid is not configured yet! Please connect to LiveGrid AP and setup your device.");
  }

  // Start WebServer
  server.begin();
}

void loop(void) {
  // Handle WebServer
  server.handleClient();

  ElegantOTA.loop();
  NW.loop();
}