#include "Edmx.h"

#include <TaskManager.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {
void logMemoryStats(const char* context) {
  const size_t freeHeap = ESP.getFreeHeap();
  const size_t minFreeHeap = ESP.getMinFreeHeap();
  const size_t freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t largestInternal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  const size_t freePsram = ESP.getFreePsram();
  const size_t largestPsram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

  log_i("[%s] Heap free=%u bytes (min=%u), internal free=%u bytes (largest=%u), "
        "PSRAM free=%u bytes (largest=%u)",
        context, freeHeap, minFreeHeap, freeInternal, largestInternal, freePsram,
        largestPsram);
}
}  // namespace

Edmx& Edmx::getInstance() {
  static Edmx instance;
  return instance;
}

bool Edmx::begin(Matrix* matrix, StateManager* stateManager) {
  this->matrix = matrix;
  this->stateManager = stateManager;
  totalPixels = matrix->getXResolution() * matrix->getYResolution();
  
  log_i("=== DMX Memory Requirements ===");
  log_i("Resolution: %dx%d = %d pixels", 
    matrix->getXResolution(), matrix->getYResolution(), totalPixels);
  
  bool started = applySettings();

  log_i("Free Heap before E131: %u bytes", ESP.getFreeHeap());
  log_i("Required universes: %d", numUniverses);
  log_i("Est. E131 memory: ~%d bytes", numUniverses * 638);
  logMemoryStats("Edmx::begin (post applySettings)");

  return started;
}

