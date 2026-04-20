#include "MQTTManager.h"

#include <ArduinoJson.h>
#include <WiFi.h>

MQTTManager& MQTTManager::getInstance() {
  static MQTTManager instance;
  return instance;
}

MQTTManager::MQTTManager() {}
MQTTManager::~MQTTManager() {}

void MQTTManager::begin(StateManager* stateManager) {
  stateManager_ = stateManager;
  setupCallbacks();

  // Arm immediately if we already have usable settings; the first loop()
  // call will attempt the actual connection when WiFi is up.
  if (haveValidSettings()) {
    phase_ = ARMED;
    armedSinceMs_ = millis();
    lastAttemptMs_ = 0;
    log_i("MQTT: Armed with host=%s:%s",
          stateManager_->getState()->settings.mqtt.host.c_str(),
          stateManager_->getState()->settings.mqtt.port.c_str());
  } else {
    phase_ = IDLE;
    log_i("MQTT: No host configured. Staying idle.");
  }
}

bool MQTTManager::haveValidSettings() const {
  if (stateManager_ == nullptr) return false;
  const auto& mqtt = stateManager_->getState()->settings.mqtt;
  if (mqtt.host.isEmpty()) return false;
  if (mqtt.host == "0") return false;
  if (mqtt.port.isEmpty() || mqtt.port.toInt() <= 0) return false;
  return true;
}

void MQTTManager::applySettingsToClient() {
  if (stateManager_ == nullptr) return;
  const auto& mqtt = stateManager_->getState()->settings.mqtt;
  mqttClient_.setServer(mqtt.host.c_str(), (uint16_t)mqtt.port.toInt());

  if (!mqtt.client_id.isEmpty()) {
    mqttClient_.setClientId(mqtt.client_id.c_str());
  }
  if (!mqtt.username.isEmpty() && !mqtt.password.isEmpty()) {
    mqttClient_.setCredentials(mqtt.username.c_str(), mqtt.password.c_str());
  }
}

void MQTTManager::setupCallbacks() {
  mqttClient_.onConnect(onMqttConnect);
  mqttClient_.onDisconnect(onMqttDisconnect);
  mqttClient_.onMessage(onMqttMessage);
}

void MQTTManager::loop(float temperature, float humidity, int co2) {
  if (stateManager_ == nullptr) return;

  // Settings might have been cleared from UI - check every tick.
  const bool valid = haveValidSettings();
  if (!valid) {
    if (mqttClient_.connected()) mqttClient_.disconnect();
    phase_ = IDLE;
    discoveryPublished_ = false;
    return;
  }

  // Transitioning from IDLE now that settings exist.
  if (phase_ == IDLE) {
    phase_ = ARMED;
    armedSinceMs_ = millis();
    lastAttemptMs_ = 0;
    log_i("MQTT: Became armed after settings change");
  }

  // Need WiFi before we can do anything network-facing.
  if (!WiFi.isConnected()) {
    log_v("MQTT: Waiting for WiFi");
    return;
  }

  // Connected? Publish and return.
  if (mqttClient_.connected()) {
    if (phase_ != CONNECTED) {
      phase_ = CONNECTED;
    }
    if (!discoveryPublished_) {
      publishHomeAssistantConfig();
      discoveryPublished_ = true;
      log_i("MQTT: Published Home Assistant discovery config");
    }

    publishSensorData(temperature, humidity, co2);
    publishSensorData(
        temperature, humidity, co2,
        stateManager_->getState()->settings.mqtt.co2_topic.c_str());
    return;
  }

  // Not connected. Already gave up?
  if (phase_ == GAVE_UP) {
    return;
  }

  // Retry window.
  const uint32_t now = millis();
  if (now - armedSinceMs_ > RETRY_WINDOW_MS) {
    log_w("MQTT: Gave up connecting after %u s. Will retry only after "
          "settings update or reboot.",
          (unsigned)(RETRY_WINDOW_MS / 1000));
    phase_ = GAVE_UP;
    stateManager_->getState()->settings.mqtt.status = DISCONNECTED;
    stateManager_->getState()->settings.home_assistant.status = DISCONNECTED;
    return;
  }

  // Cooldown between attempts.
  if (lastAttemptMs_ != 0 && (now - lastAttemptMs_) < RETRY_INTERVAL_MS) {
    return;
  }

  const uint32_t remainingMs = RETRY_WINDOW_MS - (now - armedSinceMs_);
  log_i("MQTT: Attempting connection (window: %u s left)...",
        (unsigned)(remainingMs / 1000));
  applySettingsToClient();
  mqttClient_.connect();
  lastAttemptMs_ = now;
  stateManager_->getState()->settings.mqtt.status = RECONNECTING;
  stateManager_->getState()->settings.home_assistant.status = RECONNECTING;
}

void MQTTManager::onSettingsUpdated() {
  log_i("MQTT: Settings update notification received");
  discoveryPublished_ = false;

  if (mqttClient_.connected()) {
    mqttClient_.disconnect();
  }

  if (!haveValidSettings()) {
    phase_ = IDLE;
    log_i("MQTT: No usable settings after update; staying idle.");
    return;
  }

  phase_ = ARMED;
  armedSinceMs_ = millis();
  lastAttemptMs_ = 0;
  log_i("MQTT: Re-armed (retry window reset)");
}

bool MQTTManager::isConnected() {
  return mqttClient_.connected();
}

void MQTTManager::publish(const char* topic, uint8_t qos, bool retain,
                          const char* payload) {
  mqttClient_.publish(topic, qos, retain, payload);
}

void MQTTManager::subscribe(const char* topic, uint8_t qos) {
  mqttClient_.subscribe(topic, qos);
}

