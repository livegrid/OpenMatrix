#include "GeneralSettings.h"
#include "StateManager.h"
#include "TaskManager.h"

#ifdef WIFI_ENABLED
#include <ElegantOTA.h>
#include <NetWizard.h>
#include <WebServer.h>
#include <Edmx.h>
#include "MQTTManager.h"

#include "UI.h"
#include "WebServerManager.h"
#endif

TaskManager& taskManager = TaskManager::getInstance();

#ifdef PANEL_UPCYCLED
#include "MBI5153/UMatrix.h"
UMatrix matrix;
#else
#include "Original/OMatrix.h"
OMatrix matrix;
#endif

#ifdef SCD40_ENABLED
#include "SCD40.h"
SCD40 scd40;
#endif

#include "Aquarium.h"
Aquarium aquarium(&matrix, &scd40);

#ifdef ADXL345_ENABLED
#include "AutoRotate.h"
AutoRotate autoRotate(&matrix);
#endif

StateManager stateManager;

#ifdef TOUCH_ENABLED
#include "TouchMenu.h"
TouchMenu touchMenu(&matrix, &taskManager, &stateManager);
#endif

#include "EffectManager.h"
EffectManager effectManager(&matrix);

#include "ImageDraw.h"
ImageDraw imageDraw(&matrix);

#include "TextDraw.h"
TextDraw textDraw(&matrix);

#include "Edmx.h"
Edmx dmx(&matrix, &stateManager);

#include "MQTTManager.h"

#ifdef WIFI_ENABLED
WebServerManager webServerManager(&matrix, &effectManager, &imageDraw,
                                  &stateManager, &taskManager);
#endif

