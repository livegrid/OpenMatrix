#include "Edmx.h"

Edmx& Edmx::getInstance() {
  static Edmx instance;
  return instance;
}

void Edmx::begin(Matrix* matrix, StateManager* stateManager) {
  this->matrix = matrix;
  this->stateManager = stateManager;
  totalPixels = matrix->getXResolution() * matrix->getYResolution();
  applySettings();
    _e131.registerCallback(
        [this](void* packet, protocol_t protocol, void* userInfo) {
            this->onNewPacketReceived(packet, protocol, userInfo);
        });
}

void Edmx::update() {
  if(millis() - lastPacketReceived > stateManager->getState()->settings.edmx.timeout) {
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

void Edmx::setRGBMode(bool rgbMode) {
  if (isRGBMode != rgbMode) {
    isRGBMode = rgbMode;
    applySettings();
  }
}

bool Edmx::getRGBMode() const {
  return isRGBMode;
}

void Edmx::applySettings() {
  delete[] rawDataBuffer;
  isRGBMode = stateManager->getState()->settings.edmx.mode == eDmxMode::DMX_MODE_RGB;
  numUniverses = (totalPixels * (isRGBMode ? 3 : 1) + channelsPerUniverse - 1) / channelsPerUniverse;
  rawDataBuffer = new uint8_t[totalPixels * (isRGBMode ? 3 : 1)];
  packetDelay = stateManager->getState()->settings.edmx.timeout;

  int retryCount = 0;
  const int maxRetries = 5;

  while (retryCount < maxRetries) {
    log_i("Attempt %d to start E1.31", retryCount + 1);
    
    if (_e131.begin(stateManager->getState()->settings.edmx.multicast ? E131_MULTICAST : E131_UNICAST,
                    stateManager->getState()->settings.edmx.start_universe,
                    numUniverses,
                    stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? PROTOCOL_E131 : PROTOCOL_ARTNET)) {
      log_i("E1.31 initialization successful");
      return;
    } else {
      log_e("E1.31 initialization failed. WiFi status: %d", WiFi.status());
      retryCount++;
      delay(1000);  // Wait for 1 second before retrying
    }
  }

  log_e("E1.31 initialization failed after %d attempts", maxRetries);
}

void Edmx::onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo) {
  if (protocol != PROTOCOL_E131 && protocol != PROTOCOL_ARTNET) {
    return;  // Ignore non-E131/ArtNet packets
  }

  e131_packet_t* e131Packet = reinterpret_cast<e131_packet_t*>(packet);
  if (!e131Packet) {
    return;  // Invalid packets
  }
  newPacket = true;
  uint16_t universe = htons(e131Packet->universe);
  
  // Check if the received universe is within our range
  if (universe < stateManager->getState()->settings.edmx.start_universe || 
      universe >= stateManager->getState()->settings.edmx.start_universe + numUniverses) {
    return;  // Ignore packets outside our universe range
  }

  uint16_t universeIndex = universe - stateManager->getState()->settings.edmx.start_universe;
  uint16_t startChannel = universeIndex * channelsPerUniverse;
  uint16_t endChannel = min(startChannel + channelsPerUniverse, totalPixels * (isRGBMode ? 3 : 1));

  // Copy data to the correct position in rawDataBuffer
  memcpy(rawDataBuffer + startChannel, e131Packet->property_values + 1, endChannel - startChannel);

  if (stateManager->getState()->mode != OpenMatrixMode::DMX) {
    prevMode = stateManager->getState()->mode;
    stateManager->getState()->mode = OpenMatrixMode::DMX;
  }

  lastPacketReceived = millis();
}