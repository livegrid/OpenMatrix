#pragma once

#include <Arduino.h>
#include <ESPAsyncE131.h>
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
  uint16_t totalPixels;
  bool isRGBMode = true;

  bool fullPacketReceived = false;

  uint8_t remainingBytes = 0;
  uint8_t prevData[2];

  void onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo);

  std::vector<std::string> suspendedTasks;
  bool startupSheddingActive = false;
};