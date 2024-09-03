#pragma once

#include <Arduino.h>
#include <ESPAsyncE131.h>
#include <Matrix.h>
#include <StateManager.h>

class Edmx {
 public:
  static Edmx& getInstance();
  void begin(Matrix* matrix, StateManager* stateManager);
  void update();
  void setRGBMode(bool rgbMode);
  bool getRGBMode() const;
  void applySettings();

 private:
  Edmx() {}
  ~Edmx() {}
  Edmx(const Edmx&) = delete;
  Edmx& operator=(const Edmx&) = delete;

  Matrix* matrix;
  StateManager* stateManager;
  ESPAsyncE131 _e131;
  uint8_t numUniverses;
  const uint16_t channelsPerUniverse = 512;
  uint8_t colorToSet[3];
  bool newPacket = false;
  unsigned long lastPacketReceived;
  unsigned long packetDelay = 5000;
  OpenMatrixMode prevMode;
  uint8_t* rawDataBuffer = nullptr;
  uint16_t totalPixels;
  bool isRGBMode = true;

  bool fullPacketReceived = false;

  uint8_t remainingBytes = 0;
  uint8_t prevData[2];

  void onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo);
};