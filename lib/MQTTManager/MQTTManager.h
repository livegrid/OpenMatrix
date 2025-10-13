#pragma once

#include <AsyncMqttClient.h>
#include <StateManager.h>

class MQTTManager {
 public:
  static MQTTManager& getInstance();
  void begin(const char* host, uint16_t port, StateManager* stateManager);
  void connect();
  void disconnect();
  bool isConnected();
  void publish(const char* topic, uint8_t qos, bool retain, const char* payload);
  void subscribe(const char* topic, uint8_t qos);
  void setCallback(std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> callback);
  void publishHomeAssistantConfig();
  void publishAutoBrightnessConfig();
  void publishAutoBrightnessState();
  void publishSensorData(float temperature, float humidity, int co2, const char* topic = nullptr);
  void subscribeToTextTopic();
  void subscribeToLightTopic();
  void subscribeToAutoBrightnessTopic();
  void handleIncomingMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
  void updateSettingsFromState();
  void checkSettingsAndReconnect();
  void publishAvailability(bool online);
  void publishLightState();

 private:
  MQTTManager();
  ~MQTTManager();
  MQTTManager(const MQTTManager&) = delete;
  MQTTManager& operator=(const MQTTManager&) = delete;
  String currentHost;
  uint16_t currentPort = 0;
  String currentClientId;
  String currentUsername;
  String currentPassword;
  String deviceId;
  String availabilityTopic;
  String lightCommandTopic;
  String lightStateTopic;
  String sensorsStateTopic;

  StateManager* stateManager;
  AsyncMqttClient mqttClient;
  TimerHandle_t mqttReconnectTimer;
  std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> messageCallback;
  bool pendingImmediateReconnect = false;

  void setupCallbacks();
  void computeTopics();
  static void onMqttConnect(bool sessionPresent);
  static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
};