#pragma once

#include <Arduino.h>
#include <StateManager.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <map>
#include <string>

class TaskManager {
 public:
  enum class TaskState { RUNNING, SUSPENDED, STOPPED };

  static TaskManager& getInstance(StateManager* stateManager = nullptr);
  
  void createTask(const std::string& taskName, TaskFunction_t taskFunction,
                  uint32_t stackSize, UBaseType_t priority, BaseType_t coreID);

  void resumeTask(const std::string& taskName);
  void suspendTask(const std::string& taskName);
  void startTask(const std::string& taskName);
  TaskState getTaskState(const std::string& taskName);
  bool isTaskRunning(const std::string& taskName);
  StateManager* getStateManager();

 private:
  TaskManager(StateManager* stateManager);

  StateManager* _stateManager;

  struct TaskInfo {
    TaskHandle_t handle;
    TaskState state;
  };

  std::map<std::string, TaskInfo> tasks;
};