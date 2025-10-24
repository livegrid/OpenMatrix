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
  updateSettingsFromState();
  computeTopics();
  
  const State* state = stateManager->getState();
  if (!state->settings.mqtt.host.isEmpty() && state->settings.mqtt.host != "0") {
    connect();
  } else {
    log_i("MQTT host not defined or set to 0. Skipping initial connection.");
  }
}

void MQTTManager::updateSettingsFromState() {
  const State* state = stateManager->getState();
  mqttClient.setServer(state->settings.mqtt.host.c_str(), state->settings.mqtt.port.toInt());
  currentHost = state->settings.mqtt.host;
  currentPort = state->settings.mqtt.port.toInt();
  // Track client credentials to detect changes
  currentClientId = state->settings.mqtt.client_id;
  currentUsername = state->settings.mqtt.username;
  currentPassword = state->settings.mqtt.password;
  computeTopics();
  // Set LWT after topics are computed
  if (availabilityTopic.length()) {
    mqttClient.setWill(availabilityTopic.c_str(), 0, true, "offline");
  }
  
  // Only set optional parameters if they are not empty
  if (!state->settings.mqtt.client_id.isEmpty()) {
    mqttClient.setClientId(state->settings.mqtt.client_id.c_str());
  }
  if (!state->settings.mqtt.username.isEmpty() && !state->settings.mqtt.password.isEmpty()) {
    mqttClient.setCredentials(state->settings.mqtt.username.c_str(), state->settings.mqtt.password.c_str());
  }
}

void MQTTManager::connect() {
  const State* state = stateManager->getState();
  if (state->settings.mqtt.host.isEmpty() || state->settings.mqtt.host == "0") {
    log_i("MQTT host not defined or set to 0. Skipping connection attempt.");
    return;
  }
  
  log_i("Connecting to MQTT...");
  updateSettingsFromState();
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
  // Last Will and Testament for availability
  if (availabilityTopic.length()) {
    mqttClient.setWill(availabilityTopic.c_str(), 0, true, "offline");
  }
}

void MQTTManager::onMqttConnect(bool sessionPresent) {
  log_i("Connected to MQTT. Session present: %d", sessionPresent);
  MQTTManager::getInstance().stateManager->getState()->settings.mqtt.status = CONNECTED;
  MQTTManager::getInstance().stateManager->getState()->settings.home_assistant.status = CONNECTED;
  MQTTManager::getInstance().subscribeToTextTopic();
  MQTTManager::getInstance().subscribeToLightTopic();
  MQTTManager::getInstance().subscribeToAutoBrightnessTopic();
  MQTTManager::getInstance().publishHomeAssistantConfig();
  MQTTManager::getInstance().publishAutoBrightnessConfig();
  MQTTManager::getInstance().publishAvailability(true);
  MQTTManager::getInstance().publishLightState();
  MQTTManager::getInstance().publishAutoBrightnessState();
}

void MQTTManager::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  log_w("Disconnected from MQTT. Reason: %d", static_cast<int>(reason));
  MQTTManager::getInstance().stateManager->getState()->settings.mqtt.status = DISCONNECTED;
  MQTTManager::getInstance().stateManager->getState()->settings.home_assistant.status = DISCONNECTED;
  if (WiFi.isConnected()) {
    if (MQTTManager::getInstance().pendingImmediateReconnect) {
      MQTTManager::getInstance().pendingImmediateReconnect = false;
      // immediate reconnect for settings change
      MQTTManager::getInstance().connect();
    } else {
      xTimerStart(MQTTManager::getInstance().mqttReconnectTimer, 0);
    }
    MQTTManager::getInstance().stateManager->getState()->settings.mqtt.status = RECONNECTING;
    MQTTManager::getInstance().stateManager->getState()->settings.home_assistant.status = RECONNECTING;
  }
}

void MQTTManager::onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  MQTTManager::getInstance().handleIncomingMessage(topic, payload, properties, len, index, total);
}

void MQTTManager::subscribeToTextTopic() {
  // Prefer device-scoped text topic if not provided
  String textTopic = stateManager->getState()->settings.mqtt.matrix_text_topic;
  if (textTopic.isEmpty() || textTopic == "homeassistant/text/livegrid/matrix_text/set") {
    textTopic = String("livegrid/") + deviceId + "/text/set";
    stateManager->getState()->settings.mqtt.matrix_text_topic = textTopic;
  }
  subscribe(textTopic.c_str(), 0);
  log_i("Subscribed to text topic: %s", textTopic.c_str());
}

