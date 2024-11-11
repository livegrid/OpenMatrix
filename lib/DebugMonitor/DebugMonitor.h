#pragma once

#include <Arduino.h>
#include <TaskManager.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class DebugMonitor {
public:
    static void init();
    static void debugTask(void* parameters);

private:
    static const uint32_t STACK_SIZE = 4096;
    static const UBaseType_t PRIORITY = 1;
    static const BaseType_t CORE_ID = 0;
    static const uint32_t DEBUG_INTERVAL_MS = 5000;  // 5 seconds
    static const char* getTaskStateString(TaskManager::TaskState state);
    static const char* getTaskStateString(eTaskState state);
}; 