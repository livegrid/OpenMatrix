#include "BleHidRemote.h"

#ifdef BLE_HID_REMOTE_ENABLED

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <StateManager.h>
#include <esp_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <algorithm>
#include <cstring>

static constexpr const char* kTag = "BleHidRemote";
static constexpr const char* kTargetMac = "dc:32:62:7f:b6:6b";
static constexpr uint32_t kConnectTimeoutMs       = 10000;
static constexpr int      kConnectAttemptsPerAddr = 3;
static constexpr uint32_t kPostInitDelayMs        = 1500;
static constexpr uint32_t kBetweenConnectMs       = 2500;
static constexpr uint32_t kBetweenStagesMs        = 3000;
static constexpr unsigned kReadHexMax             = 96;

/** EffectManager slot indices: Constellation, Meteor, Gravity Flap, Space Invaders, Asteroid Hopper, Space Drift (Noise excluded). */
static const uint8_t kRemoteEffectSlots[] = {0, 1, 3, 2, 5, 6};
static constexpr size_t kRemoteEffectCount = sizeof(kRemoteEffectSlots) / sizeof(kRemoteEffectSlots[0]);

static NimBLEClient* g_client         = nullptr;
static volatile bool g_connected      = false;
static StateManager*   g_stateManager = nullptr;
static size_t          g_remoteRingIdx = 0;
static BleHidRemoteTofRangeAdjustCallback g_tofRangeAdjustCallback = nullptr;

static void syncRemoteRingFromState() {
  if (!g_stateManager) {
    return;
  }
  const uint8_t slot =
      (static_cast<int>(g_stateManager->getState()->effects.selected) > 0)
          ? static_cast<uint8_t>(g_stateManager->getState()->effects.selected) - 1
          : 0;
  for (size_t i = 0; i < kRemoteEffectCount; i++) {
    if (kRemoteEffectSlots[i] == slot) {
      g_remoteRingIdx = i;
      return;
    }
  }
  g_remoteRingIdx = 0;
}

void bleHidRemoteSetStateManager(StateManager* stateManager) {
  g_stateManager = stateManager;
  syncRemoteRingFromState();
}

void bleHidRemoteSetTofRangeAdjustCallback(BleHidRemoteTofRangeAdjustCallback callback) {
  g_tofRangeAdjustCallback = callback;
}

struct HidReportSample {
  uint8_t len;
  uint8_t data[16];
};
static QueueHandle_t g_hidQueue = nullptr;

