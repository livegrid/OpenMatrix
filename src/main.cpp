#include "GeneralSettings.h"
#include "StateManager.h"
#include "TaskManager.h"

#ifdef WIFI_ENABLED
  #include <NetWizard.h>
  #include <WebServer.h>
  #include "UI.h"
  #include "WebServerManager.h"
#endif

#ifdef OTA_ENABLED
  #include <ElegantOTA.h>
#endif

#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 1
#define FIRMWARE_VERSION_PATCH 0

TaskManager &taskManager = TaskManager::getInstance();

#include "Matrix.h"
Matrix matrix;

#ifdef SCD40_ENABLED
#include "SCD40.h"
SCD40 scd40;
#endif

#ifdef ADXL345_ENABLED
#include "AutoRotate.h"
AutoRotate autoRotate(&matrix);
#endif

#ifdef TOUCH_ENABLED
#include "TouchMenu.h"
TouchMenu touchMenu(&matrix, &taskManager);
#endif

#include "EffectManager.h"
EffectManager effectManager(&matrix);

StateManager stateManager;

#ifdef WIFI_ENABLED
WebServerManager webServerManager(&matrix, &effectManager, &stateManager);
#endif

TaskHandle_t effectsTaskHandle;
TaskHandle_t touchTaskHandle;


#ifndef SCD40_ENABLED
void demoTask(void *parameter) {
  for (;;) {
    // Update temperature, humidity and CO2
    State *state = stateManager.getState();
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

void effectsTask(void *parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(30);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  uint8_t currentEffect = 0;
  effectManager.setEffect(currentEffect);

  for (;;) {
    static unsigned long lastLogTime = 0;
    static unsigned long lastFrameTime = 0;
    unsigned long currentTime = millis();

    effectManager.updateCurrentEffect();
    matrix.background->display();

    // matrix.background->clear();
    // matrix.background->fillScreen(CRGB(0, 255, 0));
    // matrix.background->fillCircle(32, 32, 6, CRGB(0, 0, 255));
    // matrix.foreground->clear();
    // matrix.foreground->fillCircle(32, 32, 12, CRGB(255, 0, 0));
    // // matrix.foreground->display();
    // matrix.gfx_compositor->Blend(*matrix.background, *matrix.foreground, 127);

    // Calculate and print framerate every 5 seconds
    if (currentTime - lastLogTime >= 10000) {
      float framerate = 1000.0 / (currentTime - lastFrameTime);
      log_e("Framerate: %.2f FPS", framerate);
      lastLogTime = currentTime;

      // effectManager.nextEffect();
    }
    lastFrameTime = currentTime;
    vTaskDelayUntil(&xLastWakeTime, xFrequency);  // Delay until the next interval
  }
}

#ifdef TOUCH_ENABLED
void touchTask(void *parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(30);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    touchMenu.update();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);  // Delay until the next interval
  }
}
#endif

#ifdef WIFI_ENABLED
void serverTask(void *parameter) {
  for (;;) {
    webServerManager.handleClient();
    vTaskDelay(1);  // Small delay to prevent watchdog timer issues
  }
}
#endif

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

  // Start matrix task
  TaskManager::getInstance().createTask("EffectsTask", effectsTask, 4096, 1, 1);

#ifdef WIFI_ENABLED
  // Start server task
  TaskManager::getInstance().createTask("ServerTask", serverTask, 4096, 1, 1);
#endif

#ifdef TOUCH_ENABLED
  TaskManager::getInstance().createTask("TouchTask", touchTask, 2048, 1, 1);
#endif

#ifdef WIFI_ENABLED
  webServerManager.begin();
#endif
}

void loop(void) {
  vTaskDelete(NULL);  // Delete the task running the loop function
}