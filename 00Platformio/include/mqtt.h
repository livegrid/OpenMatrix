#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <common.h>

extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}

#define MQTT_BUFMAX 200

class Mqtt {
  private:
  AsyncMqttClient mqttClient;
  TimerHandle_t mqttReconnectTimer;

  char keepalivetopics[256];
  DynamicJsonDocument value(1024);
  char clientMQTT[64];

  const size_t MAX_TOPICS;
  const size_t MAX_TOPIC_LENGTH;
  char mqttTopics[][];

  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  void onMqttSubscribe(uint16_t packetId, uint8_t qos);
  void onMqttUnsubscribe(uint16_t packetId);
  void onMqttPublish(uint16_t packetId);
  void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t length, size_t index, size_t total);
  void connectToMqtt();

  public:
  void startMQTT();
  void keepalive();
  
  // void debugMessage(char*);
  // void debugMessage(const char*);
  // void debugMessage(const char[], int);
};

#endif