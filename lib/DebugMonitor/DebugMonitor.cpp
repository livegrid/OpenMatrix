#include "DebugMonitor.h"

void DebugMonitor::init() {
    TaskManager& taskManager = TaskManager::getInstance();
    taskManager.createTask("DebugMonitor", debugTask, STACK_SIZE, PRIORITY, CORE_ID);
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
        float heapPercentage = ((float)freeHeap / totalHeap) * 100;

        // Print system information
        log_e("=== Debug Information ===");
        log_e("Free Heap: %u bytes (%.1f%%)", freeHeap, heapPercentage);
        log_e("CPU Frequency: %u MHz", ESP.getCpuFreqMHz());
        log_e("Minimum Free Heap: %u bytes", ESP.getMinFreeHeap());
        
        // Print task information
        log_e("\n=== Task Information ===");
        log_e("Task Name            State     Stack High Water  Core  Priority");
        log_e("------------------------------------------------------------");
        
        // Iterate through all tasks in TaskManager
        for (const auto& task : taskManager.tasks) {
            const std::string& taskName = task.first;
            TaskHandle_t handle = task.second.handle;
            
            // Get detailed task information
            UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(handle);
            eTaskState runtimeState = eTaskGetState(handle);
            UBaseType_t priority = uxTaskPriorityGet(handle);
            BaseType_t core = xTaskGetAffinity(handle);

            log_e("%-20s %-9s %6u bytes    %2d    %3u", 
                taskName.c_str(),
                getTaskStateString(runtimeState),
                stackHighWater * sizeof(StackType_t),
                core,
                priority
            );
        }
        
        log_e("======================\n");

        vTaskDelay(pdMS_TO_TICKS(DEBUG_INTERVAL_MS));
    }
}