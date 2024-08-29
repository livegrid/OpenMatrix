#pragma once

#include <Arduino.h>
#include <ESPAsyncE131.h>
#include <Matrix.h>
#include <StateManager.h>

class Edmx {
 private:
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
  uint8_t* rawDataBuffer;
  uint16_t totalPixels;
  bool isRGBMode = true;  // New member to determine if we're in RGB or monochrome mode

  bool fullPacketReceived = false;

  // Global variables to keep track of the remaining bytes and their values
  uint8_t remainingBytes = 0;
  uint8_t prevData[2];  // Maximum 2 bytes can be left from previous universe

    void onNewPacketReceived(void* packet, protocol_t protocol, void*
    userInfo) {
      if (protocol != PROTOCOL_E131 && protocol != PROTOCOL_ARTNET) {
        return;  // Ignore non-E131 packets
      }

      e131_packet_t* e131Packet = reinterpret_cast<e131_packet_t*>(packet);
      if (!e131Packet) {
        return;  // Invalid packets
      }

      newPacket = true;
      uint16_t universe = htons(e131Packet->universe);
      uint16_t offset = (universe - 1) * channelsPerUniverse;

      memcpy(rawDataBuffer + offset, e131Packet->property_values + 1,
             channelsPerUniverse);

      if (stateManager->getState()->mode != OpenMatrixMode::DMX) {
        prevMode = stateManager->getState()->mode;
        stateManager->getState()->mode = OpenMatrixMode::DMX;
      }

      lastPacketReceived = millis();
    }

 public:
  Edmx(Matrix* matrix, StateManager* stateManager)
      : matrix(matrix), stateManager(stateManager) {
    totalPixels = matrix->getXResolution() * matrix->getYResolution();
    numUniverses = (totalPixels * (isRGBMode ? 3 : 1) + channelsPerUniverse - 1) / channelsPerUniverse;
    rawDataBuffer = new uint8_t[totalPixels * (isRGBMode ? 3 : 1) + channelsPerUniverse - 1];

    _e131.registerCallback(
        [this](void* packet, protocol_t protocol, void* userInfo) {
          this->onNewPacketReceived(packet, protocol, userInfo);
        });
  }
  ~Edmx() {
    delete[] rawDataBuffer;
  }

  void begin() {
    // Listen via Unicast
    if (_e131.begin(E131_MULTICAST, 1, numUniverses, PROTOCOL_E131)) {
      log_i("Listening for data...");
    } else {
      log_e("*** e131.begin failed ***");
    }
  }

  void update() {
    if(millis() - lastPacketReceived > packetDelay) {
        newPacket = false;
      stateManager->getState()->mode = prevMode;
    }
    uint16_t k = 0;
    for (uint16_t j = 0; j < matrix->getYResolution(); j++) {
      for (uint16_t i = 0; i < matrix->getXResolution(); i++) {
        if (isRGBMode) {
          matrix->drawPixelRGB888(i, j, rawDataBuffer[k], rawDataBuffer[k + 1], rawDataBuffer[k + 2]);
          k += 3;
        } else {
          matrix->drawPixelRGB888(i, j, rawDataBuffer[k], rawDataBuffer[k], rawDataBuffer[k]);
          k++;
        }
      }
    }
  }
  
  void setRGBMode(bool rgbMode) {
    if (isRGBMode != rgbMode) {
      isRGBMode = rgbMode;
      delete[] rawDataBuffer;
      numUniverses = (totalPixels * (isRGBMode ? 3 : 1) + channelsPerUniverse - 1) / channelsPerUniverse;
      rawDataBuffer = new uint8_t[totalPixels * (isRGBMode ? 3 : 1)];
      begin();  // Reinitialize E1.31 with new universe count
    }
  }

  bool getRGBMode() const {
    return isRGBMode;
  }
};