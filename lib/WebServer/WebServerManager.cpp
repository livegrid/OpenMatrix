#include "WebServerManager.h"

WebServerManager::WebServerManager(Matrix* matrix, EffectManager* effectManager,
                                   ImageDraw* imageDraw, StateManager* stateManager,
                                   TaskManager* taskManager)
    : matrix(matrix),
      effectManager(effectManager),
      imageDraw(imageDraw),
      server(80),
      nw(&server),
      interface(&server, stateManager),
      stateManager(stateManager),
      taskManager(taskManager) {}

void WebServerManager::begin() {
  setupNetWizard();
  setupInterface();
  startServer();
  setupUniqueHostname();
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
    log_i("[NW] Connection status changed to: %s", status_str.c_str());
    if (status == NetWizardConnectionStatus::CONNECTED) {
      log_i("[NW] Local IP: %s", WiFi.localIP().toString().c_str());
      log_i("[NW] Gateway IP: %s", WiFi.gatewayIP().toString().c_str());
      log_i("[NW] Subnet mask: %s", WiFi.subnetMask().toString().c_str());
    }
  });

  nw.onPortalState([](NetWizardPortalState state) {
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
    log_i("[NW] Portal state changed to: %s", state_str.c_str());
  });

  log_i("[*] Starting NetWizard");
  nw.autoConnect("LiveGrid", "");
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
    uint16_t timeout
  ) {
    stateManager->getState()->settings.edmx.protocol = protocol;
    stateManager->getState()->settings.edmx.mode = mode;
    stateManager->getState()->settings.edmx.multicast = multicast;
    stateManager->getState()->settings.edmx.start_universe = start_universe;
    stateManager->getState()->settings.edmx.start_address = start_address;
    stateManager->getState()->settings.edmx.timeout = timeout;
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
    nw.reset();
    ESP.restart();
  });

  interface.onFactoryReset([this]() {
    log_i("[*] Resetting factory settings");
    nw.reset();
    LittleFS.remove("/aquarium_state.json");
    LittleFS.remove("/state.json");
    ESP.restart();
  });
}

void WebServerManager::startServer() {
  log_i("[*] Attaching ElegantOTA");
  ElegantOTA.begin(&server);

  server.begin();

  if (nw.isConfigured()) {
    log_i("OpenMatrix is configured!");
  } else {
    log_w("OpenMatrix is not configured yet! Please connect to LiveGrid AP and setup your device.");
  }
}