static constexpr uint8_t kMaskX_byte7 = 0x08;
static constexpr uint8_t kMaskYBtn    = 0x10;
static constexpr uint8_t kMaskA_byte8 = 0x08;
static constexpr uint8_t kMaskB_byte7 = 0x80;
static constexpr uint8_t kMaskOpt_b8  = 0x04;
static bool              g_prevX        = false;
static bool              g_prevYBtn     = false;
static bool              g_prevA        = false;
static bool              g_prevB        = false;
static bool              g_prevOpt      = false;
static bool              g_prevTop      = false;
static uint8_t           g_prevSuffix[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static bool              g_havePrevSuffix  = false;

static void logHex(const char* label, const uint8_t* p, size_t len, size_t maxBytes) {
  Serial.printf("%s (%u bytes): ", label, (unsigned)len);
  const size_t n = std::min(len, maxBytes);
  for (size_t i = 0; i < n; i++) {
    Serial.printf("%02X ", p[i]);
  }
  if (len > maxBytes) {
    Serial.print("...");
  }
  Serial.println();
}

static void applyRemoteEffectSlot(uint8_t slotIndex) {
  if (!g_stateManager || slotIndex >= kRemoteEffectCount) {
    return;
  }
  State* st = g_stateManager->getState();
  st->tofDebugView = false;
  st->mode         = OpenMatrixMode::EFFECT;
  st->effects.selected =
      static_cast<Effects>(static_cast<int>(kRemoteEffectSlots[slotIndex]) + 1);
  g_stateManager->save();
  ESP_LOGI(kTag, "Remote effect ring[%u] -> manager slot %u, enum %d", (unsigned)slotIndex,
           (unsigned)kRemoteEffectSlots[slotIndex], (int)st->effects.selected);
}

static void onRemoteNextEffect() {
  if (!g_stateManager) {
    return;
  }
  g_remoteRingIdx = (g_remoteRingIdx + 1) % kRemoteEffectCount;
  applyRemoteEffectSlot(static_cast<uint8_t>(g_remoteRingIdx));
}

static void onRemotePrevEffect() {
  if (!g_stateManager) {
    return;
  }
  g_remoteRingIdx = (g_remoteRingIdx + kRemoteEffectCount - 1) % kRemoteEffectCount;
  applyRemoteEffectSlot(static_cast<uint8_t>(g_remoteRingIdx));
}

static void onRemoteAquarium() {
  if (!g_stateManager) {
    return;
  }
  State* st        = g_stateManager->getState();
  st->tofDebugView = false;
  st->mode         = OpenMatrixMode::AQUARIUM;
  g_stateManager->save();
  ESP_LOGI(kTag, "Mode -> AQUARIUM");
}

static void onRemoteTofDebugToggle() {
#ifndef VL53L8CX_ENABLED
  ESP_LOGW(kTag, "OPT: TOF not compiled in (VL53L8CX_ENABLED)");
  return;
#endif
  if (!g_stateManager) {
    return;
  }
  State* st        = g_stateManager->getState();
  st->tofDebugView = !st->tofDebugView;
  g_stateManager->save();
  ESP_LOGI(kTag, "TOF debug overlay %s", st->tofDebugView ? "on" : "off");
}

static void onRemoteIncreaseTofRange() {
  if (!g_tofRangeAdjustCallback) {
    return;
  }
  g_tofRangeAdjustCallback(+250);
}

static void onRemoteDecreaseTofRange() {
  if (!g_tofRangeAdjustCallback) {
    return;
  }
  g_tofRangeAdjustCallback(-250);
}

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    (void)pClient;
    g_connected = true;
    Serial.println("[CB] connected");
  }

  void onConnectFail(NimBLEClient* pClient, int reason) override {
    (void)pClient;
    Serial.printf("[CB] connect failed reason=%d (%s)\n", reason,
                  NimBLEUtils::returnCodeToString(reason));
  }

  void onDisconnect(NimBLEClient* pClient, int reason) override {
    (void)pClient;
    g_connected = false;
    g_prevX = g_prevYBtn = g_prevA = g_prevB = g_prevOpt = g_prevTop = false;
    g_havePrevSuffix = false;
    memset(g_prevSuffix, 0xFF, sizeof(g_prevSuffix));
    Serial.printf("[CB] disconnected reason=%d (%s)\n", reason,
                  NimBLEUtils::returnCodeToString(reason));
  }

  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
    Serial.printf("[CB] authentication complete: bonded=%s encrypted=%s\n",
                  connInfo.isBonded() ? "yes" : "no",
                  connInfo.isEncrypted() ? "yes" : "no");
  }

  void onMTUChange(NimBLEClient* pClient, uint16_t MTU) override {
    (void)pClient;
    Serial.printf("[CB] MTU=%u\n", (unsigned)MTU);
  }
};