#ifndef SCD40_ENABLED
void demoTask(void* parameter) {
  for (;;) {
    // Update temperature, humidity and CO2
    State* state = stateManager.getState();
    state->environment.temperature.value = random(0, 100);
    state->environment.temperature.diff.type = DiffType::DISABLE;
    state->environment.humidity.value = random(0, 100);
    state->environment.humidity.diff.type = DiffType::DISABLE;
    state->environment.co2.value = random(0, 5000);
    state->environment.co2.diff.type = DiffType::DISABLE;

    // Randomize History
    for (int i = 0; i < 24; i++) {
      state->environment.temperature.history_24h[i] = random(0, 100);
      state->environment.humidity.history_24h[i] = random(0, 100);
      state->environment.co2.history_24h[i] = random(0, 5000);
    }

    // Delay for 1 second
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
#endif

void displayTask(void* parameter) {
  const uint8_t idealFPS = 30; // Set your desired FPS here
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / idealFPS);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Refresh Matrix Config every 30 seconds
  unsigned long lastRefreshTime = 0;
  const unsigned long refreshInterval = 30000; // 30 seconds
  
  static unsigned long lastLogTime = 0;
  static unsigned long frameCount = 0;
  uint8_t currentMode = 99; // make sure currentMode is not the same as OpenMatrixMode
  
  // Initialize components
  effectManager.setEffect(stateManager.getState()->effects.selected - 1);
  imageDraw.begin();
    stateManager.getState()->mode = OpenMatrixMode::AQUARIUM;
  
  for (;;) {
    unsigned long currentTime = millis();
    // stateManager.getState()->mode = OpenMatrixMode::AQUARIUM;
    
  #ifdef PANEL_UPCYCLED
    if (currentTime - lastRefreshTime >= refreshInterval) {
      log_i("Refreshing Matrix Config");
      matrix.refreshMatrixConfig();
      lastRefreshTime = currentTime;
    }
  #endif

    if(currentMode != stateManager.getState()->mode) {
      if(currentMode == OpenMatrixMode::IMAGE) {
        imageDraw.closeGIF();
      }
      currentMode = stateManager.getState()->mode;
      switch (currentMode) {
        case OpenMatrixMode::EFFECT:
          effectManager.setEffect(stateManager.getState()->effects.selected - 1);
          break;
        case OpenMatrixMode::IMAGE:
          imageDraw.openGIF(stateManager.getState()->image.selected.c_str());
          break;
        case OpenMatrixMode::TEXT:
          textDraw.setSize(stateManager.getState()->text.size);
          textDraw.drawText(stateManager.getState()->text.payload);
          break;
      }
    }
    
    switch (stateManager.getState()->mode) {
      case OpenMatrixMode::EFFECT:
        effectManager.updateCurrentEffect();
        matrix.background->display();
        matrix.update();
        break;
      case OpenMatrixMode::IMAGE:
        imageDraw.showGIF();
        matrix.background->display();
        matrix.update();
        break;
      case OpenMatrixMode::TEXT:
        textDraw.setSize(stateManager.getState()->text.size);
        textDraw.drawText(stateManager.getState()->text.payload);
        matrix.background->display();
        matrix.update();
        break;
      case OpenMatrixMode::AQUARIUM:
        aquarium.update();
        matrix.gfx_compositor->Stack(*matrix.background, *matrix.foreground);
        matrix.foreground->clear();
        matrix.update();
        break;
      case OpenMatrixMode::DMX:
        dmx.update();
        matrix.update();
        break;
      default:
        break;
      
    }
    
    frameCount++;

    if (millis() - lastLogTime >= 30000) {
      float framerate = frameCount / ((currentTime - lastLogTime) / 1000.0);
      log_i("Framerate: %.2f FPS", framerate);
      log_i("Free heap: %u bytes", ESP.getFreeHeap());
      log_i("Min free heap: %u bytes", ESP.getMinFreeHeap());
      log_i("Max alloc heap: %u bytes", ESP.getMaxAllocHeap());
      lastLogTime = currentTime;
      frameCount = 0;
    }
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

#ifdef TOUCH_ENABLED
void touchTask(void* parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(30);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    touchMenu.update();
    vTaskDelayUntil(&xLastWakeTime,
                    xFrequency);  // Delay until the next interval
  }
}
#endif

void serverTask(void* parameter) {
  for (;;) {
    webServerManager.handleClient();
    vTaskDelay(1);  // Small delay to prevent watchdog timer issues
  }
}

void mqttTask(void* parameter) {
  MQTTManager& mqttManager = MQTTManager::getInstance();
  mqttManager.begin("192.168.1.102", 1883, &stateManager);

  // Set up a callback for incoming messages
  mqttManager.setCallback([&mqttManager](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    mqttManager.handleIncomingMessage(topic, payload, properties, len, index, total);
  });
  

  const TickType_t xFrequency = pdMS_TO_TICKS(5000); // 5 seconds
  TickType_t xLastWakeTime = xTaskGetTickCount();

  bool configPublished = false;

  for (;;) {
    if (mqttManager.isConnected()) {
      if (!configPublished) {
        // Publish Home Assistant discovery messages
        mqttManager.publishHomeAssistantConfig();
        configPublished = true;
        log_i("MQTT: Published Home Assistant discovery config");
      }
      float temperature = stateManager.getState()->environment.temperature.value;
      float humidity = stateManager.getState()->environment.humidity.value;
      int co2 = stateManager.getState()->environment.co2.value;

      mqttManager.publishSensorData(temperature, humidity, co2);
      log_i("MQTT: Sent sensor data to Home Assistant");
    } else {
      log_w("MQTT: Not connected, skipping message");
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup(void) {
  // Serial.begin(115200);
  log_i("\n");

  log_i(R""""(
 _     _            ____      _     _ 
| |   (_)_   _____ / ___|_ __(_) __| |
| |   | \ \ / / _ \ |  _| '__| |/ _` |
| |___| |\ V /  __/ |_| | |  | | (_| |
|_____|_| \_/ \___|\____|_|  |_|\__,_|

)"""");
  log_i("----------------------------");
  log_i("Firmware Version: %u.%u.%u", FIRMWARE_VERSION_MAJOR,
        FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
  log_i("");

  matrix.init();

  // Start LittleFS
  LittleFS.begin(true);
  // Restore State
  stateManager.restore();
  matrix.setBrightness(stateManager.getState()->brightness);
  // Start periodic save task
  stateManager.startPeriodicSave();

#ifdef SCD40_ENABLED
  scd40.init();
#else
  TaskManager::getInstance().createTask("DemoTask", demoTask, 1024, 1, 1);
#endif

#if ADXL345_ENABLED
  autoRotate.init();
#endif

#ifdef TOUCH_ENABLED
  touchMenu.setupInterrupts();
#endif

  TaskManager::getInstance().createTask("DisplayTask", displayTask, 8192, 1, 1);

#ifdef WIFI_ENABLED
  // Start server task
  TaskManager::getInstance().createTask("ServerTask", serverTask, 4096, 1, 0);
#endif

#ifdef TOUCH_ENABLED
  TaskManager::getInstance().createTask("TouchTask", touchTask, 2048, 1, 0);
#endif

#ifdef WIFI_ENABLED
  webServerManager.begin();
  dmx.begin();
#endif

  // Initialize MQTT
  TaskManager::getInstance().createTask("MQTTTask", mqttTask, 4096, 1, 0);
  stateManager.getState()->settings.mqtt.matrix_text_topic = "homeassistant/text/livegrid/matrix_text/set";
}

void loop(void) {
  vTaskDelete(NULL);  // Delete the task running the loop function
}