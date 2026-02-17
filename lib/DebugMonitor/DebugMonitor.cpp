#include "DebugMonitor.h"
#include <esp_heap_caps.h>

void DebugMonitor::init() {
    TaskManager& taskManager = TaskManager::getInstance();
    taskManager.createTask("DebugMonitor", debugTask, STACK_SIZE, PRIORITY, CORE_ID, true);
}

const char* DebugMonitor::getTaskStateString(TaskManager::TaskState state) {
    switch (state) {
        case TaskManager::TaskState::RUNNING:
            return "RUNNING";
        case TaskManager::TaskState::SUSPENDED:
            return "SUSPENDED";
        case TaskManager::TaskState::STOPPED:
            return "STOPPED";
        default:
            return "UNKNOWN";
    }
}

const char* DebugMonitor::getTaskStateString(eTaskState state) {
    switch (state) {
        case eTaskState::eRunning:
            return "Running";
        case eTaskState::eReady:
            return "Ready";
        case eTaskState::eBlocked:
            return "Blocked";
        case eTaskState::eSuspended:
            return "Suspended";
        case eTaskState::eDeleted:
            return "Deleted";
        default:
            return "Unknown";
    }
}

void DebugMonitor::debugTask(void* parameters) {
    TaskManager& taskManager = TaskManager::getInstance();
    
    while (true) {
        // Get heap information
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t totalHeap = ESP.getHeapSize();
        float heapPercentage = totalHeap > 0 ? ((float)freeHeap / totalHeap) * 100 : 0;
        size_t internalFree = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t internalLargest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        size_t psramFree = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t psramLargest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

        // Print system information
        log_i("=== Memory & Tasks ===");
        log_i("Heap: free=%u (%.1f%%) minFree=%u | internal free=%u largest=%u | PSRAM free=%u largest=%u",
              (unsigned)freeHeap, heapPercentage, (unsigned)ESP.getMinFreeHeap(),
              (unsigned)internalFree, (unsigned)internalLargest, (unsigned)psramFree, (unsigned)psramLargest);
        log_i("Task Name            State     Stack HWM   Core  Pri");
        log_i("----------------------------------------------------");

        for (const auto& task : taskManager.tasks) {
            const std::string& taskName = task.first;
            TaskHandle_t handle = task.second.handle;
            UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(handle);
            eTaskState runtimeState = eTaskGetState(handle);
            UBaseType_t priority = uxTaskPriorityGet(handle);
            BaseType_t core = xTaskGetAffinity(handle);
            log_i("%-20s %-9s %6u b   %2d   %3u",
                taskName.c_str(), getTaskStateString(runtimeState),
                (unsigned)(stackHighWater * sizeof(StackType_t)), (int)core, (unsigned)priority);
        }
        log_i("======================\n");

        vTaskDelay(pdMS_TO_TICKS(DEBUG_INTERVAL_MS));
    }
}