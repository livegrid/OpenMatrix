#include "GeneralSettings.h"
#include "StateManager.h"
#include "TaskManager.h"
#include <esp_heap_caps.h>

#ifdef WIFI_ENABLED
#include <Edmx.h>
// #include <ElegantOTA.h>
// #include <NetWizard.h>
#include <WebServer.h>

#include "MQTTManager.h"
#include "UI.h"
#include "WebServerManager.h"
#endif

#include <DebugMonitor.h>

#include <esp_err.h>
#include <esp_task_wdt.h>

#ifdef BLE_HID_REMOTE_ENABLED
#include "BleHidRemote.h"
#endif

TaskManager& taskManager = TaskManager::getInstance();

#ifdef PANEL_UPCYCLED
#include "MBI5153/UMatrix.h"
UMatrix matrix;
#else
#include "Original/OMatrix.h"
OMatrix matrix;
#endif

StateManager stateManager(STATE_SAVE_INTERVAL);

#ifdef SCD40_ENABLED
#include "SCD40.h"
SCD40 scd40;
#endif

#ifdef VL53L8CX_ENABLED
#include "TOFSensor.h"
#include "TOFVisualizer.h"
TOFSensor tofSensor(TOF_PWREN_PIN_1, TOF_SENSOR_1_ADDRESS);
// Static allocation instead of heap to reduce fragmentation
TOFVisualizer tofVisualizerStatic(&tofSensor, nullptr);
TOFVisualizer* tofVisualizer = &tofVisualizerStatic;
#endif

#include "Aquarium.h"
#ifdef SCD40_ENABLED
  Aquarium aquarium(&matrix, &scd40, &stateManager);
#else
  Aquarium aquarium(&matrix, nullptr, &stateManager);  // nullptr when SCD40 disabled
#endif

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

#ifdef VL53L8CX_ENABLED
static constexpr int16_t kRemoteTofMaxMinMm = 1000;
static constexpr int16_t kRemoteTofMaxMaxMm = 4000;
static int16_t g_tofRuntimeMaxDetectionDistance = TOF_MAX_DETECTION_DIST;

static void applyRuntimeTofRange() {
  tofVisualizer->setDistanceRange(TOF_MIN_DETECTION_DIST, g_tofRuntimeMaxDetectionDistance);
  effectManager.setTofDetectionRange(TOF_MIN_DETECTION_DIST, g_tofRuntimeMaxDetectionDistance);
  log_i("TOF range updated: min=%d max=%d", TOF_MIN_DETECTION_DIST,
        g_tofRuntimeMaxDetectionDistance);
}
#endif

#if defined(VL53L8CX_ENABLED) && defined(BLE_HID_REMOTE_ENABLED)
static void onBleRemoteTofRangeAdjust(int deltaMm) {
  int32_t next = (int32_t)g_tofRuntimeMaxDetectionDistance + (int32_t)deltaMm;
  if (next < kRemoteTofMaxMinMm) {
    next = kRemoteTofMaxMinMm;
  }
  if (next > kRemoteTofMaxMaxMm) {
    next = kRemoteTofMaxMaxMm;
  }
  int16_t clamped = (int16_t)next;
  if (clamped == g_tofRuntimeMaxDetectionDistance) {
    return;
  }
  g_tofRuntimeMaxDetectionDistance = clamped;
  applyRuntimeTofRange();
}
#endif

#include "ImageDraw.h"
ImageDraw imageDraw(&matrix);

#include "TextDraw.h"
TextDraw textDraw(&matrix);

#ifdef WIFI_ENABLED
WebServerManager webServerManager(&matrix, &effectManager, &imageDraw,
                                  &stateManager, &taskManager);

// Edmx and MQTT only exist when WiFi is enabled - they're both networked services.
Edmx& dmx = Edmx::getInstance();
#endif

#ifdef TOUCH_ENABLED
#include "TouchMenu.h"
#ifdef WIFI_ENABLED
TouchMenu touchMenu(&matrix, &stateManager, &webServerManager);
#else
TouchMenu touchMenu(&matrix, &stateManager, nullptr);
#endif
#endif

