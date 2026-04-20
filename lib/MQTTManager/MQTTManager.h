#pragma once

#include <AsyncMqttClient.h>
#include <StateManager.h>

/**
 * MQTTManager
 *
 * Connection lifecycle
 * --------------------
 * States:
 *   IDLE      - no usable settings (empty host or host == "0"). Does nothing.
 *   ARMED     - have settings, want to connect. Keeps retrying until the
 *               retry window closes or a connection succeeds.
 *   CONNECTED - connected; publishes sensor data periodically.
 *   GAVE_UP   - retry window exhausted without success. Stays silent until
 *               settings are updated from the web UI (onSettingsUpdated) or
 *               the device reboots.
 *
 * The retry window is 10 minutes, with an attempt every 30 s. These are
 * tuned to be polite to the broker and to stop hogging the network stack
 * indefinitely when the broker is unreachable.
 */
class MQTTManager {
 public:
  enum Phase { IDLE, ARMED, CONNECTED, GAVE_UP };

  static MQTTManager& getInstance();

  // Register the state manager. Safe to call before WiFi is up.
  void begin(StateManager* stateManager);

  // Called periodically (e.g. every 5 s) from mqttTask. Drives the
  // connection state machine and publishes sensor data when connected.
  void loop(float temperature, float humidity, int co2);

  // Called by WebServerManager whenever the MQTT settings have been
  // modified from the UI. Resets the retry window.
  void onSettingsUpdated();

  bool isConnected();
  Phase phase() const { return phase_; }

  void publish(const char* topic, uint8_t qos, bool retain, const char* payload);
  void subscribe(const char* topic, uint8_t qos);
  void setCallback(std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> callback);
  void publishHomeAssistantConfig();
  void publishSensorData(float temperature, float humidity, int co2,
                         const char* topic = "homeassistant/sensor/livegrid/state");
  void subscribeToTextTopic();
  void handleIncomingMessage(char* topic, char* payload,
                             AsyncMqttClientMessageProperties properties,
                             size_t len, size_t index, size_t total);

 private:
  MQTTManager();
  ~MQTTManager();
  MQTTManager(const MQTTManager&) = delete;
  MQTTManager& operator=(const MQTTManager&) = delete;

  // Retry behaviour.
  static constexpr uint32_t RETRY_WINDOW_MS   = 10UL * 60UL * 1000UL;  // 10 min
  static constexpr uint32_t RETRY_INTERVAL_MS = 30UL * 1000UL;         // 30 s

  void setupCallbacks();
  void applySettingsToClient();
  bool haveValidSettings() const;

  static void onMqttConnect(bool sessionPresent);
  static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  static void onMqttMessage(char* topic, char* payload,
                            AsyncMqttClientMessageProperties properties,
                            size_t len, size_t index, size_t total);

  StateManager* stateManager_ = nullptr;
  AsyncMqttClient mqttClient_;
  std::function<void(char*, char*, AsyncMqttClientMessageProperties, size_t, size_t, size_t)> messageCallback_;

  Phase phase_ = IDLE;
  uint32_t armedSinceMs_ = 0;
  uint32_t lastAttemptMs_ = 0;
  bool discoveryPublished_ = false;
};
