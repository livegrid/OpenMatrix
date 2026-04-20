---
name: post-change-upload-monitor
description: >-
  After agent-made firmware or embedded changes, decides whether to upload to the
  device and whether to run a short serial monitor pass. Skips redundant
  build-before-upload (upload compiles). Handles PlatformIO upload failures when
  the serial port is busy (Windows), ensures monitors are stopped cleanly, and
  caps retries after crashes. Use when finishing edits to OpenMatrix firmware
  (src/, include/, platformio.ini), memory or stack tuning, BLE, boot-critical
  paths, or any change where a flash-and-watch verification is expected.
---

# Post-change upload and monitor

Apply at the **end of a batch of agent changes** to this repo (ESP32-S3, PlatformIO). Do not upload after documentation-only edits unless the user asked to verify a build.

## 1. Should you upload?

**Upload** when the change touches firmware behavior, build, or device configuration: `src/`, `include/`, `lib/`, `platformio.ini`, partition tables, or anything that affects the binary.

**Skip upload** when the change is docs-only, comments-only, or the user explicitly said not to flash.

**Do nothing** (no compile, no upload, no monitor) when the change is **minor**, you are confident it is safe, and **a device flash is not needed**—a redundant build only slows things down.

**Default for real firmware edits:** assume **upload is usually required** after changes that affect the binary.

**Do not** run a **separate build before upload.** `pio run -t upload` (and `fw.ps1 upload`) already compile; an extra `build` first is wasted time.

Use **`build` / `pio run` alone** only when you need a compile check **without** flashing (e.g. the user asked for a build-only sanity check, or no hardware is available).

## 2. Should you monitor serial for a few seconds?

**Run a short monitor** (typically **5–15 seconds** after boot lines appear) when any of these apply:

- Memory, heap, stack, PSRAM, or WiFi buffer tuning
- Startup, crash, watchdog, or boot-loop risk
- USB CDC / serial logging relied on for validation
- BLE, OTA, or timing-sensitive paths where “it compiles” is insufficient

**Skip or shorten monitoring** for trivial edits with no runtime risk, or when the user forbids attaching to the port.

While monitoring: watch for **panic, Guru Meditation, boot loop, repeated crash**, or obvious functional failure. **Always stop the monitor** when done (exit the monitor process cleanly so the port is freed for the next upload).

## 3. Execution order

Prefer the repo **`.cursor/scripts/fw.ps1`** helpers from the project root—these are **for agent automation** (see `AGENTS.md`), not end-user documentation. They keep timed monitors bounded and let `free-serial` target monitor processes only:

- **Upload** (includes compile): `pwsh -File .cursor/scripts/fw.ps1 upload` (or `pio run -t upload`; use project default env unless the user specified another). Do **not** run **`build` immediately before** this.
- **Build only** (no flash): `pwsh -File .cursor/scripts/fw.ps1 build` when a compile check without uploading is explicitly appropriate—**not** as a routine step before upload.
- **If upload fails — port busy / access denied**:
   - Note the COM port from the error or `pio device list`.
   - Run `pwsh -File .cursor/scripts/fw.ps1 free-serial`, then retry **`upload` once**. Also close any other serial monitor (IDE serial tab, PuTTY, etc.) if still blocked. Do not kill unrelated system processes.
- **Monitor (if warranted)**: `pwsh -File .cursor/scripts/fw.ps1 monitor` (default **15 seconds**, then the script stops the monitor). Use `-Seconds N` or `-Port COMn` if needed. This replaces a manually started `pio device monitor` so the port is always released afterward.
- **If the device crashes or misbehaves**:
   - **First failure**: Summarize logs; if the fix is obvious and low-risk (e.g., revert a single bad flag, fix a clear null dereference), apply it and repeat from **upload** (counts as one retry round).
   - **Second failure**: Reassess. If the cause is unclear or the fix would be speculative, **stop** and report logs + hypothesis to the user.
   - **Hard limit**: After **2 failed fix-and-retry cycles** (or **3** total upload+verify attempts including the first), **stop trying**. Briefly explain what failed and what you would need from the user (hardware state, other branches, serial capture). Do not burn more attempts; the user may have context you do not.

## 4. What to report to the user

- Whether you uploaded and for how long you monitored
- Upload retry after freeing the port (yes/no)
- Outcome: stable / crash / inconclusive, with **short** log excerpts if useful
- If stopped early: why, and what you recommend next

## 5. Anti-patterns

- Running **`build` then `upload`** in one flow (upload already builds)
- Leaving `pio device monitor` running in the background after the task is done
- Uploading after every tiny comment-only change without user preference
- More than **~3** upload-and-verify attempts for the same issue without new information
- Killing processes unrelated to serial/build tooling just to “free” the port