// Small helpers so the display loop stays clean when TOUCH_ENABLED is off.
static inline bool touchShouldStartDemo() {
#ifdef TOUCH_ENABLED
  return touchMenu.shouldStartDemo();
#else
  return false;
#endif
}
static inline bool touchIsMenuOpen() {
#ifdef TOUCH_ENABLED
  return touchMenu.isMenuOpen();
#else
  return false;
#endif
}
static inline bool touchShowSensorData() {
#ifdef TOUCH_ENABLED
  return touchMenu.showSensorData();
#else
  return false;
#endif
}

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
  // Give some time for system to stabilize after boot
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Initialize matrix
  log_i("Initializing matrix display...");
  matrix.init();
  matrix.setRotation(2);
  matrix.setBrightness(200);
  
  const uint8_t idealFPS = 30;  // Set your desired FPS here
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / idealFPS);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // Refresh Matrix Config every 5 mins
  unsigned long lastRefreshTime = 0;
  const unsigned long refreshInterval = 300000;  // 5 mins

  static unsigned long lastLogTime = 0;
  static unsigned long frameCount = 0;

  uint8_t currentMode =
      99;  // make sure currentMode is not the same as OpenMatrixMode

  // Initialize components
  imageDraw.begin();
  
#ifdef VL53L8CX_ENABLED
  // Initialize TOF visualizer after matrix is initialized (static allocation)
  tofVisualizer->setMatrix(&matrix);
  applyRuntimeTofRange();
  tofSensor.setRotation(TOF_DEFAULT_ROTATION);
  
  // Connect TOF sensor to interactive effects (MeteorShower, SpaceInvaders, GravityFlap)
  effectManager.setTofSensor(&tofSensor);
  
  // Connect TOF sensor to Aquarium for interactive fish behavior
  aquarium.setTofSensor(&tofSensor);