void Edmx::update() {
  if(millis() - lastPacketReceived > stateManager->getState()->settings.edmx.timeout) {
    newPacket = false;
    // stateManager->getState()->mode = prevMode;
  }

  // Display the background layer (which contains our DMX data) to the matrix
  if (matrix->background) {
    matrix->background->display();
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
bool Edmx::applySettings() {
  isRGBMode = stateManager->getState()->settings.edmx.mode == eDmxMode::DMX_MODE_RGB;
  uint32_t totalChannels = totalPixels * (isRGBMode ? 3 : 1);
  numUniverses = (totalChannels + channelsPerUniverse - 1) / channelsPerUniverse;

  // Ensure we have at least one universe
  if(numUniverses < 1) {
    numUniverses = 1;
  }

  log_i("DMX will use GFX_Layer buffer (%dx%d pixels, %d channels, %d universes)",
        matrix->getXResolution(), matrix->getYResolution(), totalChannels, numUniverses);

  packetDelay = stateManager->getState()->settings.edmx.timeout;

  log_i("Settings: RGB=%s, Pixels=%d, Channels=%d, Universes=%d, StartUni=%d, StartAddr=%d",
        isRGBMode ? "true" : "false", totalPixels, totalChannels, numUniverses,
        stateManager->getState()->settings.edmx.start_universe,
        stateManager->getState()->settings.edmx.start_address);
        
  logMemoryStats("Edmx::applySettings");
  
  bool dmxStarted = startE131();
  bool udpStarted = startUdp();

  return dmxStarted || udpStarted;
}

bool Edmx::startE131() {
  prepareForStart();

  int retryCount = 0;
  const int maxRetries = 5;

  while (retryCount < maxRetries) {
    log_i("Attempt %d to start %s", retryCount + 1, 
          stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net");
    
    logMemoryStats("Edmx::startE131 (pre begin)");

    bool success = _e131.begin(
      stateManager->getState()->settings.edmx.multicast ? E131_MULTICAST : E131_UNICAST,
      stateManager->getState()->settings.edmx.start_universe,
      numUniverses,
      stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? PROTOCOL_E131 : PROTOCOL_ARTNET
    );

    if (success) {
      log_i("%s initialization successful", 
            stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net");
      
      _e131.registerCallback(
        [this](void* packet, protocol_t protocol, void* userInfo) {
          this->onNewPacketReceived(packet, protocol, userInfo);
        }
      );
      restoreSuspendedTasks();
      return true;
    } else {
      log_e("%s initialization failed. WiFi status: %d", 
            stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net",
            WiFi.status());
      logMemoryStats("Edmx::startE131 (post failure)");
      retryCount++;
      vTaskDelay(pdMS_TO_TICKS(50));
    }
  }

  restoreSuspendedTasks();
  log_e("%s initialization failed after %d attempts", 
        stateManager->getState()->settings.edmx.protocol == eDmxProtocol::S_ACN ? "E1.31" : "Art-Net",
        maxRetries);
  return false;
}

bool Edmx::startUdp() {
  if (!stateManager) {
    return false;
  }

  const auto& settings = stateManager->getState()->settings.edmx;
  if (!settings.udp_enabled) {
    stopUdp();
    return false;
  }

  const uint16_t desiredPort = settings.udp_port;

  if (udpListening && currentUdpPort == desiredPort) {
    return true;
  }

  stopUdp();

  if (!_udp.listen(desiredPort)) {
    log_e("Failed to start UDP listener on port %u", desiredPort);
    return false;
  }

  _udp.onPacket([this](AsyncUDPPacket packet) {
    this->handleUdpPacket(packet);
  });

  udpListening = true;
  currentUdpPort = desiredPort;
  log_i("UDP streaming listener active on port %u", desiredPort);
  return true;
}

void Edmx::stopUdp() {
  if (!udpListening) {
    return;
  }
  _udp.close();
  udpListening = false;
  currentUdpPort = 0;
  log_i("UDP streaming listener stopped");
}

void Edmx::prepareForStart() {
  suspendedTasks.clear();
  startupSheddingActive = true;

  TaskManager& taskManager = TaskManager::getInstance();
  auto maybeSuspend = [&](const char* taskName) {
    if (taskManager.isTaskRunning(taskName)) {
      log_w("Suspending task '%s' during DMX startup", taskName);
      taskManager.suspendTask(taskName);
      suspendedTasks.emplace_back(taskName);
    }
  };

  maybeSuspend("SensorTask");
  maybeSuspend("TouchTask");
  maybeSuspend("DemoTask");

  logMemoryStats("Edmx::prepareForStart");
  vTaskDelay(pdMS_TO_TICKS(50));
}

void Edmx::restoreSuspendedTasks() {
  if (!startupSheddingActive) {
    return;
  }

  TaskManager& taskManager = TaskManager::getInstance();
  for (const auto& taskName : suspendedTasks) {
    log_i("Resuming task '%s' after DMX startup", taskName.c_str());
    taskManager.resumeTask(taskName);
  }

  suspendedTasks.clear();
  startupSheddingActive = false;
  logMemoryStats("Edmx::restoreSuspendedTasks");
}

void Edmx::onNewPacketReceived(void* packet, protocol_t protocol, void* userInfo) {
  newPacket = true;
  uint16_t universe;
  uint8_t* data;
  uint16_t dataSize;

  if (protocol == PROTOCOL_E131) {
    e131_packet_t* e131Packet = reinterpret_cast<e131_packet_t*>(packet);
    if (!e131Packet) {
      return;
    }
    universe = ntohs(e131Packet->universe);
    data = e131Packet->property_values + 1;
    dataSize = ntohs(e131Packet->property_value_count) - 1;

  } else if (protocol == PROTOCOL_ARTNET) {
    artnet_dmx_packet_t* artnetPacket = reinterpret_cast<artnet_dmx_packet_t*>(packet);
    if (!artnetPacket) {
      return;
    }
    uint16_t artnetUniverse = ntohs(artnetPacket->universe) & 0x7FFF;
    universe = (artnetUniverse / 256) + 1;
    data = artnetPacket->dmx;
    dataSize = ntohs(artnetPacket->length);
  } else {
    return;
  }

  if (universe < stateManager->getState()->settings.edmx.start_universe ||
      universe >= stateManager->getState()->settings.edmx.start_universe + numUniverses) {
    return;
  }

  // Update the background layer directly with DMX data
  if (!matrix->background) {
    return;
  }

  uint16_t universeIndex = universe - stateManager->getState()->settings.edmx.start_universe;
  uint16_t startChannel = universeIndex * channelsPerUniverse;
  uint16_t startAddress = stateManager->getState()->settings.edmx.start_address - 1;

  startChannel = (startChannel > startAddress) ? startChannel - startAddress : 0;

  uint16_t totalChannels = totalPixels * (isRGBMode ? 3 : 1);
  uint16_t endChannel = (startChannel + dataSize < totalChannels) ? startChannel + dataSize : totalChannels;

  // Convert linear DMX channel indexing to 2D pixel coordinates
  uint16_t channelsToCopy = endChannel - startChannel;
  uint8_t* sourceData = data + startAddress;

  // Process each channel in this packet
  for (uint16_t channelOffset = 0; channelOffset < channelsToCopy; channelOffset++) {
    uint16_t globalChannelIndex = startChannel + channelOffset;
    uint16_t pixelIndex = globalChannelIndex / (isRGBMode ? 3 : 1);
    uint16_t channelInPixel = globalChannelIndex % (isRGBMode ? 3 : 1);

    if (pixelIndex >= totalPixels) continue;

    uint16_t x = pixelIndex % matrix->getXResolution();
    uint16_t y = pixelIndex / matrix->getXResolution();

    // Get the current pixel color from the layer, modify the appropriate channel, and set it back
    CRGB currentColor = matrix->background->pixels->data[y][x];

    if (isRGBMode) {
      if (channelInPixel == 0) currentColor.r = sourceData[channelOffset];
      else if (channelInPixel == 1) currentColor.g = sourceData[channelOffset];
      else if (channelInPixel == 2) currentColor.b = sourceData[channelOffset];
    } else {
      // Monochrome mode - set all channels to the same value
      currentColor.r = currentColor.g = currentColor.b = sourceData[channelOffset];
    }

    matrix->background->pixels->data[y][x] = currentColor;
  }

  if (stateManager->getState()->mode != OpenMatrixMode::DMX) {
    prevMode = stateManager->getState()->mode;
    stateManager->getState()->mode = OpenMatrixMode::DMX;
  }

  lastPacketReceived = millis();
}

void Edmx::handleUdpPacket(AsyncUDPPacket packet) {
  if (!matrix || !matrix->background) {
    return;
  }

  const size_t length = packet.length();
  if (length < kUdpHeaderSize) {
    log_v("UDP packet too small (%u bytes)", static_cast<unsigned>(length));
    return;
  }

  const uint8_t* data = packet.data();

  if (data[0] != kUdpMagic0 || data[1] != kUdpMagic1) {
    log_v("UDP packet with invalid magic bytes");
    return;
  }

  if (data[2] != kUdpVersion) {
    log_w("UDP packet version mismatch: %u", data[2]);
    return;
  }

  const uint8_t flags = data[3];
  auto readLE16 = [](const uint8_t* ptr) -> uint16_t {
    return static_cast<uint16_t>(ptr[0]) |
           (static_cast<uint16_t>(ptr[1]) << 8);
  };

  const uint16_t frameId = readLE16(data + 4);
  const uint16_t chunkId = readLE16(data + 6);
  const uint16_t chunkCount = readLE16(data + 8);
  const uint16_t x = readLE16(data + 10);
  const uint16_t y = readLE16(data + 12);
  const uint16_t width = readLE16(data + 14);
  const uint16_t height = readLE16(data + 16);

  if (width == 0 || height == 0) {
    log_v("UDP packet ignored due to zero sized payload (frame %u chunk %u/%u)",
          frameId, chunkId, chunkCount);
    return;
  }

  if (x + width > matrix->getXResolution() ||
      y + height > matrix->getYResolution()) {
    log_w("UDP payload out of bounds: x=%u y=%u w=%u h=%u (frame %u chunk %u/%u)",
          x, y, width, height, frameId, chunkId, chunkCount);
    return;
  }

  const uint8_t* payload = data + kUdpHeaderSize;
  const size_t payloadLength = length - kUdpHeaderSize;

  bool success = false;
  if (flags & kUdpFlagCompressed) {
    success = processRlePayload(payload, payloadLength, x, y, width, height);
  } else {
    success = processRawPayload(payload, payloadLength, x, y, width, height);
  }

  if (!success) {
    log_w("Failed to process UDP chunk frame=%u chunk=%u/%u (flags=0x%02X)",
          frameId, chunkId, chunkCount, flags);
    return;
  }

  newPacket = true;
  lastPacketReceived = millis();

  if (stateManager->getState()->mode != OpenMatrixMode::DMX) {
    prevMode = stateManager->getState()->mode;
    stateManager->getState()->mode = OpenMatrixMode::DMX;
  }
}

bool Edmx::processRawPayload(const uint8_t* payload, size_t length,
                             uint16_t x, uint16_t y,
                             uint16_t width, uint16_t height) {
  if (!matrix || !matrix->background) {
    return false;
  }

  const size_t expectedPixels =
      static_cast<size_t>(width) * static_cast<size_t>(height);
  const size_t expectedBytes = expectedPixels * 3;

  if (length < expectedBytes) {
    log_w("Raw UDP payload too small: expected %u bytes, got %u bytes",
          static_cast<unsigned>(expectedBytes),
          static_cast<unsigned>(length));
    return false;
  }

  for (uint16_t row = 0; row < height; ++row) {
    CRGB* targetRow = matrix->background->pixels->data[y + row];
    for (uint16_t col = 0; col < width; ++col) {
      const size_t idx =
          (static_cast<size_t>(row) * width + col) * 3;
      CRGB& pixel = targetRow[x + col];
      pixel.r = payload[idx];
      pixel.g = payload[idx + 1];
      pixel.b = payload[idx + 2];
    }
  }

  return true;
}

bool Edmx::processRlePayload(const uint8_t* payload, size_t length,
                             uint16_t x, uint16_t y,
                             uint16_t width, uint16_t height) {
  if (!matrix || !matrix->background) {
    return false;
  }

  const size_t totalPixels =
      static_cast<size_t>(width) * static_cast<size_t>(height);

  size_t offset = 0;
  size_t written = 0;

  while (offset + 5 <= length && written < totalPixels) {
    const uint16_t runLength =
        static_cast<uint16_t>(payload[offset]) |
        (static_cast<uint16_t>(payload[offset + 1]) << 8);
    offset += 2;

    if (runLength == 0) {
      log_w("Encountered zero-length RLE run");
      return false;
    }

    CRGB color;
    color.r = payload[offset++];
    color.g = payload[offset++];
    color.b = payload[offset++];

    for (uint16_t i = 0; i < runLength && written < totalPixels; ++i) {
      const size_t localIndex = written++;
      const uint16_t row = static_cast<uint16_t>(localIndex / width);
      const uint16_t col = static_cast<uint16_t>(localIndex % width);
      CRGB& pixel = matrix->background->pixels->data[y + row][x + col];
      pixel = color;
    }
  }

  if (written != totalPixels) {
    log_w("RLE payload incomplete: expected %u pixels, wrote %u pixels",
          static_cast<unsigned>(totalPixels),
          static_cast<unsigned>(written));
    return false;
  }

  if (offset > length) {
    log_w("RLE payload read past buffer (offset=%u length=%u)",
          static_cast<unsigned>(offset),
          static_cast<unsigned>(length));
    return false;
  }

  return true;
}