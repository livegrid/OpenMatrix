#include "WebServerManager.h"

WebServerManager::WebServerManager(Matrix* matrix, EffectManager* effectManager,
                                   ImageDraw* imageDraw, StateManager* stateManager,
                                   TaskManager* taskManager)
    : matrix(matrix),
      effectManager(effectManager),
      imageDraw(imageDraw),
      server(80),
      interface(&server, stateManager),
      stateManager(stateManager),
      taskManager(taskManager) {}

void WebServerManager::begin() {
  connectToWiFi();
  setupInterface();
  startServer();
  setupUniqueHostname();
}

void WebServerManager::handleClient() {
  server.handleClient();
  // ElegantOTA.loop();
}

void WebServerManager::connectToWiFi() {
  log_i("[*] Connecting to WiFi");
  log_i("[WiFi] SSID: '%s'", WIFI_SSID);
  log_i("[WiFi] Password length: %d", strlen(WIFI_PASSWORD));
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Clear any previous connection attempts
  
  // Set hostname before connecting (helps with some routers)
  // WiFi.setHostname("livegrid");
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection with longer timeout (30 seconds)
  int attempts = 0;
  const int maxAttempts = 60; // 60 * 500ms = 30 seconds
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    if (attempts % 10 == 0) { // Log every 5 seconds
      wl_status_t status = WiFi.status();
      log_i("[WiFi] Attempt %d/%d, Status: %d", attempts, maxAttempts, status);
    } else {
      log_i(".");
    }
    attempts++;
  }

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    log_i("[WiFi] Connected successfully");
    log_i("[WiFi] IP address: %s", WiFi.localIP().toString().c_str());
    log_i("[WiFi] Gateway IP: %s", WiFi.gatewayIP().toString().c_str());
    log_i("[WiFi] Subnet mask: %s", WiFi.subnetMask().toString().c_str());
    log_i("[WiFi] RSSI: %d dBm", WiFi.RSSI());
  } else {
    log_e("[WiFi] Failed to connect after %d attempts", attempts);
    log_e("[WiFi] Final status code: %d", status);
    switch(status) {
      case WL_IDLE_STATUS:
        log_e("[WiFi] Status: WL_IDLE_STATUS - WiFi is in process of changing between states");
        break;
      case WL_NO_SSID_AVAIL:
        log_e("[WiFi] Status: WL_NO_SSID_AVAIL - SSID cannot be reached");
        break;
      case WL_SCAN_COMPLETED:
        log_e("[WiFi] Status: WL_SCAN_COMPLETED - Scan completed, no SSID found");
        break;
      case WL_CONNECT_FAILED:
        log_e("[WiFi] Status: WL_CONNECT_FAILED - Connection failed (wrong password?)");
        break;
      case WL_CONNECTION_LOST:
        log_e("[WiFi] Status: WL_CONNECTION_LOST - Connection lost");
        break;
      case WL_DISCONNECTED:
        log_e("[WiFi] Status: WL_DISCONNECTED - Disconnected");
        break;
      default:
        log_e("[WiFi] Status: Unknown (%d)", status);
        break;
    }
  }
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