void MQTTManager::setCallback(
    std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t,
                       size_t, size_t)>
        callback) {
  messageCallback_ = callback;
}

void MQTTManager::onMqttConnect(bool sessionPresent) {
  log_i("MQTT: Connected (session present: %d)", sessionPresent);
  auto& self = MQTTManager::getInstance();
  self.phase_ = CONNECTED;
  if (self.stateManager_) {
    self.stateManager_->getState()->settings.mqtt.status = ::CONNECTED;
    self.stateManager_->getState()->settings.home_assistant.status = ::CONNECTED;
  }
  self.subscribeToTextTopic();
}

void MQTTManager::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  log_w("MQTT: Disconnected (reason: %d)", static_cast<int>(reason));
  auto& self = MQTTManager::getInstance();
  if (self.stateManager_) {
    self.stateManager_->getState()->settings.mqtt.status = DISCONNECTED;
    self.stateManager_->getState()->settings.home_assistant.status = DISCONNECTED;
  }
  // Do not schedule a reconnect here - loop() handles retry cadence so the
  // retry window is respected consistently.
  self.discoveryPublished_ = false;
  if (self.phase_ == CONNECTED) {
    // Treat unexpected disconnect as a new retry window.
    self.phase_ = ARMED;
    self.armedSinceMs_ = millis();
    self.lastAttemptMs_ = 0;
  }
}

void MQTTManager::onMqttMessage(char* topic, char* payload,
                                AsyncMqttClientMessageProperties properties,
                                size_t len, size_t index, size_t total) {
  MQTTManager::getInstance().handleIncomingMessage(topic, payload, properties,
                                                   len, index, total);
}

void MQTTManager::subscribeToTextTopic() {
  if (stateManager_ == nullptr) return;
  const char* textTopic =
      stateManager_->getState()->settings.mqtt.matrix_text_topic.c_str();
  subscribe(textTopic, 0);
  log_i("MQTT: Subscribed to text topic: %s", textTopic);
}

void MQTTManager::handleIncomingMessage(
    char* topic, char* payload,
    AsyncMqttClientMessageProperties /*properties*/, size_t len,
    size_t /*index*/, size_t /*total*/) {
  String topicStr = String(topic);
  String payloadStr = String(payload, len);
  log_i("MQTT: Received on %s: %s", topicStr.c_str(), payloadStr.c_str());
  if (stateManager_ == nullptr) return;
  if (topicStr == stateManager_->getState()->settings.mqtt.matrix_text_topic) {
    stateManager_->getState()->text.payload = payloadStr;
    log_i("MQTT: Updated text payload");
  }
}

void MQTTManager::publishHomeAssistantConfig() {
  if (stateManager_ == nullptr) return;

  const char* sensors[] = {"temperature", "humidity", "co2"};
  const char* units[]   = {"°C", "%", "ppm"};
  const char* icons[]   = {"mdi:thermometer", "mdi:water-percent", "mdi:molecule-co2"};

  for (int i = 0; i < 3; i++) {
    char discoveryTopic[128];
    snprintf(discoveryTopic, sizeof(discoveryTopic),
             "homeassistant/sensor/livegrid_%s/config", sensors[i]);

    JsonDocument doc;
    doc["name"] = String("Livegrid ") + sensors[i];
    doc["unique_id"] = String("livegrid_") +
                       String((uint32_t)ESP.getEfuseMac(), HEX) + "_" +
                       sensors[i];
    doc["state_topic"] = "homeassistant/sensor/livegrid/state";
    doc["unit_of_measurement"] = units[i];
    doc["value_template"] = String("{{ value_json.") + sensors[i] + " }}";
    doc["icon"] = icons[i];
    doc["device"]["identifiers"][0] =
        String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX);
    doc["device"]["name"] = "Livegrid Sensor";
    doc["device"]["model"] = "Livegrid v1.0";
    doc["device"]["manufacturer"] = "Livegrid.tech";

    char payload[512];
    serializeJson(doc, payload);
    publish(discoveryTopic, 0, true, payload);
  }

  char textDiscoveryTopic[128];
  snprintf(textDiscoveryTopic, sizeof(textDiscoveryTopic),
           "homeassistant/text/livegrid/matrix_text/config");

  JsonDocument textDoc;
  textDoc["name"] = "Matrix Text";
  textDoc["unique_id"] = String("livegrid_") +
                         String((uint32_t)ESP.getEfuseMac(), HEX) +
                         "_matrix_text";
  textDoc["command_topic"] =
      stateManager_->getState()->settings.mqtt.matrix_text_topic;
  textDoc["state_topic"] =
      stateManager_->getState()->settings.mqtt.matrix_text_topic;
  textDoc["device"]["identifiers"][0] =
      String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX);
  textDoc["device"]["name"] = "Livegrid Device";
  textDoc["device"]["model"] = "Livegrid v1.0";
  textDoc["device"]["manufacturer"] = "Livegrid.tech";

  char textPayload[512];
  serializeJson(textDoc, textPayload);
  publish(textDiscoveryTopic, 0, true, textPayload);
}

void MQTTManager::publishSensorData(float temperature, float humidity, int co2,
                                    const char* topic) {
  JsonDocument doc;
  if (!isnan(temperature)) {
    doc["temperature"] = String(temperature, 2);
  }
  if (!isnan(humidity) && humidity >= 0 && humidity <= 100) {
    doc["humidity"] = (int)round(humidity);
  }
  if (co2 > 0) {
    doc["co2"] = co2;
  }

  if (!doc.isNull()) {
    char payload[128];
    serializeJson(doc, payload);
    publish(topic, 0, false, payload);
    log_v("MQTT: Published %s = %s", topic, payload);
  }
}
