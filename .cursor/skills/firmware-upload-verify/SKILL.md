---
name: firmware-upload-verify
description: Runs firmware validation with upload-first testing for embedded PlatformIO projects, and checks serial logs when risk is high. Use when code changes affect firmware behavior, build/upload flow, memory usage, timing, interrupts, sensors, or effect loops.
---

# Firmware Upload Verify

## Goal

Prefer flashing real hardware (`upload`) over compile-only checks after most code changes.  
For higher-risk changes, also verify runtime stability from serial logs.

## Trigger Conditions

Apply this workflow when:
- Editing firmware code (`src/`, `lib/`, low-level drivers, effect runtime logic)
- Changing `platformio.ini`, partition tables, memory-heavy buffers, timing/loop behavior
- Fixing bugs that could regress only on real hardware

Skip or downscope only for truly minor edits:
- Comments, docs, spelling-only text changes
- Pure rename/refactor with zero behavior change and no build-impacting edits

## Terminal Ownership Rules (Critical)

Before running commands:
1. Check currently running terminals and identify:
   - a **build/upload terminal**
   - a **serial monitor terminal** (if needed)
2. Reuse existing suitable terminals when possible; do not spawn duplicates.
3. Keep track of which terminal is assigned to each role during the session.
4. If serial is open and upload fails due to port lock:
   - stop/close serial monitor first
   - run upload
   - reopen serial monitor after upload completes
5. On task completion, report terminal roles used (upload terminal, serial terminal).

## Port Lock Prevention (Required)

Use this cleanup policy to avoid stale monitor processes keeping COM ports busy:

1. **Pre-upload cleanup (always):**
   - In PowerShell, terminate any stale monitor processes before `upload`:
     - `Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*pio device monitor*' -or $_.CommandLine -like '*--target monitor*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }`
2. **Never use combined long-running upload+monitor for validation by default:**
   - Prefer `pio run -t upload` first, then start monitor only if needed.
3. **Time-bound monitor sessions:**
   - For runtime checks, observe serial output for about 8-15 seconds after boot unless issue reproduction needs longer.
   - After observation, stop monitor immediately (`Ctrl+C` in terminal). If terminal close does not end child processes, run the cleanup command again.
4. **Post-validation cleanup (always):**
   - Before finishing, ensure no `pio device monitor` process remains.

## Standard Validation Flow

1. **Classify change size/risk**
   - `minor`: compile-only allowed
   - `normal`: upload required
   - `high-risk`: upload + serial monitoring required

2. **Run build/upload**
   - First run pre-upload stale monitor cleanup from **Port Lock Prevention**.
   - Default command:
     - `pio run -t upload`
   - If project requires environment selection:
     - `pio run -e <env> -t upload`

3. **Handle failures**
   - If upload fails due to serial/COM port busy, release serial terminal and retry upload.
   - If compile or upload fails, capture the key error lines and stop.

4. **High-risk runtime check**
   - Start serial monitor:
     - `pio device monitor`
     - or `pio device monitor -b <baud>`
   - Observe boot + steady-state behavior for crash indicators:
     - reboot loops, panic traces, Guru Meditation, watchdog resets, OOM/malloc failures
   - Keep monitor window time-bounded (default 8-15 seconds), then stop monitor and verify monitor processes are gone.
   - If seen, treat as failure and report signature.

## High-Risk Heuristics

Treat as high-risk when any of these are touched:
- Dynamic memory usage (`new`, `malloc`, large buffers, frame data, sensor arrays)
- ISR/concurrency/timing-sensitive paths
- Sensor processing pipelines and real-time rendering loops
- Effect manager scheduling or persistent state structures
- Networking + sensor/effect interactions

## Completion Report Format

Return a concise outcome with:
- Validation level used: `minor`, `normal`, or `high-risk`
- Upload status: pass/fail
- Serial check status: not-run/pass/fail
- Terminal roles tracked: upload terminal id, serial terminal id (or reused/none)
- Any blocking errors and their first actionable cause

