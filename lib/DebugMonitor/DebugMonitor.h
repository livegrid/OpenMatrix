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
    static const uint32_t STACK_SIZE = 3072;  // HWM ~2484 B
    static const UBaseType_t PRIORITY = 1;
    static const BaseType_t CORE_ID = 0;
    static const uint32_t DEBUG_INTERVAL_MS = 15000;  // 15 seconds
    static const char* getTaskStateString(TaskManager::TaskState state);
    static const char* getTaskStateString(eTaskState state);
}; 