#include "TaskManager.h"

TaskManager& TaskManager::getInstance(StateManager* stateManager) {
  static TaskManager instance(stateManager);
  return instance;
}

TaskManager::TaskManager(StateManager* stateManager)
    : _stateManager(stateManager) {}

void TaskManager::createTask(const std::string& taskName, TaskFunction_t taskFunction,
                uint32_t stackSize, UBaseType_t priority, BaseType_t coreID) {
  TaskHandle_t taskHandle;
  xTaskCreatePinnedToCore(taskFunction, taskName.c_str(), stackSize, NULL,
                          priority, &taskHandle, coreID);
  tasks[taskName] = {taskHandle, TaskState::RUNNING};
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