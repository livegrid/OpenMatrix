#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <map>
#include <string>

class TaskManager {
public:
    enum class TaskState {
        RUNNING,
        PAUSED,
        SUSPENDED
    };

    static TaskManager& getInstance() {
        static TaskManager instance;
        return instance;
    }

    void createTask(const std::string& taskName, TaskFunction_t taskFunction, uint32_t stackSize, UBaseType_t priority, BaseType_t coreID) {
        TaskHandle_t taskHandle;
        xTaskCreatePinnedToCore(taskFunction, taskName.c_str(), stackSize, NULL, priority, &taskHandle, coreID);
        tasks[taskName] = {taskHandle, TaskState::RUNNING};
    }

    void pauseTask(const std::string& taskName) {
        if (tasks.find(taskName) != tasks.end()) {
            vTaskSuspend(tasks[taskName].handle);
            tasks[taskName].state = TaskState::PAUSED;
        }
    }

    void resumeTask(const std::string& taskName) {
        if (tasks.find(taskName) != tasks.end() && tasks[taskName].state == TaskState::PAUSED) {
            vTaskResume(tasks[taskName].handle);
            tasks[taskName].state = TaskState::RUNNING;
        }
    }

    void suspendTask(const std::string& taskName) {
        if (tasks.find(taskName) != tasks.end()) {
            vTaskSuspend(tasks[taskName].handle);
            tasks[taskName].state = TaskState::SUSPENDED;
        }
    }

    void startTask(const std::string& taskName) {
        if (tasks.find(taskName) != tasks.end() && tasks[taskName].state == TaskState::SUSPENDED) {
            vTaskResume(tasks[taskName].handle);
            tasks[taskName].state = TaskState::RUNNING;
        }
    }

    TaskState getTaskState(const std::string& taskName) {
        if (tasks.find(taskName) != tasks.end()) {
            return tasks[taskName].state;
        }
        return TaskState::SUSPENDED;  // Return SUSPENDED if task not found
    }

    bool isTaskRunning(const std::string& taskName) {
        return tasks.find(taskName) != tasks.end() && tasks[taskName].state == TaskState::RUNNING;
    }

private:
    TaskManager() {}  // Private constructor for singleton
    
    struct TaskInfo {
        TaskHandle_t handle;
        TaskState state;
    };
    
    std::map<std::string, TaskInfo> tasks;
};

#endif // TASK_MANAGER_H