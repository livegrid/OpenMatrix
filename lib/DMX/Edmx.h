#pragma once

#include <Arduino.h>
#include <ESPAsyncE131.h>
#include <AsyncUDP.h>
#include <Matrix.h>
#include <StateManager.h>
#include <GFX_Lite.h>
#include <vector>
#include <string>

class Edmx {
 public:
  static Edmx& getInstance();
  bool begin(Matrix* matrix, StateManager* stateManager);
  void update();
  void setRGBMode(bool rgbMode);
  bool getRGBMode() const;
  bool applySettings();

 private:
  Edmx() {}
  ~Edmx() {}
  Edmx(const Edmx&) = delete;
  Edmx& operator=(const Edmx&) = delete;
  bool startE131(); 
  void prepareForStart();
  void restoreSuspendedTasks();
  bool startUdp();
  void stopUdp();
  void handleUdpPacket(AsyncUDPPacket packet);
  bool processRawPayload(const uint8_t* payload, size_t length, uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  bool processRlePayload(const uint8_t* payload, size_t length, uint16_t x, uint16_t y, uint16_t width, uint16_t height);

  Matrix* matrix;
  StateManager* stateManager;
  ESPAsyncE131 _e131;
  AsyncUDP _udp;
  uint8_t numUniverses;
  const uint16_t channelsPerUniverse = 512;
  uint8_t colorToSet[3];
  bool newPacket = false;
  unsigned long lastPacketReceived;
  unsigned long packetDelay = 5000;
  OpenMatrixMode prevMode;
  uint16_t totalPixels;
  bool isRGBMode = true;

  bool fullPacketReceived = false;

  uint8_t remainingBytes = 0;
  uint8_t prevData[2];

  void onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo);

  std::vector<std::string> suspendedTasks;
  bool startupSheddingActive = false;
  bool udpListening = false;
  uint16_t currentUdpPort = 0;
  static constexpr uint8_t kUdpFlagCompressed = 0x01;
  static constexpr uint8_t kUdpMagic0 = 'O';
  static constexpr uint8_t kUdpMagic1 = 'M';
  static constexpr uint8_t kUdpVersion = 1;
  static constexpr size_t kUdpHeaderSize = 18;
};