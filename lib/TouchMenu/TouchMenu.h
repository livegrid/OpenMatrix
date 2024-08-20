#ifndef TOUCH_MENU_H
#define TOUCH_MENU_H

#include <Arduino.h>
#include <Matrix.h>
#include <TaskManager.h>

class TouchMenu {
 private:
  Matrix* matrix;
  TaskManager* taskManager;
  long touchThreshold;
  bool menuOpen = false;
  bool sensorDataVisible = false;
  bool turnOff = false;
  bool WiFiInfoVisible = false;
  bool WiFiConnected = false;
  uint8_t currentMenuItem = 0;
  bool optionSelected = false;
  bool touch1detected = false;
  bool touch2detected = false;
  bool touch3detected = false;
  
  bool confirmationRequired = false;
  std::string itemToConfirm;

  std::string getSensorDataMenuText() const {
    return sensorDataVisible ? "Hide SensorData" : "Show SensorData";
  }

  std::vector<std::string> getItemList() const {
    std::vector<std::string> baseList = {"Go Back", getSensorDataMenuText(),"Turn Off", "Factory Reset"};
    if (WiFiConnected) {
      baseList.insert(baseList.end() - 1, {"Stop WiFi", "Show WiFi Info"});
    } else {
      baseList.insert(baseList.end() - 1, "Start WiFi");
    }
    return baseList;
  }

  unsigned long lastTapTime[3] = {0, 0, 0};
  static const unsigned long DOUBLE_TAP_INTERVAL = 500; // ms

  static TouchMenu* instance;
  static TouchMenu* getInstance() { return instance; }

  static void gotTouch1() { getInstance()->touch1detected = true; }
  static void gotTouch2() { getInstance()->touch2detected = true; }
  static void gotTouch3() { getInstance()->touch3detected = true; }

  void executeMenuItem(const std::string& selectedOption) {
    if (selectedOption == "Go Back") {
      menuOpen = false;
      taskManager->startTask("EffectsTask");
    } else if (selectedOption == "Show SensorData" || selectedOption == "Hide SensorData") {
      sensorDataVisible = !sensorDataVisible;
    } else if (selectedOption == "Start WiFi") {
      // Add action for Start WiFi
    } else if (selectedOption == "Turn Off") {
      menuOpen = false;
      turnOff = true;
    } else if (selectedOption == "Stop WiFi") {
      // Add action for Stop WiFi
    } else if (selectedOption == "Show WiFi Info") {
      // Add action for Show WiFi Info
    }  else if (selectedOption == "Factory Reset") {
      // Add action for Full Reset
    } else {
      // Optional: handle unknown case
    }
  }

  void handleDoubleTap(uint8_t pin) {
    if(!menuOpen) {
      menuOpen = true;
      if(taskManager->isTaskRunning("EffectsTask")) {
        taskManager->suspendTask("EffectsTask");
      }
    }
    log_i(" --- T%d Double Tap", pin - 10);
  }

  void handleSingleTap(uint8_t pin) {
    if (menuOpen) {
      std::vector<std::string> currentItemList = getItemList();
      if (confirmationRequired) {
        if (pin == 11) {
          currentMenuItem = (currentMenuItem - 1 + 5) % 5; // 5 options in confirmation menu
        } else if (pin == 13) {
          currentMenuItem = (currentMenuItem + 1) % 5;
        } else if (pin == 12) {
          optionSelected = true;
        }
      } else {
        if (pin == 11) {
          currentMenuItem = (currentMenuItem - 1 + currentItemList.size()) % currentItemList.size();
        } else if (pin == 13) {
          currentMenuItem = (currentMenuItem + 1) % currentItemList.size();
        } else if (pin == 12) {
          if (currentItemList[currentMenuItem] == "Stop WiFi" ||
              currentItemList[currentMenuItem] == "Start WiFi" ||
              currentItemList[currentMenuItem] == "Factory Reset") {
            confirmationRequired = true;
            itemToConfirm = currentItemList[currentMenuItem];
            currentMenuItem = 0; // Default to "No"
          } else {
            optionSelected = true;
          }
        }
      }
    }
    log_i(" --- T%d Single Tap", pin - 10);
  }
  
