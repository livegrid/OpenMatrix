# Memory tuning on the ESP32-S3

The ESP32-S3 has roughly 320 KB of usable internal DRAM. Between the HUB75
DMA buffers, framework/FreeRTOS overhead, the WiFi stack, task stacks, and
our own allocations we'd been running the internal heap flat, which caused
`esp_wifi_init` failures and AsyncUDP / TOF starvation.

This document tracks the actual tuning in place right now and explains the
rationale so we don't regress.

## Big-picture wins currently applied

| Change | Where | Savings on internal RAM |
|---|---|---|
| `GFX_Layer` pixel buffers allocated in PSRAM | `GFX_Lite` fork | ~72 KB (2 × 192×64×3) |
| HUB75 color depth 8 → 6 bits | `-DPIXEL_COLOR_DEPTH_BITS=6` | ~24 KB DMA RAM |
| `DisplayTask` stack 32 KB → 20 KB | `main.cpp` | ~12 KB |
| EDMX + MQTT compiled out when `WIFI_ENABLED` is off | `main.cpp` | varies (AsyncUDP + client buffers) |
| Lazy Effect instantiation (one at a time) | `EffectManager` | ~10–20 KB depending on effect |
| `Aquarium::drawMultilineText` uses stack buffers (no `std::vector<String>`) | `Aquarium.h` | transient fragmentation avoided |
| Fixed GFX_Layer destructor leak (was freeing only the struct, not the row data) | `GFX_Lite` fork | plugs ~72 KB leak on teardown |

## GFX_Layer in PSRAM (biggest single win)

The 192×64 display ships with two `GFX_Layer` buffers (background + foreground
for compositing — the foreground is needed because `Water` generates its
noise background 4 rows per frame and other elements must render on top of
the in-progress buffer). Each layer is 192×64 `CRGB` = ~36 KB, so two layers
is ~72 KB.

Previously they were allocated with `new CRGB[_width]`, which goes to the
default heap. Global C++ constructors run before `setup()` and before any
`heap_caps_malloc_extmem_enable(...)` flag is flipped, so these allocations
always landed in internal DRAM.

The fix is in the GFX_Lite fork at `c:/Users/dhruv/Code/GFX_Lite`:
`GFX_Layer::init()` now calls `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`
first and falls back to `MALLOC_CAP_INTERNAL` if PSRAM is unavailable. The
log line `[GFX_Layer] Allocated WxH pixel buffer (N bytes) in PSRAM` on boot
confirms the routing.

During iteration, `platformio.ini` points at the local fork via
`symlink://C:/Users/dhruv/Code/GFX_Lite`. When the fork is pushed, switch
back to the git URL.

## HUB75 color depth

Default is 8 bits per channel (`PIXEL_COLOR_DEPTH_BITS=8`). The DMA buffer
grows roughly linearly with depth:

- 8 bits → ~96 KB DMA RAM
- 6 bits → ~72 KB DMA RAM (−24 KB)
- 5 bits → ~60 KB DMA RAM (−36 KB)

We're currently at 6. Drop to 5 if the display still looks good and you
need more internal RAM.

## Task stacks

Sizes come from `uxTaskGetStackHighWaterMark()` via DebugMonitor. Numbers
below reflect the code as of this tuning pass.

| Task         | Stack (words) | Bytes  | Location     | HWM (approx) | Notes |
|--------------|---------------|--------|--------------|--------------|-------|
| DisplayTask  | 5120          | 20 KB  | Internal RAM | ~4.4 KB (steady) | Steady-state HWM is tiny, but `aquarium.begin()` first-boot path (fish/boid init, JSON (de)serialization, state restore) spikes stack usage and overflowed 12 KB. 20 KB gives ~4× headroom while still saving 12 KB vs the original 32 KB. |
| ServerTask   | 7528          | ~29 KB | PSRAM        | ~7.5 KB      | WebServer/AsyncTCP state. Stack in PSRAM frees internal RAM for AsyncUDP. |
| MQTTTask     | 4096          | 16 KB  | PSRAM        | TBD          | Only started when `WIFI_ENABLED`. Drives MQTT state machine. |
| SensorTask   | 2048          | 8 KB   | Internal RAM | ~0.6 KB      | Only started if BH1750 or ADXL345 is enabled. |
| TOFTask      | 7168          | ~28 KB | PSRAM        | ~6.5 KB      | I2C hardware is fine from a PSRAM stack; polling runs at ~30 Hz. |
| DebugMonitor | 3072          | 12 KB  | Internal RAM | ~1.5 KB      | Heap + HWM logger. |

`DisplayTask` **must** stay in internal RAM — drawing touches HUB75 control
structures on every frame and PSRAM latency costs noticeably on per-pixel
paths. Its stack (20 KB) is the biggest internal-RAM task by far. Note the
steady-state HWM (~4.4 KB) is misleading: `aquarium.begin()` on first boot
spikes much higher, and 12 KB overflowed in testing. If you lower this,
trigger a first-boot (wipe state or run with `RUN_DEMO`) while monitoring
HWM. A future option is to pre-allocate boid/fish/water state on the heap
(PSRAM) so the init path doesn't push down the stack so hard.