#endif

  // Route large allocations (Fish, Plants, Boids) to PSRAM so internal heap
  // stays free for WiFi, E1.31/AsyncUDP, and mDNS.
  log_i("Aquarium begin: routing heap allocs >= 64 B to PSRAM (internal free=%u)",
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  heap_caps_malloc_extmem_enable(64);
  aquarium.begin();
  log_i("Aquarium begin done (internal free=%u)", (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

#ifdef BLE_HID_REMOTE_ENABLED
  pinMode(0, INPUT_PULLUP);
#endif

  esp_task_wdt_add(NULL);

  for (;;) {
    esp_task_wdt_reset();
    unsigned long currentTime = millis();

    if (stateManager.getState()->power) {
#ifdef BLE_HID_REMOTE_ENABLED
      {
        static constexpr unsigned long kIo0BootDebounceMs = 240;
        static bool io0_was_high                       = true;
        static unsigned long io0_prevTriggerMs           = 0;

        const bool io0_high   = digitalRead(0);
        unsigned long io0_now = millis();
        if (io0_was_high && !io0_high &&
            io0_now - io0_prevTriggerMs >= kIo0BootDebounceMs) {
          io0_prevTriggerMs = io0_now;
          bleHidRemotePrevEffect();
        }
        io0_was_high = io0_high;
      }
#endif
      // Pin 2 disabled - now used by TOF sensor power enable
      // digitalWrite(2, LOW);
      if (currentMode != stateManager.getState()->mode) {
        if (currentMode == OpenMatrixMode::IMAGE) {
          imageDraw.closeGIF();
        }
        currentMode = stateManager.getState()->mode;
        switch (currentMode) {
          case OpenMatrixMode::EFFECT:
            break;
          case OpenMatrixMode::IMAGE:
            imageDraw.openGIF(stateManager.getState()->image.selected.c_str());
            break;
          case OpenMatrixMode::TEXT:
            textDraw.setSize(stateManager.getState()->text.size);
            textDraw.drawText(stateManager.getState()->text.payload);
            break;
          default:
            break;
        }
      }

      // Apply effect when selected changes (e.g. BLE remote) without a mode transition.
      {
        State* st = stateManager.getState();
        static Effects lastEffectApplied = Effects::NONE;
        if (st->mode == OpenMatrixMode::EFFECT) {
          if (st->effects.selected != lastEffectApplied) {
            effectManager.setEffect(st->effects.selected - 1);
            lastEffectApplied = st->effects.selected;
          }
        } else {
          lastEffectApplied = Effects::NONE;
        }
      }
      
      if (stateManager.getState()->firstBoot && aquarium.isDemoFinished()) {
        stateManager.getState()->firstBoot = false;
        stateManager.save();
      }

      if (touchShouldStartDemo()) {
        aquarium.startDemo();
      }

#ifdef TOUCH_ENABLED
      if (touchIsMenuOpen()) {
        touchMenu.displayMenu();
      } else
#endif
      {
#if defined(VL53L8CX_ENABLED)
        if (stateManager.getState()->tofDebugView && tofSensor.isActive()) {
          tofVisualizer->draw();
          matrix.background->display();
        } else
#endif
        {
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
              aquarium.update(touchShowSensorData());
              aquarium.display();
              break;
#ifdef WIFI_ENABLED
            case OpenMatrixMode::DMX:
              dmx.update();
              break;
#endif
            default:
              break;
          }
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

      if (millis() - lastFramerateLogTime >= 1000) {
        // Compute elapsed time between framerate logs, NOT since main loop started
        unsigned long now = millis();
        float secondsElapsed = (now - lastFramerateCheckTime) / 1000.0f;
        float framerate = secondsElapsed > 0 ? (frameCount / secondsElapsed) : 0.0f;
        log_i("Framerate: %.2f", framerate);

        lastFramerateLogTime = now;
        lastFramerateCheckTime = now;
        frameCount = 0;
      }

    }
    else {
      // Pin 2 disabled - now used by TOF sensor power enable
      // digitalWrite(2, HIGH);
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
  // Delay for DisplayTask to init matrix (needed by dmx.begin) and imageDraw/aquarium.
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  log_i("[ServerTask] ========================================");
  log_i("[ServerTask] Starting network services...");
  log_i("[ServerTask] Free heap: %u bytes (internal=%u, largest_block=%u) [AsyncUDP needs internal]",
        (unsigned)ESP.getFreeHeap(),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  log_i("[ServerTask] ========================================");
  
  // Initialize DMX after web server is ready
  log_i("[ServerTask] Initializing DMX/E131...");
  dmx.begin(&matrix, &stateManager);
  
  // Setup the web interface and start server (these don't require WiFi to be connected)
  log_i("[ServerTask] Setting up web interface...");
  webServerManager.setupInterface();
  webServerManager.startServer();
  webServerManager.setupUniqueHostname();
  
  log_i("[ServerTask] All services started. Entering main loop.");
  
  // Main server loop
  for (;;) {
    webServerManager.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay - reduces CPU usage while maintaining responsiveness
  }
}
#endif

#ifdef WIFI_ENABLED
// Drives publishing cadence + reconnect decisions; MQTT connect/publish handled in MQTTManager.
void mqttTask(void* parameter) {
  MQTTManager& mqttManager = MQTTManager::getInstance();
  mqttManager.begin(&stateManager);

  mqttManager.setCallback(
      [&mqttManager](char* topic, char* payload,
                     AsyncMqttClientMessageProperties properties, size_t len,
                     size_t index, size_t total) {
        mqttManager.handleIncomingMessage(topic, payload, properties, len,
                                          index, total);
      });

  const TickType_t xFrequency = pdMS_TO_TICKS(5000);
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
    mqttManager.loop(
        stateManager.getState()->environment.temperature.value,
        stateManager.getState()->environment.humidity.value,
        stateManager.getState()->environment.co2.value);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
#endif

#ifdef VL53L8CX_ENABLED
void tofTask(void* parameter) {
  // Wait for I2C bus to stabilize (initialized by SCD40)
  // Also delay to allow WiFi/network services to start without memory contention
  vTaskDelay(pdMS_TO_TICKS(7000));  // Wait 7 seconds for WiFi to stabilize
  
  // Initialize sensor
  log_i("TOF Task: Starting sensor initialization...");
  log_i("TOF Task: Free heap before init: %u bytes", ESP.getFreeHeap());
  
  if (!tofSensor.begin()) {
    log_e("TOF Task: Sensor initialization failed!");
    vTaskDelete(NULL);
    return;
  }
  
  log_i("TOF Task: Sensor initialized successfully!");
  log_i("TOF Task: Free heap after init: %u bytes", ESP.getFreeHeap());
  
  // Poll the sensor faster than its 15Hz frame rate so we pick up
  // new frames with minimal extra latency (~30 polls/sec).
  const TickType_t xFrequency = pdMS_TO_TICKS(33);  // ~30Hz polling
  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  static int updateCount = 0;
  
  for (;;) {
    // Update sensor and get new data
    if (tofSensor.update()) {
      // New data available, visualization will be updated in display task
      updateCount++;
      if (updateCount % 50 == 0) {  // Log every 5 seconds (50 * 100ms)
        log_i("TOF: Active, received %d updates", updateCount);
      }
    }
    
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
#endif

void sensorTask(void* parameter) {
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 second
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (;;) {
#ifdef BH1750_ENABLED
    autoBrightness.updateSensorValues();
    if (stateManager.getState()->autobrightness) {
      matrix.setBrightness(autoBrightness.matrixBrightness());
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

  // Pin 2 disabled - now used by TOF sensor power enable
  // pinMode(2, OUTPUT);
  // digitalWrite(2, LOW);

#ifdef SCD40_ENABLED
  delay(50);
  scd40.init();
  log_i("SCD40 sensor initialized");
#else
  log_i("SCD40 disabled - using default environmental values");
  // No need for demo task - aquarium will use default values
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
  // Aquarium TOF iteration: boot straight into Aquarium for faster test cycles.
  stateManager.getState()->mode = OpenMatrixMode::AQUARIUM;
  stateManager.save();
  // stateManager.startPeriodicSave();

  
#ifdef RUN_DEMO
  if(stateManager.getState()->firstBoot) {
    aquarium.startDemo();
  }
#endif

  // === CRITICAL: WiFi init MUST run before DisplayTask ===
  // DisplayTask allocates matrix buffers, aquarium, etc. (~40KB+). If WiFi init runs
  // in ServerTask (after DisplayTask starts), heap drops to ~20KB and esp_wifi_init
  // fails with 257 (ESP_ERR_NO_MEM). Initialize WiFi in setup() when heap is ~75KB.
#ifdef WIFI_ENABLED
  log_i("Phase 0: Initializing WiFi (before DisplayTask to preserve heap)...");
  log_i("Free heap before WiFi: %u bytes", ESP.getFreeHeap());
  webServerManager.connectToWiFi();
  log_i("WiFi phase complete. Free heap: %u bytes", ESP.getFreeHeap());
#endif

  esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 5000,
      .idle_core_mask = 0,
      .trigger_panic = true,
  };
  esp_err_t wdt_err = esp_task_wdt_init(&wdt_config);
  if (wdt_err != ESP_OK) {
    log_e("Failed to initialize task watchdog timer: %s", esp_err_to_name(wdt_err));
  }

  // === PHASE 1: Core tasks ===
  log_i("Phase 1: Creating Display and Server tasks...");

  // DisplayTask:
  //   Steady-state HWM observed ~4.4 KB (includes the ~4 KB Water batch buffer).
  //   However aquarium.begin() on first boot has a deeper call tree (fish/boid
  //   construction, state serialization, etc.) and overflowed a 12 KB stack.
  //   5120 words = 20 KB gives plenty of headroom for that path while still
  //   saving 12 KB vs the original 32 KB.
  TaskManager::getInstance().createTask("DisplayTask", displayTask, 8192, 1, 1, false);

#ifdef WIFI_ENABLED
  // ServerTask in PSRAM: frees ~30KB internal heap for AsyncUDP.listen() (needs xTaskCreate from internal)
  TaskManager::getInstance().createTask("ServerTask", serverTask, 7528, 1, 0, true);

  // MQTT task - only runs when WiFi is enabled. Stays dormant until valid settings exist.
  TaskManager::getInstance().createTask("MQTTTask", mqttTask, 4096, 1, 0, true);
#endif

  // === PHASE 2: Additional tasks ===
  log_i("Phase 2: Creating Touch, Sensor, and TOF tasks...");

#ifdef TOUCH_ENABLED
  // TouchTask: HWM 1388 B → 2048 words = 8KB [Core 1 - with display for UI responsiveness]
  // TaskManager::getInstance().createTask("TouchTask", touchTask, 2048, 1, 1, false);
#endif

// Create the combined sensor task (BH1750 + ADXL345). Tune with DebugMonitor HWM.
#if defined(BH1750_ENABLED) || defined(ADXL345_ENABLED)
  // SensorTask: HWM 604 B → 2048 words = 8KB [Core 0 - with other sensors]
  TaskManager::getInstance().createTask("SensorTask", sensorTask, 2048, 1, 0, false);
#endif

#ifdef VL53L8CX_ENABLED
  // TOFTask: HWM 6488 B → 7168 words = 28KB [Core 0 - I2C hardware access]
  // Using PSRAM stack to save internal RAM (I2C operations don't need fast memory)
  TaskManager::getInstance().createTask("TOFTask", tofTask, 7168, 1, 0, true);
#endif

#ifdef BLE_HID_REMOTE_ENABLED
  bleHidRemoteSetStateManager(&stateManager);
#if defined(VL53L8CX_ENABLED)
  bleHidRemoteSetTofRangeAdjustCallback(onBleRemoteTofRangeAdjust);
#endif
  // Runs after WiFi phase; long connect/GATT work stays off the main setup path.
  log_i("BLE HID remote: starting NimBLE task...");
  TaskManager::getInstance().createTask("BleHidRemote", bleHidRemoteTask, 8192, 1, 0, false);
#endif

  // TaskManager::getInstance().createTask("RestartTask", restartTask, 1024, 1, 0);

  // One-time post-init heap snapshot (internal + PSRAM, largest block for fragmentation)
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
