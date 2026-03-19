#include "GeneralSettings.h"
#include "StateManager.h"
#include "TaskManager.h"
#include <esp_heap_caps.h>

#ifdef WIFI_ENABLED
#include <Edmx.h>
#include <ElegantOTA.h>
#include <NetWizard.h>
#include <WebServer.h>

#include "MQTTManager.h"
#include "UI.h"
#include "WebServerManager.h"
#endif

#include <DebugMonitor.h>

#include <esp_task_wdt.h>
#include <esp_err.h>

TaskManager& taskManager = TaskManager::getInstance();

#ifdef PANEL_UPCYCLED
#include "MBI5153/UMatrix.h"
UMatrix matrix;
#else
#include "Original/OMatrix.h"
OMatrix matrix;
#endif

StateManager stateManager(STATE_SAVE_INTERVAL);

#include "SCD40.h"
SCD40 scd40;

#include "Aquarium.h"
Aquarium aquarium(&matrix, &scd40, &stateManager);

#ifdef BH1750_ENABLED
#include "AutoBrightness.h"
#ifdef PANEL_UPCYCLED
AutoBrightness autoBrightness(&matrix, 30, 600, 100, 255);
#else
AutoBrightness autoBrightness(&matrix, 30, 600, 150, 255);
#endif
#endif

#ifdef ADXL345_ENABLED
#include "AutoRotate.h"
AutoRotate autoRotate(&matrix);
#endif

#include "EffectManager.h"
EffectManager effectManager(&matrix);

#include "ImageDraw.h"
ImageDraw imageDraw(&matrix);

#include "TextDraw.h"
TextDraw textDraw(&matrix);

#ifdef WIFI_ENABLED
WebServerManager webServerManager(&matrix, &effectManager, &imageDraw,
                                  &stateManager, &taskManager);
#endif

#include "Edmx.h"
Edmx& dmx = Edmx::getInstance();

#include "MQTTManager.h"