static void dumpGattAndSubscribe(NimBLEClient* client) {
  Serial.println("\n--- GATT discovery ---");
  if (!client->discoverAttributes()) {
    Serial.printf("discoverAttributes failed, lastError=%d\n", client->getLastError());
    return;
  }

  const auto& services = client->getServices();
  Serial.printf("Services: %u\n", (unsigned)services.size());

  for (NimBLERemoteService* svc : services) {
    Serial.printf("\n[Service] %s  handle=0x%04x end=0x%04x\n",
                  svc->getUUID().toString().c_str(), svc->getStartHandle(),
                  svc->getEndHandle());

    const auto& chars = svc->getCharacteristics();
    for (NimBLERemoteCharacteristic* chr : chars) {
      Serial.printf("  [Char] %s  handle=0x%04x\n", chr->getUUID().toString().c_str(),
                    chr->getHandle());
      Serial.printf("         read=%d write=%d writeNR=%d notify=%d indicate=%d\n",
                    chr->canRead(), chr->canWrite(), chr->canWriteNoResponse(),
                    chr->canNotify(), chr->canIndicate());

      const auto& dscs = chr->getDescriptors();
      for (NimBLERemoteDescriptor* dsc : dscs) {
        Serial.printf("    [Dsc] %s  handle=0x%04x\n", dsc->getUUID().toString().c_str(),
                      dsc->getHandle());
      }

      if (chr->canRead()) {
        NimBLEAttValue v = chr->readValue();
        logHex("         READ", v.data(), v.length(), kReadHexMax);
      }
    }
  }

  Serial.println("\n--- Subscribing notify / indicate ---");
  auto notifyCb = [](NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len,
                     bool isNotify) {
    (void)chr;
    (void)isNotify;
    if (!g_hidQueue || len == 0) {
      return;
    }
    HidReportSample s{};
    s.len = (len > sizeof(s.data)) ? (uint8_t)sizeof(s.data) : (uint8_t)len;
    memcpy(s.data, data, s.len);
    xQueueSend(g_hidQueue, &s, 0);
  };

  for (NimBLERemoteService* svc : client->getServices()) {
    for (NimBLERemoteCharacteristic* chr : svc->getCharacteristics()) {
      if (chr->canNotify()) {
        if (chr->subscribe(true, notifyCb)) {
          Serial.printf("Subscribed NOTIFY: %s\n", chr->getUUID().toString().c_str());
        } else {
          Serial.printf("NOTIFY subscribe failed: %s\n", chr->getUUID().toString().c_str());
        }
      }
      if (chr->canIndicate()) {
        if (chr->subscribe(false, notifyCb)) {
          Serial.printf("Subscribed INDICATE: %s\n", chr->getUUID().toString().c_str());
        } else {
          Serial.printf("INDICATE subscribe failed: %s\n", chr->getUUID().toString().c_str());
        }
      }
    }
  }
  Serial.println(
      "Ready — A: next effect, Y: prev, X/B: TOF range +/-, OPT: TOF debug toggle, TOP: Aquarium; [RAW] on change.\n");
}

static void processHidReports() {
  HidReportSample s;
  while (g_hidQueue && xQueueReceive(g_hidQueue, &s, 0) == pdTRUE) {
    if (s.len < 9) {
      continue;
    }
    const uint8_t* d = s.data;

    if (!g_havePrevSuffix || memcmp(d + 4, g_prevSuffix, 5) != 0) {
      Serial.print("[RAW] ");
      for (unsigned i = 0; i < 9; i++) {
        Serial.printf("%02X ", d[i]);
      }
      Serial.println();
      memcpy(g_prevSuffix, d + 4, 5);
      g_havePrevSuffix = true;
    }

    const bool x   = (d[7] & kMaskX_byte7) != 0;
    const bool y   = (d[7] & kMaskYBtn) != 0;
    const bool a   = (d[8] & kMaskA_byte8) != 0;
    const bool b   = (d[7] & kMaskB_byte7) != 0;
    const bool opt = (d[8] & kMaskOpt_b8) != 0;
    const bool top = (d[5] == 0xFF);

    if (a && !g_prevA) {
      onRemoteNextEffect();
    }
    if (x && !g_prevX) {
      onRemoteIncreaseTofRange();
    }
    if (y && !g_prevYBtn) {
      onRemotePrevEffect();
    }
    if (b && !g_prevB) {
      onRemoteDecreaseTofRange();
    }
    if (opt && !g_prevOpt) {
      onRemoteTofDebugToggle();
    }
    if (top && !g_prevTop) {
      onRemoteAquarium();
    }

    g_prevX    = x;
    g_prevYBtn = y;
    g_prevA    = a;
    g_prevB    = b;
    g_prevOpt  = opt;
    g_prevTop  = top;
  }
}

