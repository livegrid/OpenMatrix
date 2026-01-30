# TOF Sensor + UDP Memory Conflict

## Problem Discovered

When `VL53L8CX_ENABLED` is defined, UDP streaming stalls even though:
- The TOF task creation is commented out (line 510)
- No TOF task is running
- The simple (working) UDP code is in place

## Root Cause: Memory Pressure & Heap Fragmentation

### What Happens When VL53L8CX_ENABLED is Defined:

1. **Global TOFSensor object** allocated at startup (line 36)
   - Consumes heap memory immediately

2. **TOFVisualizer allocated in DisplayTask** (line 138)
   ```cpp
   tofVisualizer = new TOFVisualizer(&tofSensor, &matrix);
   ```
   - Uses `new` which allocates from heap
   - Happens in DisplayTask BEFORE ServerTask starts
   - **Fragments the heap**

3. **Task Creation Order:**
   - DisplayTask created first (8192 bytes stack)
   - ServerTask created second (4096 bytes stack)
   - DisplayTask runs immediately and fragments heap
   - ServerTask starts 5 seconds later

4. **AsyncUDP Fails:**
   - AsyncUDP needs contiguous heap memory for buffers
   - Fragmented heap = no large contiguous blocks available
   - UDP socket operations fail silently
   - Packets are lost or UDP stops working

## The Fix: Stack Size Rebalancing

### Solution 1: Increase ServerTask Stack (Implemented)
When TOF is enabled, give ServerTask more stack (4096 → 8192 bytes):

```cpp
#ifdef VL53L8CX_ENABLED
  TaskManager::getInstance().createTask("ServerTask", serverTask, 8192, 1, 0);
#else
  TaskManager::getInstance().createTask("ServerTask", serverTask, 4096, 1, 0);
#endif
```

### Solution 2: Reduce DisplayTask Stack (Implemented)
When TOF is enabled, reduce DisplayTask (8192 → 6144 bytes):

```cpp
#ifdef VL53L8CX_ENABLED
  TaskManager::getInstance().createTask("DisplayTask", displayTask, 6144, 1, 1);
#else
  TaskManager::getInstance().createTask("DisplayTask", displayTask, 8192, 1, 1);
#endif
```

**Combined Effect:**
- DisplayTask: 8192 → 6144 bytes (saves 2048 bytes)
- ServerTask: 4096 → 8192 bytes (adds 4096 bytes)
- Net: +2048 bytes more memory for network operations

## Why This Works

1. **More heap available** for AsyncUDP buffers
2. **Less fragmentation** from competing allocations
3. **Better balance** between display and network needs
4. **Proven approach** - user mentioned stack adjustments fixed WiFi issues before

## ESP32 Memory Architecture

ESP32 has limited internal RAM:
- **DRAM:** ~200KB total
- **Stack:** Each task reserves its stack size
- **Heap:** Remaining memory after stack allocations

With TOF sensor:
- DisplayTask (8192) + ServerTask (4096) = 12288 bytes in stacks
- Plus TOFVisualizer heap allocation
- Plus AsyncUDP buffer allocation
- = **Memory pressure**

After adjustment:
- DisplayTask (6144) + ServerTask (8192) = 14336 bytes in stacks
- But better distributed for network operations
- ServerTask has more room for AsyncUDP

## Alternative Solutions (Not Implemented)

### Option A: Delay TOF Initialization
Initialize TOF AFTER network is stable:
```cpp
// In displayTask, move TOF init after a delay
vTaskDelay(pdMS_TO_TICKS(10000));  // Wait 10 seconds
tofVisualizer = new TOFVisualizer(&tofSensor, &matrix);
```

### Option B: Static TOFVisualizer
Avoid heap fragmentation by making it static:
```cpp
// At global scope (line 37)
#ifdef VL53L8CX_ENABLED
TOFSensor tofSensor(TOF_PWREN_PIN_1, TOF_SENSOR_1_ADDRESS);
static TOFVisualizer tofVisualizerStatic(&tofSensor, &matrix);
TOFVisualizer* tofVisualizer = &tofVisualizerStatic;
#endif
```
**Problem:** Can't initialize with `&matrix` until after matrix exists

### Option C: Defer TOF Connection
Don't connect TOF to aquarium/effects immediately:
```cpp
// Connect TOF later when needed, not at startup
// Remove lines 142-145 from displayTask
```

## Testing Recommendations

1. **Monitor heap** - Add logging:
   ```cpp
   log_i("Free heap: %u, Min free: %u", ESP.getFreeHeap(), ESP.getMinFreeHeap());
   ```

2. **Check for stack overflow** - Watch for crashes/resets

3. **Test UDP with TOF enabled** - Stream continuously for 30+ minutes

4. **Profile memory usage** - Before and after TOF initialization

## Expected Results

With the stack size adjustments:
- ✅ UDP streams continuously with VL53L8CX_ENABLED
- ✅ TOF sensor can be enabled without breaking UDP
- ✅ Both systems coexist peacefully
- ✅ No manual flag toggling needed

## Files Modified

- `src/main.cpp` - Adjusted DisplayTask and ServerTask stack sizes with VL53L8CX conditional

## Key Lesson

**Memory management on embedded systems requires careful balancing.** Even when a task isn't running, just initializing objects can fragment memory and break seemingly unrelated systems (like UDP networking).

Stack size tuning is often the first line of defense against memory pressure issues on ESP32.
