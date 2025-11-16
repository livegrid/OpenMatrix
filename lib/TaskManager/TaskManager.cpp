#include "TaskManager.h"

#include <esp_heap_caps.h>

TaskManager& TaskManager::getInstance(StateManager* stateManager) {
  static TaskManager instance(stateManager);
  return instance;
}

TaskManager::TaskManager(StateManager* stateManager)
    : _stateManager(stateManager) {}

void TaskManager::createTask(const std::string& taskName, TaskFunction_t taskFunction,
                uint32_t stackSize, UBaseType_t priority, BaseType_t coreID) {
  const size_t freeHeapBefore = ESP.getFreeHeap();
  const size_t internalBefore = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  log_i("TaskManager::createTask '%s' stack=%u bytes priority=%u core=%d "
        "(heap=%u, internal=%u)",
        taskName.c_str(), stackSize * sizeof(StackType_t), priority, coreID,
        static_cast<unsigned int>(freeHeapBefore),
        static_cast<unsigned int>(internalBefore));

  TaskHandle_t taskHandle;
  BaseType_t result = xTaskCreatePinnedToCore(taskFunction, taskName.c_str(),
                                              stackSize, NULL, priority,
                                              &taskHandle, coreID);
  if (result != pdPASS || taskHandle == nullptr) {
    log_e("Failed to create task '%s' (err=%ld). Heap now=%u internal=%u",
          taskName.c_str(), static_cast<long>(result), ESP.getFreeHeap(),
          heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    return;
  }

  tasks[taskName] = {taskHandle, TaskState::RUNNING};

  log_i("TaskManager::createTask '%s' created successfully. Heap now=%u "
        "(diff=%ld) internal=%u (diff=%ld)",
        taskName.c_str(), ESP.getFreeHeap(),
        static_cast<long>(ESP.getFreeHeap() - freeHeapBefore),
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        static_cast<long>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) -
                          internalBefore));
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