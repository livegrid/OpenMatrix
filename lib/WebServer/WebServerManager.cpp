#include "WebServerManager.h"

// Static pointer for WiFi event callback (needed because event handler can't capture 'this')
static WebServerManager* wifiEventInstance = nullptr;

WebServerManager::WebServerManager(Matrix* matrix, EffectManager* effectManager,
                                   ImageDraw* imageDraw, StateManager* stateManager,
                                   TaskManager* taskManager)
    : matrix(matrix),
      effectManager(effectManager),
      imageDraw(imageDraw),
      server(80),
      interface(&server, stateManager),
      stateManager(stateManager),
      taskManager(taskManager) {
  wifiEventInstance = this;
}

void WebServerManager::begin() {
  connectToWiFi();
  setupInterface();
  startServer();
  setupUniqueHostname();
}

// WiFi Event Handler - provides detailed diagnostics
void WebServerManager::onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      log_i("[WiFi Event] WiFi interface ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      log_i("[WiFi Event] Scan completed");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      log_i("[WiFi Event] Station mode started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      log_w("[WiFi Event] Station mode stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      log_i("[WiFi Event] Connected to AP (awaiting IP)");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      log_w("[WiFi Event] Disconnected from AP");
      // Auto-reconnect will be handled by the watchdog
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      log_w("[WiFi Event] Auth mode changed");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      log_i("[WiFi Event] Got IP: %s", WiFi.localIP().toString().c_str());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      log_w("[WiFi Event] Lost IP address");
      break;
    default:
      log_v("[WiFi Event] Event: %d", event);
      break;
  }
}

void WebServerManager::handleClient() {
  // WiFi connection watchdog - check every 30 seconds
  static unsigned long lastWiFiCheck = 0;
  static unsigned long lastReconnectAttempt = 0;
  static uint8_t reconnectAttempts = 0;
  static bool wasConnected = false;
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastWiFiCheck >= 30000) {  // Check every 30 seconds
    lastWiFiCheck = currentMillis;
    bool isConnected = (WiFi.status() == WL_CONNECTED);
    
    if (isConnected) {
      if (!wasConnected) {
        log_i("[WiFi Watchdog] Reconnected! IP: %s, RSSI: %d dBm", 
              WiFi.localIP().toString().c_str(), WiFi.RSSI());
        reconnectAttempts = 0;  // Reset counter on successful connection
      }
      // Log health periodically when connected
      log_v("[WiFi Watchdog] Connected - RSSI: %d dBm", WiFi.RSSI());
    } else {
      log_w("[WiFi Watchdog] Not connected! Status: %d", WiFi.status());
      
      // Attempt reconnection with exponential backoff
      unsigned long backoffTime = min(300000UL, (unsigned long)(5000 * (1 << min(reconnectAttempts, (uint8_t)6))));
      
      if (currentMillis - lastReconnectAttempt >= backoffTime) {
        log_i("[WiFi Watchdog] Attempting reconnection (attempt %d, backoff %lu ms)", 
              reconnectAttempts + 1, backoffTime);
        lastReconnectAttempt = currentMillis;
        reconnectAttempts++;
        
        // Perform a clean reconnection attempt
        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      }
    }
    
    wasConnected = isConnected;
  }
  
  server.handleClient();
  // ElegantOTA.loop();
}