void MQTTManager::subscribeToLightTopic() {
  if (lightCommandTopic.length()) {
    subscribe(lightCommandTopic.c_str(), 0);
    log_i("Subscribed to light command topic: %s", lightCommandTopic.c_str());
  }
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

  // Light control: expects either raw "ON"/"OFF" or JSON {"state":"ON","brightness":128}
  if (topicStr == lightCommandTopic) {
    bool stateChanged = false;
    bool brightnessChanged = false;
    bool powerOn = stateManager->getState()->power;
    uint8_t newBrightness = stateManager->getState()->brightness;

    if (payloadStr.startsWith("{") && payloadStr.endsWith("}")) {
      DynamicJsonDocument doc(256);
      auto err = deserializeJson(doc, payloadStr);
      if (!err) {
        if (doc.containsKey("state")) {
          String st = doc["state"].as<String>();
          bool newPower = (st.equalsIgnoreCase("ON"));
          if (newPower != powerOn) {
            stateChanged = true;
            powerOn = newPower;
          }
        }
        if (doc.containsKey("brightness")) {
          int b = doc["brightness"].as<int>();
          b = constrain(b, 0, 255);
          if (b != newBrightness) {
            newBrightness = static_cast<uint8_t>(b);
            brightnessChanged = true;
          }
        }
      }
    } else {
      if (payloadStr.equalsIgnoreCase("ON") || payloadStr.equalsIgnoreCase("OFF")) {
        bool newPower = payloadStr.equalsIgnoreCase("ON");
        if (newPower != powerOn) {
          stateChanged = true;
          powerOn = newPower;
        }
      }
    }

    if (stateChanged) {
      stateManager->getState()->power = powerOn;
      stateManager->save();
    }
    if (brightnessChanged) {
      stateManager->getState()->brightness = newBrightness;
      stateManager->save();
    }
    // Brightness is applied by web interface handler; for MQTT ensure device reflects new value
    // Matrix reference not available here; display task reads from state.
    publishLightState();
  }

  // Auto brightness switch
  if (topicStr == (String("livegrid/") + deviceId + "/autobrightness/set")) {
    bool turnOn = payloadStr.equalsIgnoreCase("ON");
    if (stateManager->getState()->autobrightness != turnOn) {
      stateManager->getState()->autobrightness = turnOn;
      stateManager->save();
    }
    publishAutoBrightnessState();
  }

  // Check for settings changes after processing each message
  checkSettingsAndReconnect();
}

void MQTTManager::publishHomeAssistantConfig() {
  const char* sensors[] = {"temperature", "humidity", "co2"};
  const char* units[] = {"°C", "%", "ppm"};
  const char* icons[] = {"mdi:thermometer", "mdi:water-percent", "mdi:molecule-co2"};

  for (int i = 0; i < 3; i++) {
    char discoveryTopic[128];
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/sensor/%s_%s/config", deviceId.c_str(), sensors[i]);

    DynamicJsonDocument doc(512);
    doc["name"] = String("Livegrid ") + sensors[i];
    doc["unique_id"] = deviceId + String("_") + sensors[i];
    doc["state_topic"] = sensorsStateTopic;
    doc["unit_of_measurement"] = units[i];
    doc["value_template"] = String("{{ value_json.") + sensors[i] + " }}";
    doc["icon"] = icons[i];
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0] = deviceId;
    dev["name"] = "Livegrid Device";
    dev["model"] = "Livegrid v1.0";
    dev["manufacturer"] = "Livegrid.tech";

    char payload[512];
    serializeJson(doc, payload);

    publish(discoveryTopic, 0, true, payload); // Ensure retain flag is true
  }

  // Add MQTT text component configuration
  char textDiscoveryTopic[128];
  snprintf(textDiscoveryTopic, sizeof(textDiscoveryTopic), "homeassistant/text/%s/display_text/config", deviceId.c_str());

  DynamicJsonDocument textDoc(512);
  textDoc["name"] = "Matrix Text";
  textDoc["unique_id"] = deviceId + String("_matrix_text");
  textDoc["command_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  textDoc["state_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  JsonObject textDev = textDoc["device"].to<JsonObject>();
  textDev["identifiers"][0] = deviceId;
  textDev["name"] = "Livegrid Device";
  textDev["model"] = "Livegrid v1.0";
  textDev["manufacturer"] = "Livegrid.tech";

  char textPayload[512];
  serializeJson(textDoc, textPayload);

  publish(textDiscoveryTopic, 0, true, textPayload); // Ensure retain flag is true

  // Light entity discovery (on/off + brightness)
  char lightTopic[128];
  snprintf(lightTopic, sizeof(lightTopic), "homeassistant/light/%s/light/config", deviceId.c_str());

  DynamicJsonDocument lightDoc(768);
  lightDoc["name"] = "Livegrid Light";
  lightDoc["unique_id"] = String(deviceId) + "_light";
  lightDoc["cmd_t"] = lightCommandTopic;
  lightDoc["stat_t"] = lightStateTopic;
  lightDoc["schema"] = "json";
  lightDoc["brightness"] = true;
  lightDoc["avty_t"] = availabilityTopic;
  // group the light with sensors/text as a single device
  JsonObject dev = lightDoc["device"].to<JsonObject>();
  dev["identifiers"][0] = deviceId;
  dev["name"] = "Livegrid Device";
  dev["model"] = "Livegrid v1.0";
  dev["manufacturer"] = "Livegrid.tech";

  char lightPayload[512];
  serializeJson(lightDoc, lightPayload);
  publish(lightTopic, 0, true, lightPayload);
}

