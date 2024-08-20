#ifndef TOUCH_MENU_H
#define TOUCH_MENU_H

#include <Arduino.h>
#include <Matrix.h>
#include <TaskManager.h>
#include <Fonts/Font4x5Fixed.h>

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

  CRGB activeColor = CRGB(255, 255, 255);
  CRGB inactiveColor = CRGB(128, 128, 128);
  
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
    // matrix->background->setFont();
    // matrix->resetCursor();
    if (menuOpen) {
      log_i("Displaying menu");
      matrix->background->setFont(&Font4x5Fixed);
      matrix->background->setTextSize(1);
      matrix->background->clear();
      matrix->background->setCursor(0,5);
      if (confirmationRequired) {
          matrix->background->setTextColor(inactiveColor);
        if(itemToConfirm == "Factory Reset") {
          matrix->background->println("This will wipe everything, are you sure?");
        } else if(itemToConfirm == "Start WiFi") {
          matrix->background->println("This can lead to issues, are you sure?");
        } else {
          matrix->background->println("Are you sure?");
        }
        matrix->background->println();
        matrix->background->setTextColor(inactiveColor);
        matrix->background->println(itemToConfirm.c_str());
        matrix->background->println();
        matrix->background->setTextColor(currentMenuItem == 0 ? activeColor : inactiveColor);
        matrix->background->println(">No");
        matrix->background->setTextColor(currentMenuItem == 1 ? activeColor : inactiveColor);
        matrix->background->println(">No");
        matrix->background->setTextColor(currentMenuItem == 2 ? activeColor : inactiveColor);
        matrix->background->println(">Yes");
        matrix->background->setTextColor(currentMenuItem == 3 ? activeColor : inactiveColor);
        matrix->background->println(">No");
        matrix->background->setTextColor(currentMenuItem == 4 ? activeColor : inactiveColor);
        matrix->background->println(">No");

      } else {
        std::vector<std::string> currentItemList = getItemList();
        for (size_t i = 0; i < currentItemList.size(); i++) {
          if (i == currentMenuItem) {
            matrix->background->setTextColor(activeColor);
            matrix->background->println((">" + currentItemList[i]).c_str());
          } else {
            matrix->background->setTextColor(inactiveColor);
            matrix->background->println((">" + currentItemList[i]).c_str());
          }
        }
      }

      matrix->background->display();
      matrix->update();

      if (optionSelected) {
        if (confirmationRequired) {
          if (currentMenuItem == 2) { // Yes
            log_i("Confirmed: %s", itemToConfirm.c_str());
            executeMenuItem(itemToConfirm);
          }
          confirmationRequired = false;
        } else {
          std::string selectedOption = getItemList()[currentMenuItem];
          log_i("Selected option: %s", selectedOption.c_str());
          executeMenuItem(selectedOption);
        }
        optionSelected = false;
      }
    }
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