void WebServerManager::connectToWiFi() {
  log_i("[*] ========================================");
  log_i("[*] WiFi Connection - ESP32-S3 Optimized");
  log_i("[*] ========================================");
  log_i("[WiFi] SSID: '%s'", WIFI_SSID);
  log_i("[WiFi] Password length: %d chars", strlen(WIFI_PASSWORD));
  log_i("[WiFi] Password check: '%c'...'%c'", 
        WIFI_PASSWORD[0], WIFI_PASSWORD[strlen(WIFI_PASSWORD)-1]);
  log_i("[WiFi] Free heap: %u bytes", ESP.getFreeHeap());
  
  // ============================================================
  // PHASE 0: Initialize NVS (Required for WiFi on ESP32-S3/IDF5)
  // Error 257 = ESP_ERR_NVS_NOT_INITIALIZED
  // ============================================================
  log_i("[WiFi] Phase 0: Initializing NVS...");
  esp_err_t nvs_err = nvs_flash_init();
  if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    log_w("[WiFi] NVS partition issue, erasing and reinitializing...");
    nvs_flash_erase();
    nvs_err = nvs_flash_init();
  }
  if (nvs_err != ESP_OK) {
    log_e("[WiFi] NVS init failed: %d", nvs_err);
  } else {
    log_i("[WiFi] NVS initialized successfully");
  }
  
  // ============================================================
  // PHASE 1: Initialize WiFi Subsystem
  // On ESP32-S3/IDF5, proper init order is critical
  // ============================================================
  log_i("[WiFi] Phase 1: Initializing WiFi subsystem...");
  log_i("[WiFi] Free heap before WiFi init: %u bytes", ESP.getFreeHeap());
  
  // First, ensure WiFi is in a clean state by setting mode
  // This initializes the WiFi driver properly
  WiFi.mode(WIFI_STA);
  delay(1000);  // Give plenty of time for driver initialization
  
  log_i("[WiFi] WiFi driver initialized, heap: %u bytes", ESP.getFreeHeap());
  
  // Now we can safely disconnect and configure
  log_i("[WiFi] Phase 2: Resetting WiFi state...");
  WiFi.disconnect(true);  // Disconnect
  delay(500);
  
  // Turn off and back on for clean state
  WiFi.mode(WIFI_OFF);
  delay(500);
  
  log_i("[WiFi] Radio powered down");
  
  // ============================================================
  // PHASE 3: Configure and Enable Station Mode
  // ============================================================
  log_i("[WiFi] Phase 3: Configuring WiFi...");
  
  // Don't persist WiFi config to NVS (prevents flash corruption issues)
  WiFi.persistent(false);
  
  // Enable auto-reconnect at driver level
  WiFi.setAutoReconnect(true);
  
  // Enable station mode
  log_i("[WiFi] Enabling station mode...");
  WiFi.mode(WIFI_STA);
  delay(500);  // Critical: Let mode change fully stabilize
  
  // Register WiFi event handler for detailed diagnostics (after mode is set)
  WiFi.onEvent([](WiFiEvent_t event) {
    if (wifiEventInstance) {
      wifiEventInstance->onWiFiEvent(event);
    }
  });
  
  // Set hostname AFTER mode change, BEFORE begin
  WiFi.setHostname("livegrid");
  delay(100);
  
  // ESP32-S3 IDF5 specific: WiFi modem sleep can cause AUTH issues
  // Use WIFI_PS_NONE for most reliable connection during initial connect
  esp_wifi_set_ps(WIFI_PS_NONE);
  
  log_i("[WiFi] Station mode ready, heap: %u bytes", ESP.getFreeHeap());
  
  // ============================================================
  // PHASE 4: Connection Loop with Retries
  // Skip scan - it can interfere with connection on ESP32-S3
  // ============================================================
  const int maxConnectionAttempts = 5;  // More attempts
  
  for (int attempt = 1; attempt <= maxConnectionAttempts; attempt++) {
    log_i("[WiFi] ----------------------------------------");
    log_i("[WiFi] Connection attempt %d/%d", attempt, maxConnectionAttempts);
    log_i("[WiFi] ----------------------------------------");
    
    if (attempt > 1) {
      // Soft reset between attempts - don't turn off WiFi completely
      // as that can cause initialization issues on ESP32-S3
      log_i("[WiFi] Soft reset for retry...");
      WiFi.disconnect(false);  // Disconnect but keep WiFi on
      delay(500);
      
      // Re-ensure we're in STA mode
      if (WiFi.getMode() != WIFI_STA) {
        WiFi.mode(WIFI_STA);
        delay(300);
      }
    }
    
    // Start the connection
    log_i("[WiFi] Calling WiFi.begin()...");
    wl_status_t beginResult = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    log_i("[WiFi] WiFi.begin() returned: %d", beginResult);
    
    // CRITICAL: ESP32-S3 IDF5 needs significant time after begin()
    // The WPA2 handshake needs time to initialize
    delay(2000);  // Increased from 1000ms
    
    // Wait for connection with detailed status logging
    const int maxWaitSeconds = 30;
    bool connected = false;
    
    for (int waitSec = 0; waitSec < maxWaitSeconds && !connected; waitSec++) {
      wl_status_t status = WiFi.status();
      
      // Log status every 2 seconds
      if (waitSec % 2 == 0) {
        const char* statusStr;
        switch (status) {
          case WL_IDLE_STATUS:     statusStr = "IDLE"; break;
          case WL_NO_SSID_AVAIL:   statusStr = "NO_SSID"; break;
          case WL_SCAN_COMPLETED:  statusStr = "SCAN_DONE"; break;
          case WL_CONNECTED:       statusStr = "CONNECTED"; break;
          case WL_CONNECT_FAILED:  statusStr = "FAILED"; break;
          case WL_CONNECTION_LOST: statusStr = "LOST"; break;
          case WL_DISCONNECTED:    statusStr = "DISCONNECTED"; break;
          case 254:                statusStr = "NO_SHIELD/NOT_INIT"; break;
          default:                 statusStr = "UNKNOWN"; break;
        }
        log_i("[WiFi] Wait %2d/%d sec - Status: %s (%d)", 
              waitSec, maxWaitSeconds, statusStr, status);
      }
      
      if (status == WL_CONNECTED) {
        connected = true;
        break;
      }
      
      // Early exit on definite failures
      if (status == WL_CONNECT_FAILED) {
        log_w("[WiFi] Connection definitively failed (wrong password?)");
        break;
      }
      if (status == WL_NO_SSID_AVAIL) {
        log_w("[WiFi] SSID not found - check if router is 2.4GHz and in range");
        break;
      }
      // Status 254 means WiFi not initialized - need full reset
      if (status == 254) {
        log_e("[WiFi] WiFi not initialized! Attempting recovery...");
        WiFi.mode(WIFI_STA);
        delay(500);
        break;
      }
      
      delay(1000);
    }
    
    if (connected) {
      log_i("[WiFi] ========================================");
      log_i("[WiFi] CONNECTED on attempt %d!", attempt);
      log_i("[WiFi] ========================================");
      log_i("[WiFi] IP Address:  %s", WiFi.localIP().toString().c_str());
      log_i("[WiFi] Gateway:     %s", WiFi.gatewayIP().toString().c_str());
      log_i("[WiFi] Subnet:      %s", WiFi.subnetMask().toString().c_str());
      log_i("[WiFi] DNS:         %s", WiFi.dnsIP().toString().c_str());
      log_i("[WiFi] RSSI:        %d dBm", WiFi.RSSI());
      log_i("[WiFi] Channel:     %d", WiFi.channel());
      log_i("[WiFi] MAC:         %s", WiFi.macAddress().c_str());
      log_i("[WiFi] BSSID:       %s", WiFi.BSSIDstr().c_str());
      
      // Re-enable power saving for normal operation (optional, saves power)
      // esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
      
      return;  // Success!
    }
    
    // If we didn't connect, add progressive delay between attempts
    if (attempt < maxConnectionAttempts) {
      int backoffMs = 2000 * attempt;  // 2s, 4s, 6s, 8s...
      log_w("[WiFi] Attempt %d failed. Waiting %d ms before retry...", 
            attempt, backoffMs);
      delay(backoffMs);
    }
  }
  
  // ============================================================
  // All attempts failed
  // ============================================================
  log_e("[WiFi] ========================================");
  log_e("[WiFi] FAILED after %d attempts", maxConnectionAttempts);
  log_e("[WiFi] ========================================");
  
  wl_status_t finalStatus = WiFi.status();
  log_e("[WiFi] Final status: %d", finalStatus);
  
  // Provide actionable diagnostic info
  log_e("[WiFi] Troubleshooting:");
  log_e("[WiFi] 1. Verify SSID '%s' is correct", WIFI_SSID);
  log_e("[WiFi] 2. Verify password is correct");
  log_e("[WiFi] 3. Ensure router is 2.4GHz (ESP32 doesn't support 5GHz)");
  log_e("[WiFi] 4. Ensure router uses WPA2-PSK (not WPA3-only)");
  log_e("[WiFi] 5. Check if router has MAC filtering enabled");
  log_e("[WiFi] 6. Try moving closer to the router");
  
  // Do a scan now for diagnostics (after failed connection attempts)
  log_i("[WiFi] Performing diagnostic scan...");
  int n = WiFi.scanNetworks();
  log_i("[WiFi] Found %d networks:", n);
  bool targetFound = false;
  for (int i = 0; i < n && i < 10; i++) {  // Limit to 10 for brevity
    log_i("[WiFi]   %d: '%s' (RSSI: %d, Enc: %d, Ch: %d)", 
          i, WiFi.SSID(i).c_str(), WiFi.RSSI(i), 
          WiFi.encryptionType(i), WiFi.channel(i));
    if (WiFi.SSID(i) == WIFI_SSID) {
      targetFound = true;
    }
  }
  WiFi.scanDelete();
  
  if (!targetFound) {
    log_e("[WiFi] Target SSID '%s' was NOT found in scan!", WIFI_SSID);
    log_e("[WiFi] The network may be:");
    log_e("[WiFi]   - Out of range");
    log_e("[WiFi]   - On 5GHz band only");
    log_e("[WiFi]   - Hidden (not broadcasting SSID)");
    log_e("[WiFi]   - Turned off");
  } else {
    log_e("[WiFi] Target SSID '%s' WAS found - likely a password or auth issue", WIFI_SSID);
  }
  
  log_w("[WiFi] WiFi watchdog will continue attempting reconnection in background");
}

