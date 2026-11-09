#include "MQTTManager.h"
#include <WiFi.h>
#include <ArduinoJson.h>

MQTTManager& MQTTManager::getInstance() {
  static MQTTManager instance;
  return instance;
}

MQTTManager::MQTTManager() {
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(30000), pdTRUE, this, [](TimerHandle_t xTimer) {
    static_cast<MQTTManager*>(pvTimerGetTimerID(xTimer))->connect();
  });
}

MQTTManager::~MQTTManager() {
  xTimerDelete(mqttReconnectTimer, 0);
}

void MQTTManager::begin(const char* host, uint16_t port, StateManager* stateManager) {
  this->stateManager = stateManager;
  mqttClient.setKeepAlive(60);
  setupCallbacks();
  updateSettingsFromState();
  computeTopics();

  // MQTT can only connect once WiFi has an IP. Before that the lwIP stack is
  // not running and AsyncTCP::connect crashes on a null tcpip mailbox.
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
    MQTTManager& mgr = MQTTManager::getInstance();
    if (!mgr.isConnected()) {
      log_i("WiFi got IP, connecting MQTT");
      mgr.connect();
    }
  }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  if (WiFi.isConnected()) {
    connect();
  } else {
    log_i("WiFi not connected yet. MQTT will connect once WiFi has an IP.");
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
  if (!WiFi.isConnected()) {
    log_i("WiFi not connected. Skipping MQTT connection attempt.");
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
  xTimerStop(MQTTManager::getInstance().mqttReconnectTimer, 0);
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

namespace {

// Home Assistant only offers sensors with device_class "temperature" to a
// thermostat. Units must be the real degree sign (U+00B0), not a lookalike.
struct HaSensorDef {
  const char* id;
  const char* name;
  const char* unit;
  const char* deviceClass;
  int precision;
};

const HaSensorDef kHaSensors[] = {
    {"temperature", "Temperature", "\xC2\xB0" "C", "temperature", 1},
    {"humidity", "Humidity", "%", "humidity", 0},
    {"co2", "CO2", "ppm", "carbon_dioxide", 0},
};

String haDeviceName(StateManager* stateManager) {
  const String& name = stateManager->getState()->settings.mqtt.client_id;
  if (!name.isEmpty()) return name;
  return String("Livegrid");
}

void addHaDevice(JsonDocument& doc, StateManager* stateManager, const String& deviceId) {
  JsonObject dev = doc["device"].to<JsonObject>();
  JsonArray ids = dev["identifiers"].to<JsonArray>();
  // Stable hardware id. Keep this first so an existing "Livegrid Device"
  // entry is updated in place instead of forked.
  ids.add(deviceId);

  // Older firmware published a lowercase, unpadded MAC id. Listing it here
  // makes Home Assistant merge that registry entry into this device.
  char legacyId[40];
  snprintf(legacyId, sizeof(legacyId), "livegrid_%x",
           (unsigned)(uint32_t)ESP.getEfuseMac());
  if (deviceId != legacyId) {
    ids.add(legacyId);
  }

  // The name set in the app is also an identifier, so a second empty device
  // registered under that client id is merged into the device that has data.
  const String& clientId = stateManager->getState()->settings.mqtt.client_id;
  if (!clientId.isEmpty() && clientId != deviceId && clientId != legacyId) {
    ids.add(clientId);
  }

  dev["name"] = haDeviceName(stateManager);
  dev["manufacturer"] = "Livegrid.tech";
  dev["model"] = "Livegrid v1.0";

  String mac = WiFi.macAddress();
  mac.toLowerCase();
  if (mac.length() && mac != "00:00:00:00:00:00") {
    JsonArray connections = dev["connections"].to<JsonArray>();
    JsonArray conn = connections.add<JsonArray>();
    conn.add("mac");
    conn.add(mac);
  }

  if (WiFi.isConnected()) {
    dev["configuration_url"] = String("http://") + WiFi.localIP().toString() + "/";
  }
}

void publishDiscovery(MQTTManager& mqtt, const char* topic, JsonDocument& doc) {
  String payload;
  serializeJson(doc, payload);
  if (!payload.length()) {
    log_e("HA discovery JSON empty for %s", topic);
    return;
  }
  mqtt.publish(topic, 0, true, payload.c_str());
  log_i("HA discovery %s (%u bytes)", topic, (unsigned)payload.length());
}

void clearLegacyDiscovery(MQTTManager& mqtt) {
  // Retained configs from older firmware. An empty retained payload removes
  // the entity and, once nothing else references it, the extra device.
  const char* topics[] = {
      "homeassistant/sensor/livegrid/temperature/config",
      "homeassistant/sensor/livegrid/humidity/config",
      "homeassistant/sensor/livegrid/co2/config",
      "homeassistant/sensor/livegrid_temperature/config",
      "homeassistant/sensor/livegrid_humidity/config",
      "homeassistant/sensor/livegrid_co2/config",
      "homeassistant/text/livegrid/matrix_text/config",
  };
  for (const char* topic : topics) {
    mqtt.publish(topic, 0, true, "");
  }
}

}  // namespace

void MQTTManager::publishHomeAssistantConfig() {
  for (const HaSensorDef& sensor : kHaSensors) {
    char discoveryTopic[128];
    snprintf(discoveryTopic, sizeof(discoveryTopic), "homeassistant/sensor/%s_%s/config",
             deviceId.c_str(), sensor.id);

    JsonDocument doc;
    doc["name"] = sensor.name;
    doc["unique_id"] = deviceId + String("_") + sensor.id;
    doc["state_topic"] = sensorsStateTopic;
    doc["device_class"] = sensor.deviceClass;
    doc["state_class"] = "measurement";
    doc["unit_of_measurement"] = sensor.unit;
    doc["suggested_display_precision"] = sensor.precision;
    doc["value_template"] = String("{{ value_json.") + sensor.id + " }}";
    doc["avty_t"] = availabilityTopic;
    addHaDevice(doc, stateManager, deviceId);
    publishDiscovery(*this, discoveryTopic, doc);
  }

  char textDiscoveryTopic[128];
  snprintf(textDiscoveryTopic, sizeof(textDiscoveryTopic), "homeassistant/text/%s/display_text/config",
           deviceId.c_str());

  JsonDocument textDoc;
  textDoc["name"] = "Display text";
  textDoc["unique_id"] = deviceId + String("_matrix_text");
  textDoc["command_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  textDoc["state_topic"] = stateManager->getState()->settings.mqtt.matrix_text_topic;
  textDoc["avty_t"] = availabilityTopic;
  addHaDevice(textDoc, stateManager, deviceId);
  publishDiscovery(*this, textDiscoveryTopic, textDoc);

  char lightTopic[128];
  snprintf(lightTopic, sizeof(lightTopic), "homeassistant/light/%s/light/config", deviceId.c_str());

  JsonDocument lightDoc;
  lightDoc["name"] = "Light";
  lightDoc["unique_id"] = deviceId + String("_light");
  lightDoc["cmd_t"] = lightCommandTopic;
  lightDoc["stat_t"] = lightStateTopic;
  lightDoc["schema"] = "json";
  lightDoc["brightness"] = true;
  lightDoc["avty_t"] = availabilityTopic;
  addHaDevice(lightDoc, stateManager, deviceId);
  publishDiscovery(*this, lightTopic, lightDoc);

  clearLegacyDiscovery(*this);
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

  if (!settingsChanged) {
    return;
  }

  const bool hostSet = !state->settings.mqtt.host.isEmpty() && state->settings.mqtt.host != "0";
  if (mqttClient.connected()) {
    log_i("MQTT settings changed. Reconnecting...");
    pendingImmediateReconnect = hostSet;
    disconnect();
  } else if (hostSet) {
    // Never connected, so no disconnect callback will fire to reconnect.
    log_i("MQTT settings changed. Connecting...");
    connect();
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
  JsonDocument doc;
  doc["name"] = "Auto Brightness";
  doc["unique_id"] = deviceId + String("_autobrightness");
  doc["command_topic"] = String("livegrid/") + deviceId + "/autobrightness/set";
  doc["state_topic"] = String("livegrid/") + deviceId + "/autobrightness/state";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["avty_t"] = availabilityTopic;
  addHaDevice(doc, stateManager, deviceId);
  publishDiscovery(*this, topic, doc);
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