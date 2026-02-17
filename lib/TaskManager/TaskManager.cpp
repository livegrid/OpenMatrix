#include "TaskManager.h"

#include <esp_heap_caps.h>

TaskManager& TaskManager::getInstance(StateManager* stateManager) {
  static TaskManager instance(stateManager);
  return instance;
}

TaskManager::TaskManager(StateManager* stateManager)
    : _stateManager(stateManager) {}

void TaskManager::createTask(const std::string& taskName, TaskFunction_t taskFunction,
                uint32_t stackSize, UBaseType_t priority, BaseType_t coreID,
                bool usePSRAM) {
  const size_t freeHeapBefore = ESP.getFreeHeap();
  const size_t internalBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  const size_t psramBefore = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  log_i("TaskManager::createTask '%s' stack=%u bytes priority=%u core=%d %s "
        "(heap=%u, internal=%u, psram=%u)",
        taskName.c_str(), stackSize * sizeof(StackType_t), priority, coreID,
        usePSRAM ? "[PSRAM]" : "[Internal]",
        static_cast<unsigned int>(freeHeapBefore),
        static_cast<unsigned int>(internalBefore),
        static_cast<unsigned int>(psramBefore));

  TaskHandle_t taskHandle = nullptr;
  StackType_t* stackBuffer = nullptr;
  StaticTask_t* taskBuffer = nullptr;

  if (usePSRAM) {
    // Manually allocate stack in PSRAM and use static task creation
    stackBuffer = (StackType_t*)heap_caps_malloc(stackSize * sizeof(StackType_t),
                                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    taskBuffer = (StaticTask_t*)heap_caps_malloc(sizeof(StaticTask_t),
                                                   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (stackBuffer == nullptr || taskBuffer == nullptr) {
      log_e("Failed to allocate PSRAM for task '%s' stack. Heap=%u psram=%u",
            taskName.c_str(), ESP.getFreeHeap(),
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
      if (stackBuffer) heap_caps_free(stackBuffer);
      if (taskBuffer) heap_caps_free(taskBuffer);
      return;
    }

    // Create static task with PSRAM-allocated stack
    taskHandle = xTaskCreateStaticPinnedToCore(
        taskFunction, taskName.c_str(), stackSize, NULL, priority,
        stackBuffer, taskBuffer, coreID);
  } else {
    // Allocate task stack in internal RAM (default, faster)
    BaseType_t result = xTaskCreatePinnedToCore(
        taskFunction, taskName.c_str(), stackSize, NULL, priority,
        &taskHandle, coreID);

    if (result != pdPASS || taskHandle == nullptr) {
      log_e("Failed to create task '%s' in internal RAM (err=%ld). Heap=%u internal=%u",
            taskName.c_str(), static_cast<long>(result), ESP.getFreeHeap(),
            heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
      return;
    }
  }

  if (taskHandle == nullptr) {
    log_e("Failed to create task '%s'. Heap now=%u internal=%u psram=%u",
          taskName.c_str(), ESP.getFreeHeap(),
          heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
          heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return;
  }

  tasks[taskName] = {taskHandle, TaskState::RUNNING, stackBuffer, taskBuffer};

  log_i("TaskManager::createTask '%s' created successfully. Heap now=%u "
        "(diff=%ld) internal=%u (diff=%ld) psram=%u (diff=%ld)",
        taskName.c_str(), ESP.getFreeHeap(),
        static_cast<long>(ESP.getFreeHeap() - freeHeapBefore),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        static_cast<long>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) - internalBefore),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        static_cast<long>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) - psramBefore));
}

void TaskManager::resumeTask(const std::string& taskName) {
  if (tasks.find(taskName) != tasks.end() &&
      tasks[taskName].state == TaskState::SUSPENDED) {
    vTaskResume(tasks[taskName].handle);
    tasks[taskName].state = TaskState::RUNNING;
  }
}

void TaskManager::suspendTask(const std::string& taskName) {
  if (tasks.find(taskName) != tasks.end()) {
    vTaskSuspend(tasks[taskName].handle);
    tasks[taskName].state = TaskState::SUSPENDED;
  }
}

void TaskManager::startTask(const std::string& taskName) {
  if (tasks.find(taskName) != tasks.end() &&
      tasks[taskName].state == TaskState::SUSPENDED) {
    vTaskResume(tasks[taskName].handle);
    tasks[taskName].state = TaskState::RUNNING;
  }
}

TaskManager::TaskState TaskManager::getTaskState(const std::string& taskName) {
  if (tasks.find(taskName) != tasks.end()) {
    return tasks[taskName].state;
  }
  return TaskState::SUSPENDED;  // Return SUSPENDED if task not found
}

bool TaskManager::isTaskRunning(const std::string& taskName) {
  return tasks.find(taskName) != tasks.end() &&
         tasks[taskName].state == TaskState::RUNNING;
}

StateManager* TaskManager::getStateManager() {
  return _stateManager;
}