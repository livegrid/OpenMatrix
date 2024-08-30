#include "MQTTManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>

MQTTManager& MQTTManager::getInstance() {
  static MQTTManager instance;
  return instance;
}

MQTTManager::MQTTManager() {
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(60000), pdFALSE, this, [](TimerHandle_t xTimer) {
    static_cast<MQTTManager*>(pvTimerGetTimerID(xTimer))->connect();
  });
}

MQTTManager::~MQTTManager() {
  xTimerDelete(mqttReconnectTimer, 0);
}

void MQTTManager::begin(const char* host, uint16_t port, StateManager* stateManager) {
  this->stateManager = stateManager;
  setupCallbacks();
  mqttClient.setServer(host, port);
  connect();
}

void MQTTManager::connect() {
  log_i("Connecting to MQTT...");
  mqttClient.connect();
}

void MQTTManager::disconnect() {
  mqttClient.disconnect();
}

bool MQTTManager::isConnected() {
  return mqttClient.connected();
}

void MQTTManager::publish(const char* topic, uint8_t qos, bool retain, const char* payload) {
  mqttClient.publish(topic, qos, retain, payload);
}

void MQTTManager::subscribe(const char* topic, uint8_t qos) {
  mqttClient.subscribe(topic, qos);
}

void MQTTManager::setCallback(std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> callback) {
  messageCallback = callback;
}

void MQTTManager::setupCallbacks() {
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onMessage(onMqttMessage);
}

void MQTTManager::onMqttConnect(bool sessionPresent) {
  log_i("Connected to MQTT. Session present: %d", sessionPresent);
  MQTTManager::getInstance().subscribeToTextTopic();
}

void MQTTManager::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  log_w("Disconnected from MQTT. Reason: %d", static_cast<int>(reason));
  if (WiFi.isConnected()) {
    xTimerStart(MQTTManager::getInstance().mqttReconnectTimer, 0);
  }
}

void MQTTManager::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  MQTTManager::getInstance().handleIncomingMessage(topic, payload, properties, len, index, total);
}

void MQTTManager::subscribeToTextTopic() {
  const char* textTopic = stateManager->getState()->settings.mqtt.matrix_text_topic.c_str();
  subscribe(textTopic, 0);
  log_i("Subscribed to text topic: %s", textTopic);
}

void MQTTManager::handleIncomingMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  String topicStr = String(topic);
  String payloadStr = String(payload, len);

  log_i("Received message on topic: %s", topicStr.c_str());
  log_i("Payload: %s", payloadStr.c_str());

  if (topicStr == stateManager->getState()->settings.mqtt.matrix_text_topic) {
    stateManager->getState()->text.payload = payloadStr;
    log_i("Updated text payload: %s", payloadStr.c_str());
  }
}

void MQTTManager::publishHomeAssistantConfig() {
  const char* sensors[] = {"temperature", "humidity", "co2"};
  const char* units[] = {"°C", "%", "ppm"};

  for (int i = 0; i < 3; i++) {
    char discoveryTopic[128];
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/sensor/livegrid/%s/config", sensors[i]);

    DynamicJsonDocument doc(256);
    doc["name"] = sensors[i];
    doc["unique_id"] = String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX) + "_" + sensors[i];
    doc["state_topic"] = "homeassistant/sensor/livegrid/state";
    doc["unit_of_measurement"] = units[i];
    doc["value_template"] = String("{{ value_json.") + sensors[i] + " }}";
    doc["device"]["identifiers"][0] = String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX);
    doc["device"]["name"] = "Livegrid Sensor";
    doc["device"]["model"] = "Livegrid v1.0";
    doc["device"]["manufacturer"] = "Livegrid.tech";

    char payload[256];
    serializeJson(doc, payload);

    publish(discoveryTopic, 0, true, payload);
  }

  // Add MQTT text component configuration
  char textDiscoveryTopic[128];
  snprintf(textDiscoveryTopic, sizeof(textDiscoveryTopic), "homeassistant/text/livegrid/matrix_text/config");

  DynamicJsonDocument textDoc(512);
  textDoc["name"] = "Matrix Text";
  textDoc["unique_id"] = String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX) + "_matrix_text";
  textDoc["command_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  textDoc["state_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  textDoc["device"]["identifiers"][0] = String("livegrid_") + String((uint32_t)ESP.getEfuseMac(), HEX);
  textDoc["device"]["name"] = "Livegrid Device";
  textDoc["device"]["model"] = "Livegrid v1.0";
  textDoc["device"]["manufacturer"] = "Livegrid.tech";

  char textPayload[512];
  serializeJson(textDoc, textPayload);

  publish(textDiscoveryTopic, 0, true, textPayload);
}

void MQTTManager::publishSensorData(float temperature, float humidity, int co2) {
  DynamicJsonDocument doc(128);
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["co2"] = co2;

  char payload[128];
  serializeJson(doc, payload);

  publish("homeassistant/sensor/livegrid/state", 0, false, payload);
}