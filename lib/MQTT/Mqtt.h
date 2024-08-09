#pragma once

#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

class MQTT {
public:
    MQTT(const char* host, uint16_t port);
    void start();
    void stop();
    void setCredentials(const char* username, const char* password);
    void setWill(const char* topic, uint8_t qos, bool retain, const char* payload);
    void onConnect(AsyncMqttClientInternals::OnConnectUserCallback callback);
    void onDisconnect(AsyncMqttClientInternals::OnDisconnectUserCallback callback);
    void onSubscribe(AsyncMqttClientInternals::OnSubscribeUserCallback callback);
    void onUnsubscribe(AsyncMqttClientInternals::OnUnsubscribeUserCallback callback);
    void onMessage(AsyncMqttClientInternals::OnMessageUserCallback callback);
    void onPublish(AsyncMqttClientInternals::OnPublishUserCallback callback);
    bool publish(const char* topic, uint8_t qos, bool retain, const char* payload);
    uint16_t subscribe(const char* topic, uint8_t qos);
    uint16_t unsubscribe(const char* topic);

private:
    AsyncMqttClient mqttClient;
    TimerHandle_t mqttReconnectTimer;
    const char* mqttHost;
    uint16_t mqttPort;

    void connectToMqtt();
    static void onMqttConnect(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
};