#ifdef TOUCH_ENABLED
#include "TouchMenu.h"
TouchMenu touchMenu(&matrix, &stateManager, &webServerManager, &aquarium);
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
  // Give time for WiFi to stabilize after init in setup()
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Initialize matrix in this task (after WiFi init has claimed its memory)
  log_i("Initializing matrix display...");
  matrix.init();
  matrix.setBrightness(stateManager.getState()->brightness);

  const uint8_t idealFPS = 30;  // Set your desired FPS here
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / idealFPS);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Refresh Matrix Config every 5 mins
  unsigned long lastRefreshTime = 0;
  const unsigned long refreshInterval = 300000;  // 5 mins

  static unsigned long frameCount = 0;

  uint8_t currentMode =
      99;  // make sure currentMode is not the same as OpenMatrixMode
  uint8_t lastAppliedBrightness = 255;

  // Initialize components
  effectManager.setEffect(stateManager.getState()->effects.selected - 1);
  imageDraw.begin();
  stateManager.getState()->mode = OpenMatrixMode::AQUARIUM;

  // Route large allocations (Fish, Plants, Boids) to PSRAM so internal heap
  // stays free for WiFi, E1.31/AsyncUDP, and mDNS.
  log_i("Aquarium begin: routing heap allocs >= 64 B to PSRAM (internal free=%u)",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  heap_caps_malloc_extmem_enable(64);
  aquarium.begin();
  heap_caps_malloc_extmem_enable(0);  // Restore default — only explicitly large allocs go to PSRAM
  log_i("Aquarium begin done (internal free=%u)",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  stateManager.save();

  esp_task_wdt_add(NULL);

  for (;;) {
    esp_task_wdt_reset();
    unsigned long currentTime = millis();

    if (stateManager.getState()->power) {
      digitalWrite(2, LOW);
      if (currentMode != stateManager.getState()->mode) {
        if (currentMode == OpenMatrixMode::IMAGE) {
          imageDraw.closeGIF();
        }
        currentMode = stateManager.getState()->mode;
        switch (currentMode) {
          case OpenMatrixMode::EFFECT:
            effectManager.setEffect(stateManager.getState()->effects.selected -
                                    1);
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

      if (stateManager.getState()->firstBoot && aquarium.isDemoFinished()) {
        stateManager.getState()->firstBoot = false;
        stateManager.save();
      }

      if (touchMenu.shouldStartDemo()) {
        aquarium.startDemo();
      }

      if (touchMenu.isMenuOpen()) {
        touchMenu.displayMenu();
      } else {
        switch (stateManager.getState()->mode) {
          case OpenMatrixMode::EFFECT:
            effectManager.updateCurrentEffect();
            matrix.background->display();
            break;
          case OpenMatrixMode::IMAGE:
            imageDraw.showGIF();
            matrix.background->display();
            break;
          case OpenMatrixMode::TEXT:
            textDraw.setSize(stateManager.getState()->text.size);
            textDraw.drawText(stateManager.getState()->text.payload);
            matrix.background->display();
            break;
          case OpenMatrixMode::AQUARIUM:
            aquarium.update(touchMenu.showSensorData());
            aquarium.display();
            break;
          case OpenMatrixMode::DMX:
            dmx.update();
            break;
          default:
            break;
        }
      }

#ifdef PANEL_UPCYCLED
      if (currentTime - lastRefreshTime >= refreshInterval) {
        log_i("Refreshing Matrix Config");
        matrix.refreshMatrixConfig();
        lastRefreshTime = currentTime;
      } else {
        matrix.update();
      }
#else
      matrix.update();
#endif

      frameCount++;

      static unsigned long lastFramerateLogTime = 0;
      static unsigned long lastFramerateCheckTime = 0;

      if (millis() - lastFramerateLogTime >= MATRIX_REFRESH_INTERVAL) {
        unsigned long now = millis();
        float secondsElapsed = (now - lastFramerateCheckTime) / 1000.0f;
        float framerate = secondsElapsed > 0 ? (frameCount / secondsElapsed) : 0.0f;
        log_i("Framerate: %.2f", framerate);

        lastFramerateLogTime = now;
        lastFramerateCheckTime = now;
        frameCount = 0;
      }

      // Apply manual brightness changes when autobrightness is disabled
      if (!stateManager.getState()->autobrightness) {
        uint8_t desired = stateManager.getState()->brightness;
        if (desired != lastAppliedBrightness) {
          matrix.setBrightness(desired);
          lastAppliedBrightness = desired;
        }
      }
    }
    else {
      digitalWrite(2, HIGH);
      matrix.clearScreen();
      matrix.update();
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

#ifdef WIFI_ENABLED
void serverTask(void* parameter) {
  // WiFi is already initialized in setup() (before DisplayTask) to avoid heap exhaustion.
  // Delay for DisplayTask to init matrix and aquarium.
  vTaskDelay(pdMS_TO_TICKS(3000));

  log_i("[ServerTask] Starting network services (heap=%u, internal=%u)",
        (unsigned)ESP.getFreeHeap(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // Initialize DMX after display is ready
  dmx.begin(&matrix, &stateManager);

  // Setup the web interface and start server
  webServerManager.setupAfterWiFi();

  log_i("[ServerTask] All services started.");

  for (;;) {
    webServerManager.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
#endif

void mqttTask(void* parameter) {
  MQTTManager& mqttManager = MQTTManager::getInstance();
  mqttManager.begin("192.168.1.102", 1883, &stateManager);

  // Set up a callback for incoming messages
  mqttManager.setCallback(
      [&mqttManager](char* topic, char* payload,
                     AsyncMqttClientMessageProperties properties, size_t len,
                     size_t index, size_t total) {
        mqttManager.handleIncomingMessage(topic, payload, properties, len,
                                          index, total);
      });

  const TickType_t xFrequency = pdMS_TO_TICKS(5000);  // 5 seconds
  TickType_t xLastWakeTime = xTaskGetTickCount();

  bool configPublished = false;

  for (;;) {
    if (mqttManager.isConnected()) {
      if (!configPublished) {
        // Discovery and availability handled on connect now
        configPublished = true;
      }
      float temperature =
          stateManager.getState()->environment.temperature.value;
      float humidity = stateManager.getState()->environment.humidity.value;
      int co2 = stateManager.getState()->environment.co2.value;

      mqttManager.publishSensorData(temperature, humidity, co2);
      log_v("MQTT: Sent sensor data to Home Assistant");
    } else {
      log_v("MQTT: Not connected, skipping message");
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void sensorTask(void* parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 second
  TickType_t xLastWakeTime = xTaskGetTickCount();
  unsigned long belowSinceMs = 0;
  bool lastScheduledOff = false;

  for (;;) {
#ifdef BH1750_ENABLED
    autoBrightness.updateSensorValues();
    if (stateManager.getState()->autobrightness) {
      matrix.setBrightness(autoBrightness.matrixBrightness());
    }

    // Dark-threshold scheduler
    if (stateManager.getState()->settings.scheduler.enableDarkAutoPower) {
      const float lux = autoBrightness.getLux();
      const float threshold = stateManager.getState()->settings.scheduler.darkThresholdLux;
      const float hysteresis = stateManager.getState()->settings.scheduler.darkHysteresisLux;
      const uint16_t stability = stateManager.getState()->settings.scheduler.darkStabilitySeconds;
      const bool isCurrentlyOff = !stateManager.getState()->power;

      const unsigned long nowMs = millis();
      const bool below = lux <= threshold;
      const bool above = lux >= (threshold + hysteresis);

      if (below) {
        if (belowSinceMs == 0) belowSinceMs = nowMs;
        if ((nowMs - belowSinceMs) >= (stability * 1000UL)) {
          if (!isCurrentlyOff) {
            stateManager.getState()->power = false; // display off only
            stateManager.save();
            lastScheduledOff = true;
          }
        }
      } else if (above) {
        belowSinceMs = 0;
        if (isCurrentlyOff && lastScheduledOff) {
          stateManager.getState()->power = true;
          stateManager.save();
          lastScheduledOff = false;
        }
      }
    }
#endif
    vTaskDelay(pdMS_TO_TICKS(5));  // 5 milliseconds delay

#ifdef ADXL345_ENABLED
    autoRotate.updateSensorValues();
#endif

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void restartTask(void* parameter) {
  const unsigned long RESTART_INTERVAL = 23UL * 60UL * 60UL * 1000UL; // 23 hours in milliseconds
  const TickType_t xFrequency = pdMS_TO_TICKS(60000); // Check every minute
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    if (millis() >= RESTART_INTERVAL) {
      log_i("Scheduled restart triggered after 23 hours");
      stateManager.save(); // Save state before restart
      aquarium.saveState();
      delay(100); // Small delay to ensure state is saved
      ESP.restart();
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup(void) {
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

  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // Watchdog timer
  esp_task_wdt_config_t config = {
      .timeout_ms = 5000,
      .idle_core_mask = 0,
      .trigger_panic = true
  };

  esp_err_t err = esp_task_wdt_init(&config);
  if (err != ESP_OK) {
    log_e("Failed to initialize task watchdog timer: %s", esp_err_to_name(err));
  }

#ifdef SCD40_ENABLED
  delay(50);
  scd40.init();
#else
  TaskManager::getInstance().createTask("DemoTask", demoTask, 1024, 1, 1);
#endif

#ifdef ADXL345_ENABLED
  delay(50);
  autoRotate.init();
#endif

#ifdef TOUCH_ENABLED
  delay(50);
  touchMenu.setupInterrupts();
#endif

#ifdef BH1750_ENABLED
  delay(50);
  autoBrightness.init();
#endif

  // Start LittleFS
  LittleFS.begin(true);

  // Restore State
  stateManager.restore();
  // Start periodic save task
  stateManager.startPeriodicSave();

  #ifdef RUN_DEMO
  if(stateManager.getState()->firstBoot) {
    aquarium.startDemo();
  }
  #endif

  // === CRITICAL: WiFi init MUST run before DisplayTask ===
  // DisplayTask allocates matrix buffers + aquarium (~40KB+). If WiFi init runs
  // after that, heap drops too low and esp_wifi_init fails with ESP_ERR_NO_MEM.
  // Initialize WiFi here in setup() when heap is still ~75KB+.
#ifdef WIFI_ENABLED
  log_i("Phase 0: Initializing WiFi (before DisplayTask to preserve heap)...");
  log_i("Free heap before WiFi: %u bytes (internal=%u)",
        (unsigned)ESP.getFreeHeap(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  webServerManager.initWiFi();
  log_i("WiFi phase complete. Free heap: %u bytes (internal=%u)",
        (unsigned)ESP.getFreeHeap(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

  // === Create tasks ===
  // DisplayTask initializes matrix and aquarium (with PSRAM routing)
  TaskManager::getInstance().createTask("DisplayTask", displayTask, 16384, 1, 1);

#ifdef WIFI_ENABLED
  // ServerTask: PSRAM stack frees ~30KB internal heap for AsyncUDP
  TaskManager::getInstance().createTask("ServerTask", serverTask, 8192, 1, 0, true);
#endif

#ifdef TOUCH_ENABLED
  TaskManager::getInstance().createTask("TouchTask", touchTask, 4096, 1, 0);
#endif

  // Initialize MQTT
  TaskManager::getInstance().createTask("MQTTTask", mqttTask, 4096, 1, 0);

// Create the combined sensor task
#if defined(BH1750_ENABLED) || defined(ADXL345_ENABLED)
  TaskManager::getInstance().createTask("SensorTask", sensorTask, 4096, 1, 0);
#endif

  // TaskManager::getInstance().createTask("RestartTask", restartTask, 1024, 1, 0);

  // Post-init heap snapshot
  {
    const size_t internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t internalLargest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const size_t psramLargest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    log_i("Heap after init: internal free=%u largest_block=%u | PSRAM free=%u largest_block=%u",
          (unsigned)internalFree, (unsigned)internalLargest, (unsigned)psramFree, (unsigned)psramLargest);
  }

  DebugMonitor::init();
}

void loop(void) {
  vTaskDelete(NULL);  // Delete the task running the loop function
}
