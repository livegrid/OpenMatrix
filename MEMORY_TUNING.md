# Memory tuning (stack sizes & monitoring)

Stack sizes are set from `uxTaskGetStackHighWaterMark()` runs. DebugMonitor logs heap and per-task stack HWM every 15s.

## ✅ PSRAM Task Stacks - Memory Optimization

**Solution implemented: Tasks with PSRAM stacks free internal RAM**
- DisplayTask: 20KB in PSRAM (drawing operations, not time-critical)
- SensorTask: 8KB in PSRAM (periodic sensor reads)
- ServerTask: 28KB in Internal RAM (WiFi needs fast access)
- TOFTask: 28KB in Internal RAM (I2C/DMA operations)
- TouchTask: 8KB in Internal RAM (needs responsive input)
- **Result**: ~28KB internal RAM freed, all features working!

## Current task stack sizes (words → bytes)

| Task          | Stack (words) | Bytes | Memory | Notes |
|---------------|---------------|-------|--------|--------|
| DisplayTask   | 5120          | 20480 | PSRAM  | HWM ~4404 B, drawing ops |
| ServerTask    | 7168          | 28672 | Internal | HWM ~7528 B, WiFi/web |
| TouchTask     | 2048          | 8192  | Internal | HWM ~1388 B, responsive input |
| SensorTask    | 2048          | 8192  | PSRAM  | HWM ~604 B, periodic reads |
| TOFTask       | 7168          | 28672 | Internal | HWM ~6488 B, I2C/DMA |
| DebugMonitor  | 3072          | 12288 | Internal | HWM ~1468 B, monitoring |

## After you run with serial monitor

1. Close any serial monitor so COM9 is free.
2. Upload and open monitor:
   ```powershell
   .\scripts\upload_and_monitor.ps1
   ```
   Or: `pio run -t upload` then `pio device monitor -b 115200`.
3. Check log line **"Heap after init"**: internal free and largest_block.
4. In **"Memory & Tasks"** blocks (every 15s), note each task’s **Stack HWM**. If any HWM is within ~500 bytes of that task’s stack size, increase that task’s stack in `main.cpp` to avoid overflow.

## If something crashes or doesn't start

- **Stack overflow** in logs → increase that task's stack (e.g. TOFTask 4096→5120, ServerTask 6144→7168).
- **OOM / allocation failure** → reduce a task stack or move large buffers to PSRAM (see plan TBD memory section).

## Solutions for enabling TouchTask

To enable TouchTask while keeping WiFi and VL53L8CX, choose one:

1. **Move task stacks to PSRAM** (recommended, requires code changes):
   - Modify TaskManager to use `xTaskCreatePinnedToCoreWithCaps()` with MALLOC_CAP_SPIRAM
   - Move DisplayTask, SensorTask stacks to PSRAM (~28KB freed)
   - Keep ServerTask and TOFTask in internal RAM (need fast access)

2. **Reduce TOF resolution** (simpler, may affect quality):
   - Change from 8x8 to 4x4 resolution (reduces memory usage)
   - Adjust TOFSensor configuration in main.cpp

3. **Conditional compilation** (choose features at build time):
   - Add build flags to choose: WiFi+TOF or Touch+TOF
   - User selects which features are needed per build
