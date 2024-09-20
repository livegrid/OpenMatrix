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
  _e131.begin(
    stateManager->getState()->settings.edmx.multicast ? E131_MULTICAST : E131_UNICAST,
    stateManager->getState()->settings.edmx.start_universe,
    numUniverses,
    stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? PROTOCOL_E131 : PROTOCOL_ARTNET
  );

  _e131.registerCallback(
    [this](void* packet, protocol_t protocol, void* userInfo) {
      this->onNewPacketReceived(packet, protocol, userInfo);
    }
  );
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
  uint32_t totalChannels = totalPixels * (isRGBMode ? 3 : 1);
  numUniverses = (totalChannels + channelsPerUniverse - 1) / channelsPerUniverse;
  rawDataBuffer = new uint8_t[totalChannels];
  packetDelay = stateManager->getState()->settings.edmx.timeout;

  log_i("Applying settings: RGB mode: %s, Total pixels: %d, Total channels: %d, Num universes: %d",
        isRGBMode ? "true" : "false", totalPixels, totalChannels, numUniverses);

  int retryCount = 0;
  const int maxRetries = 5;

  while (retryCount < maxRetries) {
    log_i("Attempt %d to start %s", retryCount + 1, 
          stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net");
    
    bool success = _e131.begin(
      stateManager->getState()->settings.edmx.multicast ? E131_MULTICAST : E131_UNICAST,
      stateManager->getState()->settings.edmx.start_universe,
      numUniverses,
      stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? PROTOCOL_E131 : PROTOCOL_ARTNET
    );

    if (success) {
      log_i("%s initialization successful", 
            stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net");
      
      // Re-register the callback
      _e131.registerCallback(
        [this](void* packet, protocol_t protocol, void* userInfo) {
          this->onNewPacketReceived(packet, protocol, userInfo);
        }
      );
      
      return;
    } else {
      log_e("%s initialization failed. WiFi status: %d", 
            stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net",
            WiFi.status());
      retryCount++;
    }

    delay(1000);  // Wait a bit before retrying
  }

  log_e("%s initialization failed after %d attempts", 
        stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net",
        maxRetries);
}

void Edmx::onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo) {
  if (protocol != PROTOCOL_E131 && protocol != PROTOCOL_ARTNET) {
    return;  // Ignore non-E131/ArtNet packets
  }

  newPacket = true;
  uint16_t universe;
  uint8_t* data;
  uint16_t dataSize;

  if (protocol == PROTOCOL_E131) {
    e131_packet_t* e131Packet = reinterpret_cast<e131_packet_t*>(packet);
    if (!e131Packet) {
      return;  // Invalid E1.31 packet
    }
    universe = htons(e131Packet->universe);
    data = e131Packet->property_values + 1;
    dataSize = htons(e131Packet->property_value_count) - 1;
  } else {  // PROTOCOL_ARTNET
    artnet_dmx_packet_t* artnetPacket = reinterpret_cast<artnet_dmx_packet_t*>(packet);
    if (!artnetPacket) {
      return;  // Invalid Art-Net packet
    }
    // Convert Art-Net universe to 1-based universe
    uint16_t artnetUniverse = htons(artnetPacket->universe) & 0x7FFF;
    universe = (artnetUniverse / 256) + 1;  // Map every 256 Art-Net universes to one of our universes
    data = artnetPacket->dmx;
    dataSize = htons(artnetPacket->length);
  }

  // Check if the received universe is within our range
  if (universe < stateManager->getState()->settings.edmx.start_universe || 
      universe >= stateManager->getState()->settings.edmx.start_universe + numUniverses) {
    return;  // Ignore packets outside our universe range
  }

  uint16_t universeIndex = universe - stateManager->getState()->settings.edmx.start_universe;
  uint16_t startChannel = universeIndex * channelsPerUniverse;
  uint16_t endChannel = min(startChannel + dataSize, totalPixels * (isRGBMode ? 3 : 1));

  // Copy data to the correct position in rawDataBuffer
  memcpy(rawDataBuffer + startChannel, data, endChannel - startChannel);

  if (stateManager->getState()->mode != OpenMatrixMode::DMX) {
    prevMode = stateManager->getState()->mode;
    stateManager->getState()->mode = OpenMatrixMode::DMX;
  }

  lastPacketReceived = millis();
}