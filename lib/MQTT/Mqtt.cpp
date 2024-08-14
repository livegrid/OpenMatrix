#include "Mqtt.h"

MQTT::MQTT(const char* host, uint16_t port) : mqttHost(host), mqttPort(port) {
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(15000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
}

void MQTT::start() {
    mqttClient.setServer(mqttHost, mqttPort);
    connectToMqtt();
}

void MQTT::stop() {
    xTimerStop(mqttReconnectTimer, 0);
    mqttClient.disconnect();
}

void MQTT::setCredentials(const char* username, const char* password) {
    mqttClient.setCredentials(username, password);
}

void MQTT::setWill(const char* topic, uint8_t qos, bool retain, const char* payload) {
    mqttClient.setWill(topic, qos, retain, payload);
}

void MQTT::onConnect(AsyncMqttClientInternals::OnConnectUserCallback callback) {
    mqttClient.onConnect(callback);
}

void MQTT::onDisconnect(AsyncMqttClientInternals::OnDisconnectUserCallback callback) {
    mqttClient.onDisconnect(callback);
}

void MQTT::onSubscribe(AsyncMqttClientInternals::OnSubscribeUserCallback callback) {
    mqttClient.onSubscribe(callback);
}

void MQTT::onUnsubscribe(AsyncMqttClientInternals::OnUnsubscribeUserCallback callback) {
    mqttClient.onUnsubscribe(callback);
}

void MQTT::onMessage(AsyncMqttClientInternals::OnMessageUserCallback callback) {
    mqttClient.onMessage(callback);
}

void MQTT::onPublish(AsyncMqttClientInternals::OnPublishUserCallback callback) {
    mqttClient.onPublish(callback);
}

bool MQTT::publish(const char* topic, uint8_t qos, bool retain, const char* payload) {
    return mqttClient.publish(topic, qos, retain, payload) != 0;
}

uint16_t MQTT::subscribe(const char* topic, uint8_t qos) {
    return mqttClient.subscribe(topic, qos);
}

uint16_t MQTT::unsubscribe(const char* topic) {
    return mqttClient.unsubscribe(topic);
}

void MQTT::connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void MQTT::onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
}

void MQTT::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");
    if (WiFi.isConnected()) {
        xTimerStart(mqttReconnectTimer, 0);
    }
}
