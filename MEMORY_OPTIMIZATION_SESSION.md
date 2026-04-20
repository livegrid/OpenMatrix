# Memory optimization & web stability — session summary

Handoff document: what we planned, what shipped, what broke, and what to try next.  
For ongoing tuning detail, see **`MEMORY_TUNING.md`**.

---

## Original problem

ESP32-S3 internal DRAM (~320 KB usable) was exhausted when enabling WiFi, SCD40, and other features. Crashes included heap failures, `esp_wifi_init` issues, and instability under load. Goal was **large wins** (tens of KB), not micro-optimizations.

---

## Approved plan (what we set out to do)

1. **Move `GFX_Layer` pixel buffers to PSRAM** — local fork at `C:/Users/dhruv/Code/GFX_Lite`; keep **two layers** (Aquarium water background generates incrementally; second layer for compositing).
2. **Reduce HUB75 color depth** — fewer DMA-capable bytes.
3. **Update `MEMORY_TUNING.md`** — reflect real behavior.
4. **MQTT** — bounded retry window (e.g. ~10 min), then give up; re-arm on settings update or reboot; no MQTT/EDMX when WiFi is disabled; cleaner API (`onSettingsUpdated`, etc.).
5. **Touch** — fix compile/runtime so `TOUCH_ENABLED` can be off without errors (guards in `main.cpp`).
6. **EffectManager** — lazy instantiation (one active effect); avoid `dynamic_cast` (build uses `-fno-rtti`); use factory/registry lambdas.
7. **Defer Aquarium lazy load** — optional later.
8. **Web UI** — try serving gzipped HTML from LittleFS to free flash; PROGMEM fallback.
9. **Aquarium** — `drawMultilineText` without `std::vector<String>` churn.
10. **Explicit deferrals** — TOF buffers in PSRAM at 30 fps, JsonDoc memory pool, aggressive WiFi simplification — skip for now.

---

## What was implemented (executed)

| Area | Change |
|------|--------|
| **GFX_Lite** | `GFX_Layer::init()` allocates row table + rows with `heap_caps_malloc(..., MALLOC_CAP_SPIRAM \| MALLOC_CAP_8BIT)`, internal fallback; destructor frees rows correctly (fixes leak). |
| **platformio.ini** | Local `symlink://.../GFX_Lite`; `-DPIXEL_COLOR_DEPTH_BITS=6`; (later) flash size overrides for N8R2 where applied — see repo. |
| **DisplayTask** | Stack reduced from 32 KB → **20 KB** (5120 words); **12 KB overflowed** on first `aquarium.begin()` — steady-state HWM was misleading. |
| **main.cpp** | `WIFI_ENABLED`: EDMX + MQTT tasks/objects gated; touch helpers when `TOUCH_ENABLED` off; DMX `update()` only with WiFi. |
| **MQTTManager** | State machine (`IDLE` / `ARMED` / `CONNECTED` / `GAVE_UP`), retry window + interval, `onSettingsUpdated()`. |
| **WebServerManager** | Calls `onSettingsUpdated()` instead of old reconnect helper. |
| **EffectManager** | Slots with `std::function` factories + TOF apply lambdas; single `m_current` effect. |
| **Aquarium.h** | `drawMultilineText` uses fixed stack buffers (`char lines[][]`), no `String`/`vector` for line splitting. |
| **UI.cpp** | Replaced naive `streamFile()` / large `send_P` with **chunked sends** + `vTaskDelay(1)` to reduce **task watchdog (TG1WDT_SYS_RST)** risk; index from **PROGMEM** `OPEN_MATRIX_HTML` chunked; static assets via `streamLittleFSFileYielding()`. |
| **LittleFS / data** | Script `scripts/extract_interface_html.py` → `data/public/index.html.gz`; `uploadfs` workflow documented. |
| **Partitions (N8R2)** | Earlier iteration expanded LittleFS in `partitions_custom.csv` and set **8 MB flash** in `platformio.ini` — **verify current file** matches your chip (N8R2 = 8 MB flash / 2 MB PSRAM). |

---

## Issues encountered along the way

### 1. DisplayTask stack canary (`Stack canary watchpoint`)

- **Cause:** `aquarium.begin()` first-boot path uses much more stack than steady-state drawing.
- **Fix:** 20 KB stack (was briefly 12 KB).

### 2. LittleFS “filesystem full” / intermittent `uploadfs` failures

- **Cause:** Old partition table gave **~64 KB** for the FS while `data/` (GIFs + `index.html.gz`) is **~240 KB+**. Packing order made failures **non-deterministic**.
- **Fix:** Enlarge spiffs/LittleFS partition and use **8 MB** flash settings for N8R2 (see `partitions_custom.csv` + `platformio.ini`).

### 3. Web: `TG1WDT_SYS_RST` when loading UI from another device

- **Symptom:** Reset often right after parsing a request (e.g. `/`, `/public/logo_32.png`), **not** always a Guru Meditation with backtrace.
- **Hypothesis:** Synchronous `WebServer` work on **ServerTask** (core 0) monopolizes CPU; **IDLE** doesn’t run → **task watchdog** fires. `delay(0)` does **not** yield in FreeRTOS; **`vTaskDelay(1)`** does.
- **Mitigations tried:** Chunked PROGMEM index; `streamLittleFSFileYielding()` with per-chunk `vTaskDelay(1)`.
- **Still open (when you resume):** Logs still showed **`[W][WebServer.cpp:572] send(): content length is zero`** before index completed, then success log — worth checking **ESP32 WebServer** expectations for `setContentLength` + empty `send()` + `sendContent`. TWDT may still fire on **`/public/logo_32.png`** if something else blocks (LWIP stack, **ServerTask stack**, **second client**, **TOF init** overlapping in time — your log shows TOF init lines around the same window). Next steps: **raise ServerTask stack**, **run web on dedicated lower-priority work**, or **migrate hot paths to AsyncWebServer**; add **TWDT feed** only as a last resort; decode **Saved PC** with `addr2line` on current `.elf`.

---

## Explicitly not done (per agreement)

- TOF results buffer in PSRAM at 30 fps (perf concern).
- JsonDocument with static pool (later).
- Aquarium lazy allocation (deferred).
- Further WiFi simplification until rest is validated.

---

## Files to read first when resuming

| File | Why |
|------|-----|
| `MEMORY_TUNING.md` | Canonical tuning table, stacks, PSRAM notes. |
| `lib/UI/UI.cpp` | `serveIndexHtml`, `streamLittleFSFileYielding`, all routes. |
| `src/main.cpp` | `serverTask`, `displayTask`, WiFi/MQTT gating. |
| `partitions_custom.csv` / `platformio.ini` | Flash + LittleFS size. |
| `C:/Users/dhruv/Code/GFX_Lite` (fork) | PSRAM layer allocation. |

---

## Quick validation checklist (device)

- Boot: two lines like `[GFX_Layer] Allocated … in PSRAM`.
- Heap after init: internal free should be **well above** pre-change baseline (~70 KB → **~180 KB+** was seen in logs).
- WiFi UI: load `/`, then assets and `/openmatrix/state` without reset.
- If WDT persists: capture **full serial** from boot through reset and note **last parsed URL**; run `pio run` and use `xtensa-esp32s3-elf-addr2line` on **Saved PC** from reset line.

---

*Generated as a project handoff; edit as you narrow the remaining web/TWDT issue.*