void MQTTManager::publishSensorData(float temperature, float humidity, int co2, const char* topic) {
  DynamicJsonDocument doc(128);
  
  // Check if values are valid before adding them to the JSON document
  if (!isnan(temperature)) {
    // round to two decimal places using double precision to avoid floating point errors
    doc["temperature"] = static_cast<float>(round(temperature * 100.0) / 100.0);
  }
  if (!isnan(humidity) && humidity >= 0 && humidity <= 100) {
    doc["humidity"] = static_cast<int>(round(humidity));
  }
  if (co2 > 0) {
    doc["co2"] = co2;
  }
  
// Only publish if we have at least one valid value
  if (!doc.isNull()) {
    char payload[128];
    serializeJson(doc, payload);
    const char* selectedTopic = topic ? topic : sensorsStateTopic.c_str();
    publish(selectedTopic, 0, false, payload);
    log_i("Published sensor data: %s", payload);
  } else {
    log_w("No valid sensor data to publish");
  }
}

void MQTTManager::checkSettingsAndReconnect() {
  const State* state = stateManager->getState();
  bool settingsChanged = false;

  // Check changed connection params
  if (currentHost != state->settings.mqtt.host || currentPort != state->settings.mqtt.port.toInt()) {
    settingsChanged = true;
  }
  if (currentClientId != state->settings.mqtt.client_id) {
    settingsChanged = true;
  }
  if (currentUsername != state->settings.mqtt.username) {
    settingsChanged = true;
  }
  // Only compare password if new non-empty value present
  if (!state->settings.mqtt.password.isEmpty() && currentPassword != state->settings.mqtt.password) {
    settingsChanged = true;
  }

  if (settingsChanged) {
    log_i("MQTT host changed. Reconnecting...");
    pendingImmediateReconnect = true;
    disconnect();
  }
}

void MQTTManager::computeTopics() {
  char buf[64];
  snprintf(buf, sizeof(buf), "livegrid_%08X", (uint32_t)ESP.getEfuseMac());
  deviceId = String(buf);
  availabilityTopic = String("livegrid/") + deviceId + "/status";
  lightCommandTopic = String("livegrid/") + deviceId + "/light/set";
  lightStateTopic = String("livegrid/") + deviceId + "/light/state";
  sensorsStateTopic = String("livegrid/") + deviceId + "/sensors";
}

void MQTTManager::publishAutoBrightnessConfig() {
  char topic[128];
  snprintf(topic, sizeof(topic), "homeassistant/switch/%s/autobrightness/config", deviceId.c_str());
  DynamicJsonDocument doc(512);
  doc["name"] = "Auto Brightness";
  doc["unique_id"] = deviceId + String("_autobrightness");
  doc["command_topic"] = String("livegrid/") + deviceId + "/autobrightness/set";
  doc["state_topic"] = String("livegrid/") + deviceId + "/autobrightness/state";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["avty_t"] = availabilityTopic;
  JsonObject dev = doc["device"].to<JsonObject>();
  dev["identifiers"][0] = deviceId;
  dev["name"] = "Livegrid Device";
  dev["model"] = "Livegrid v1.0";
  dev["manufacturer"] = "Livegrid.tech";
  char payload[512];
  serializeJson(doc, payload);
  publish(topic, 0, true, payload);
}

void MQTTManager::publishAutoBrightnessState() {
  String st = stateManager->getState()->autobrightness ? "ON" : "OFF";
  String topic = String("livegrid/") + deviceId + "/autobrightness/state";
  publish(topic.c_str(), 0, true, st.c_str());
}

void MQTTManager::subscribeToAutoBrightnessTopic() {
  String topic = String("livegrid/") + deviceId + "/autobrightness/set";
  subscribe(topic.c_str(), 0);
}

void MQTTManager::publishAvailability(bool online) {
  if (!availabilityTopic.length()) return;
  publish(availabilityTopic.c_str(), 0, true, online ? "online" : "offline");
}

void MQTTManager::publishLightState() {
  DynamicJsonDocument doc(128);
  const bool on = stateManager->getState()->power;
  const uint8_t b = stateManager->getState()->brightness;
  doc["state"] = on ? "ON" : "OFF";
  doc["brightness"] = b;
  char payload[128];
  serializeJson(doc, payload);
  publish(lightStateTopic.c_str(), 0, true, payload);
}