## What happens when WiFi is disabled

All network-facing code is gated behind `#ifdef WIFI_ENABLED`:

- `WebServerManager`, `UI`, `Edmx` (E1.31 + custom UDP), and `MQTTManager`
  are not instantiated.
- `ServerTask` and `MQTTTask` are not created.
- The `OpenMatrixMode::DMX` case is compiled out of the display loop.

This is the intended "offline" build. You should never see AsyncUDP,
AsyncTCP, mDNS, or MQTT activity with `WIFI_ENABLED` off.

## MQTT connection lifecycle

`MQTTManager` implements a small state machine:

- `IDLE` — no usable host configured. Does nothing.
- `ARMED` — host is set; try to connect every 30 s for up to 10 minutes.
- `CONNECTED` — publish sensor data + HA discovery.
- `GAVE_UP` — retry window exhausted. Stays silent until either the user
  updates MQTT settings via the web UI (`onSettingsUpdated()` is called from
  `WebServerManager`) or the device reboots.

No hard-coded broker IP — everything comes from
`state->settings.mqtt.host/port/...`.

## Touch

`TOUCH_ENABLED` can now be toggled independently of the rest of the build.
All `touchMenu.*` call sites in `main.cpp` are either compiled out or
delegated to `touchShouldStartDemo()` / `touchIsMenuOpen()` /
`touchShowSensorData()` helpers that return safe defaults when the feature
is disabled.

`TouchTask` itself is still commented out pending a proper calibration pass
for the S3 board.

## Web UI served from LittleFS

`lib/UI/interface.cpp` still contains a ~160 KB PROGMEM copy of the gzipped
web UI as a fallback. The runtime code now prefers
`/public/index.html.gz` on LittleFS (see `lib/UI/UI.cpp::serveIndexHtml`).

To populate the LittleFS copy and verify end-to-end:

1. `python scripts/extract_interface_html.py` — decodes `interface.cpp`
   into `data/public/index.html.gz`.
2. `pio run -t uploadfs` — flashes the LittleFS image.
3. Confirm boot log says `Sent UI from LittleFS`.

Once verified across all target devices, delete the `OPEN_MATRIX_HTML` array
from `interface.cpp` (and its declaration in `interface.h`) to reclaim
~160 KB of firmware space for OTA headroom.

`serveIndexHtml` sends the 160 KB payload in 4 KB chunks and calls
`vTaskDelay(1)` every 4 chunks (~16 KB). This is critical: a single
`streamFile()` / `send_P()` call, or even a tight loop of small chunks
without a real tick yield, starves `IDLE_0` on core 0 long enough to trip
the task watchdog (`TG1WDT_SYS_RST`). Note that `delay(0)` is a no-op in
FreeRTOS — only `vTaskDelay(n)` with `n>=1` actually lets IDLE run. If you
extend this helper to serve other large assets, keep the chunk-and-real-yield
pattern.

### Flash layout (N8R2: 8 MB flash / 2 MB PSRAM)

`partitions_custom.csv` was previously sized for a 4 MB chip (LittleFS only
got 64 KB, not enough for the 160 KB web UI). Current layout:

| Partition | Offset     | Size       | Notes                               |
|-----------|------------|------------|-------------------------------------|
| nvs       | `0x9000`   | 20 KB      | WiFi creds, settings                |
| otadata   | `0xE000`   | 8 KB       |                                     |
| app0      | `0x10000`  | 3.875 MB   | firmware (~1.2 MB today)            |
| spiffs    | `0x3F0000` | 4 MB       | LittleFS — web UI + image GIFs      |
| coredump  | `0x7F0000` | 64 KB      |                                     |

`platformio.ini` sets `board_upload.flash_size = 8MB` and
`board_build.flash_size = 8MB` so the bootloader's flash-size field matches
the chip (the stock board JSON defaults to 32 MB).

## How to monitor

1. Close any existing serial monitor so the COM port is free.
2. `pio run -t upload && pio device monitor -b 115200`
   (or `./scripts/upload_and_monitor.ps1`).
3. Check the boot line `Heap after init: internal free=... largest_block=...`.
4. Every 15 s DebugMonitor prints per-task stack HWM. If any HWM is within
   ~500 B of the stack size, bump that task's stack in `main.cpp`.

## Things we explicitly chose NOT to do (for now)

- **TOF on PSRAM stack at 30 Hz with high-res data**: measured too slow in
  a previous pass. Stays at 15 Hz polling rate with the buffers in internal
  RAM.
- **Aquarium lazy instantiation**: Aquarium is tightly coupled to the TOF
  interaction manager, SCD40, state, and boids. Effects are the easy win;
  Aquarium remains the always-resident mode.
- **Aggressive WiFi restart tweaks**: revisit only if the other wins
  aren't enough.
