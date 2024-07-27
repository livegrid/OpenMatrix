#include <WebServer.h>
#include <NetWizard.h>
#include <ElegantOTA.h>
#include "StateManager.h"
#include "UI.h"
#include "GeneralSettings.h"

#include "Matrix.h"
Matrix matrix;

#ifdef SCD40_ENABLED
#include <SCD40/scd40.h>
SCD40 scd40;
#endif


#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 0

StateManager stateManager;

WebServer server(80);
NetWizard NW(&server);
UI Interface(&server, &stateManager);

#ifdef ADXL345_ENABLED
#include "AutoRotate.h"
AutoRotate autoRotate(&matrix);
#endif

TaskHandle_t updateMatrixTaskHandle;
TaskHandle_t serverTaskHandle;

void updateMatrix() {
  effectManager.updateCurrentEffect();
  matrix.update();
}

void serverTask(void *parameter) {
  for (;;) {
    server.handleClient();
    ElegantOTA.loop();
    NW.loop();
    vTaskDelay(1); // Small delay to prevent watchdog timer issues
  }
}

void updateMatrixTask(void *parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(20);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    static unsigned long lastLogTime = 0;
    static unsigned long lastFrameTime = 0;
    unsigned long currentTime = millis();
    
    // Calculate and print framerate every 5 seconds
    if (currentTime - lastLogTime >= 10000) {
      float framerate = 1000.0 / (currentTime - lastFrameTime);
      log_e("Framerate: %.2f FPS", framerate);
      lastLogTime = currentTime;
    }
    lastFrameTime = currentTime;
    effectManager.updateCurrentEffect();
    matrix.update();

    vTaskDelayUntil(&xLastWakeTime, xFrequency); // Delay until the next interval
  }
}

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

  #ifdef SCD40_ENABLED
  scd40.init();
  #endif
  #if ADXL345_ENABLED
  autoRotate.init();
  #endif
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
  Interface.begin();

  Interface.onPower([&](bool state) {
    stateManager.getState()->power = state;
    // Save state
    stateManager.save();

    // TODO: Change matrix power state below this line
    if (state == true) {
      // Turn on matrix
    } else {
      // Turn off matrix
    }
  });

  Interface.onBrightness([&](uint8_t value) {
    stateManager.getState()->brightness = value;
    // Save state
    stateManager.save();
    
    // TODO: Adjust matrix brightness below this line
  });

  Interface.onMode([&](OpenMatrixMode mode) {
    stateManager.getState()->mode = mode;
    // Save state
    stateManager.save();

    // TODO: Change matrix mode below this line
  });

  Interface.onEffect([&](Effects effect) {
    stateManager.getState()->effects.selected = effect;
    // Save state
    stateManager.save();

    // TODO: Change matrix effect below this line
  });

  // Start ElegantOTA
  Serial.println("[*] Attaching ElegantOTA");
  ElegantOTA.begin(&server);

  // Start NetWizard with "LiveGrid" AP
  Serial.println("[*] Starting NetWizard");
  NW.autoConnect("LiveGrid", "");

  // Start WebServer
  server.begin();
  
  // Check if configured
  if (NW.isConfigured()) {
    Serial.println("OpenMatrix is configured!");
  } else {
    Serial.println("OpenMatrix is not configured yet! Please connect to LiveGrid AP and setup your device.");
  }

  // Start matrix task
  xTaskCreatePinnedToCore(
    updateMatrixTask,          // Task function
    "Update Matrix",           // Name of the task
    8192,                      // Stack size in words
    NULL,                      // Task input parameter
    1,                         // Priority of the task
    NULL,                      // Task handle
    1                          // Core where the task should run (1)
  );

  // Start server task
  xTaskCreatePinnedToCore(
    serverTask,                // Task function
    "Server Task",             // Name of the task
    8192,                      // Stack size in words
    NULL,                      // Task input parameter
    1,                         // Pri ority of the task
    &serverTaskHandle,         // Task handle
    0                          // Core where the task should run (0)
  );
}

void loop(void) {
  vTaskDelete(NULL); // Delete the task running the loop function
}