void WebServerManager::setupUniqueHostname() {
  const char* baseHostname = "livegrid";
  String hostname = baseHostname;
  int suffix = 1;

  while (suffix <= 5) {  // Increase max attempts to 5
    if (MDNS.begin(hostname.c_str())) {
      log_i("Hostname set to: %s", hostname.c_str());
      MDNS.addService("http", "tcp", 80);  // Add this line
      return;
    }

    // If setting the hostname fails, increment the suffix and try again
    hostname = String(baseHostname) + String(suffix);
    suffix++;
  }

  log_w("Failed to set unique hostname after 99 attempts. Using default.");
}

void WebServerManager::setupInterface() {
  interface.begin();
  interface.onPower([this](bool state) {
    stateManager->getState()->power = state;
    stateManager->save();
    log_i("Power state changed to: %s", state ? "ON" : "OFF");
  });

  interface.onAutoBrightness([this](bool state) {
    stateManager->getState()->autobrightness = state;
    stateManager->save();
    log_i("Autobrightness state changed to: %s", state ? "ON" : "OFF");
  });

  interface.onBrightness([this](uint8_t value) {
    stateManager->getState()->brightness = value;
    stateManager->save();
    matrix->setBrightness(value);
    log_i("Brightness changed to: %d", value);
    stateManager->save();
  });

  interface.onMode([this](OpenMatrixMode mode) {
    if (mode < 4 && mode != stateManager->getState()->mode) {
      stateManager->getState()->mode = mode;
      stateManager->save();
      log_i("Mode changed to: %d", static_cast<int>(mode));
    }
  });

  interface.onEffect([this](Effects effect) {
    if (effectManager->getCurrentEffect() != (effect - 1) &&
        (effect - 1) <= effectManager->getEffectCount()) {
      effectManager->setEffect(effect - 1);
      stateManager->getState()->effects.selected = effect;
      stateManager->getState()->mode = OpenMatrixMode::EFFECT;
      stateManager->save();
      log_i("Effect changed to: %d", static_cast<int>(effect));
    }
  });
  
  interface.onEffectSettings([this](Effects effect, JsonObject settings) {
      log_i("Updating settings for effect: %d", static_cast<int>(effect));
      // effectManager->updateEffectSettings(effect - 1, settings);
      // stateManager->save();
  });
  
  interface.onImage([this](String fileName) {
    if (imageDraw->openGIF(fileName.c_str())) {
      stateManager->getState()->image.selected = fileName;
      stateManager->getState()->mode = OpenMatrixMode::IMAGE;
      stateManager->save();
      log_i("Image changed to: %s", fileName.c_str());
    } else {
      log_e("Failed to open GIF: %s", fileName.c_str());
    }
  });

  interface.onText([this](String payload, TextSize size) {
    stateManager->getState()->text.payload = payload;
    stateManager->getState()->text.size = size;
    stateManager->getState()->mode = OpenMatrixMode::TEXT;
    stateManager->save();
    log_i("Text changed to: (%d) %s", size, payload.c_str());
  });

  interface.onMqttSettings([this](
    const char* host,
    uint16_t port,
    const char* client_id,
    const char* username,
    const char* password,
    const char* co2_topic,
    const char* matrix_text_topic,
    bool show_text
  ) {
    stateManager->getState()->settings.mqtt.host = host;
    stateManager->getState()->settings.mqtt.port = port;
    stateManager->getState()->settings.mqtt.client_id = client_id;
    stateManager->getState()->settings.mqtt.username = username;
    stateManager->getState()->settings.mqtt.password = password;
    stateManager->getState()->settings.mqtt.co2_topic = co2_topic;
    stateManager->getState()->settings.mqtt.matrix_text_topic = matrix_text_topic;
    stateManager->getState()->settings.mqtt.show_text = show_text;
    
    MQTTManager::getInstance().checkSettingsAndReconnect();
    stateManager->save();

    // TODO: Do something with these MQTT settings     
  });

  interface.onDmxSettings([this](
    eDmxProtocol protocol,
    eDmxMode mode,
    bool multicast,
    uint16_t start_universe,
    uint16_t start_address,
    uint16_t timeout,
    bool udp_enabled,
    uint16_t udp_port
  ) {
    stateManager->getState()->settings.edmx.protocol = protocol;
    stateManager->getState()->settings.edmx.mode = mode;
    stateManager->getState()->settings.edmx.multicast = multicast;
    stateManager->getState()->settings.edmx.start_universe = start_universe;
    stateManager->getState()->settings.edmx.start_address = start_address;
    stateManager->getState()->settings.edmx.timeout = timeout;
    stateManager->getState()->settings.edmx.udp_enabled = udp_enabled;
    stateManager->getState()->settings.edmx.udp_port = udp_port;
    stateManager->save();

    // Apply the new settings to the Edmx instance
    Edmx::getInstance().applySettings();
  });

  interface.onHomeAssistantSettings([this](
    bool show_text
  ) {
    stateManager->getState()->settings.home_assistant.show_text = show_text;
    stateManager->save();

    // TODO: Do something with these Home Assistant settings
  });

  interface.onNetworkReset([this]() {
    log_i("[*] Resetting network");
    ESP.restart();
  });

  interface.onFactoryReset([this]() {
    log_i("[*] Resetting factory settings");
    LittleFS.remove("/aquarium_state.json");
    LittleFS.remove("/state.json");
    ESP.restart();
  });
}

void WebServerManager::startServer() {
  // log_i("[*] Attaching ElegantOTA");
  // ElegantOTA.begin(&server);

  server.begin();

  if (WiFi.status() == WL_CONNECTED) {
    log_i("OpenMatrix is configured!");
  } else {
    log_w("OpenMatrix is not configured yet! Please connect to LiveGrid AP and setup your device.");
  }
}