static void prepareClientBetweenConnectAttempts() {
  if (g_client->isConnected()) {
    g_client->disconnect();
    delay(400);
  }
  g_client->cancelConnect();
  delay(kBetweenConnectMs);
}

static bool connectWithRetries(const NimBLEAddress& addr, const char* stageLabel) {
  for (int attempt = 1; attempt <= kConnectAttemptsPerAddr; attempt++) {
    Serial.printf("  [%s] attempt %d/%d\n", stageLabel, attempt, kConnectAttemptsPerAddr);
    if (g_client->connect(addr)) {
      return true;
    }
    if (attempt < kConnectAttemptsPerAddr) {
      prepareClientBetweenConnectAttempts();
    }
  }
  return false;
}

static bool connectDirect() {
  g_client->setConnectTimeout(kConnectTimeoutMs);
  g_client->setConnectRetries(0);

  Serial.printf(
      "Direct connect %s — %d tries/stage, %lu ms/connect, +%lu ms between tries, +%lu ms "
      "between stages\n",
      kTargetMac, kConnectAttemptsPerAddr, (unsigned long)kConnectTimeoutMs,
      (unsigned long)kBetweenConnectMs, (unsigned long)kBetweenStagesMs);

  NimBLEAddress randomAddr(kTargetMac, BLE_ADDR_RANDOM);
  Serial.printf("--- Stage 1: RANDOM %s\n", randomAddr.toString().c_str());
  if (connectWithRetries(randomAddr, "RANDOM")) {
    return true;
  }

  Serial.printf("--- Stage 1 exhausted — waiting %lu ms before PUBLIC\n",
                (unsigned long)kBetweenStagesMs);
  prepareClientBetweenConnectAttempts();
  delay(kBetweenStagesMs);

  NimBLEAddress publicAddr(kTargetMac, BLE_ADDR_PUBLIC);
  Serial.printf("--- Stage 2: PUBLIC %s\n", publicAddr.toString().c_str());
  return connectWithRetries(publicAddr, "PUBLIC");
}

void bleHidRemoteTask(void* parameter) {
  (void)parameter;

  Serial.begin(115200);
  delay(300);
  vTaskDelay(pdMS_TO_TICKS(500));
  Serial.println();
  Serial.println("=== MOCUTE / HID client (NimBLE) ===");

  NimBLEDevice::init("ESP32_HID");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  g_hidQueue = xQueueCreate(32, sizeof(HidReportSample));
  if (!g_hidQueue) {
    Serial.println("xQueueCreate failed");
  }

  g_client = NimBLEDevice::createClient();
  g_client->setClientCallbacks(new ClientCallbacks(), true);

  Serial.printf("Waiting %lu ms after BLE init before connect...\n",
                (unsigned long)kPostInitDelayMs);
  delay(kPostInitDelayMs);

  if (!connectDirect()) {
    Serial.println(
        "Connection failed — giving up (enable pad / check MAC / try other address type).");
    for (;;) {
      processHidReports();
      delay(500);
    }
  }

  Serial.printf("Peer: %s  RSSI=%d  MTU=%u\n", g_client->getPeerAddress().toString().c_str(),
                g_client->getRssi(), (unsigned)g_client->getMTU());

  Serial.println("Starting encryption/pairing (secureConnection)...");
  if (g_client->secureConnection(false)) {
    Serial.println("secureConnection finished OK (or not required).");
  } else {
    Serial.println("secureConnection failed — continuing; some reads may fail until paired.");
  }
  delay(200);

  dumpGattAndSubscribe(g_client);
  syncRemoteRingFromState();

  for (;;) {
    processHidReports();

    if (g_client && g_connected && g_client->isConnected()) {
      static uint32_t last = 0;
      if (millis() - last > 60000) {
        last = millis();
        Serial.printf("[alive] RSSI=%d\n", g_client->getRssi());
      }
    }
    delay(5);
  }
}

#endif  // BLE_HID_REMOTE_ENABLED