  void displayMenu() {
    // matrix->setFont(1);
    // matrix->resetCursor();
    // if (menuOpen) {
    //   matrix->clearScreen();
    //   if (confirmationRequired) {
    //     if(itemToConfirm == "Factory Reset") {
    //       matrix->background->drawText("This will wipe everything, are you sure?", 128, 128, 128);
    //     } else if(itemToConfirm == "Start WiFi") {
    //       matrix->drawText("This can lead to issues, are you sure?", 128, 128, 128);
    //     } else {
    //       matrix->drawText("Are you sure?", 128, 128, 128);
    //     }
    //     matrix->newLine();
    //     matrix->drawText(itemToConfirm, 128, 128, 128);
    //     matrix->newLine();
    //     matrix->drawText(">No", currentMenuItem == 0 ? 255 : 128, currentMenuItem == 0 ? 255 : 128, currentMenuItem == 0 ? 255 : 128);
    //     matrix->newLine();
    //     matrix->drawText(">No", currentMenuItem == 1 ? 255 : 128, currentMenuItem == 1 ? 255 : 128, currentMenuItem == 1 ? 255 : 128);
    //     matrix->newLine();
    //     matrix->drawText(">Yes", currentMenuItem == 2 ? 255 : 128, currentMenuItem == 2 ? 255 : 128, currentMenuItem == 2 ? 255 : 128);
    //     matrix->newLine();
    //     matrix->drawText(">No", currentMenuItem == 3 ? 255 : 128, currentMenuItem == 3 ? 255 : 128, currentMenuItem == 3 ? 255 : 128);
    //     matrix->newLine();
    //     matrix->drawText(">No", currentMenuItem == 4 ? 255 : 128, currentMenuItem == 4 ? 255 : 128, currentMenuItem == 4 ? 255 : 128);
    //   } else {
    //     std::vector<std::string> currentItemList = getItemList();
    //     for (size_t i = 0; i < currentItemList.size(); i++) {
    //       if (i == currentMenuItem) {
    //         matrix->drawText(">" + currentItemList[i], 255, 255, 255);  // White for selected item
    //       } else {
    //         matrix->drawText(">" + currentItemList[i], 128, 128, 128);  // Grey for other items
    //       }
    //       matrix->newLine();
    //     }
    //   }
    //   if (optionSelected) {
    //     if (confirmationRequired) {
    //       if (currentMenuItem == 2) { // Yes
    //         log_i("Confirmed: %s", itemToConfirm.c_str());
    //         executeMenuItem(itemToConfirm);
    //       }
    //       confirmationRequired = false;
    //     } else {
    //       std::string selectedOption = getItemList()[currentMenuItem];
    //       log_i("Selected option: %s", selectedOption.c_str());
    //       executeMenuItem(selectedOption);
    //     }
    //     optionSelected = false;
    //   }
    // }
    // matrix->update();
  }


 public:
  TouchMenu(Matrix* matrix, TaskManager* taskManager, long touchThreshold = 10000) : matrix(matrix), taskManager(taskManager), touchThreshold(touchThreshold) {
    instance = this;
  }

  void setupInterrupts() {
    touchAttachInterrupt(11, gotTouch1, touchThreshold);
    touchAttachInterrupt(12, gotTouch2, touchThreshold);
    touchAttachInterrupt(13, gotTouch3, touchThreshold);
  }

  void update() {
    unsigned long now = millis();

    for (int i = 0; i < 3; i++) {
      bool* touchDetected = nullptr;
      uint8_t pin = 11 + i;

      switch (i) {
        case 0: touchDetected = &touch1detected; break;
        case 1: touchDetected = &touch2detected; break;
        case 2: touchDetected = &touch3detected; break;
      }

      if (*touchDetected) {
        *touchDetected = false;
        if (touchInterruptGetLastStatus(pin)) {
          log_i(" --- T%d Touched", i + 1);
          
          if (now - lastTapTime[i] < DOUBLE_TAP_INTERVAL) {
            handleDoubleTap(pin);
          } else {
            handleSingleTap(pin);
          }
          if(menuOpen) {
            displayMenu();
          }
          lastTapTime[i] = now;
        } else {
          log_i(" --- T%d Released", i + 1);
        }
      }
    }
  }

  bool isMenuOpen() {
    return menuOpen;
  }

  bool isDisplayOn() {
    return turnOff;
  }

  bool showSensorData() {
    return sensorDataVisible;
  }

  bool showWiFiInfo() {
    return WiFiInfoVisible;
  }
};

TouchMenu* TouchMenu::instance = nullptr;

#endif  // TOUCH_MENU_H