#include "TouchMenu.h"

TouchMenu* TouchMenu::instance = nullptr;

TouchMenu* TouchMenu::getInstance() { return instance; }

void TouchMenu::gotTouch1() { getInstance()->touch1detected = true; }
void TouchMenu::gotTouch2() { getInstance()->touch2detected = true; }

TouchMenu::TouchMenu(Matrix* matrix, TaskManager* taskManager, StateManager* stateManager, long touchThreshold)
    : matrix(matrix), taskManager(taskManager), stateManager(stateManager), touchThreshold(touchThreshold) {
  instance = this;
}

std::string TouchMenu::getSensorDataMenuText() const {
  return sensorDataVisible ? "Hide SensorData" : "Show SensorData";
}

std::vector<std::string> TouchMenu::getItemList() const {
  std::vector<std::string> baseList = {"Go Back", getSensorDataMenuText(), "Turn Off", "Factory Reset"};
  if (WiFiConnected) {
    baseList.insert(baseList.end() - 1, {"Stop WiFi", "Show WiFi Info"});
  } else {
    baseList.insert(baseList.end() - 1, "Start WiFi");
  }
  return baseList;
}

void TouchMenu::executeMenuItem(const std::string& selectedOption) {
  if (selectedOption == "Go Back") {
    menuOpen = false;
    taskManager->resumeTask("DisplayTask");
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
  } else if (selectedOption == "Factory Reset") {
    // Add action for Full Reset
  } else {
    // Optional: handle unknown case
  }
}

void TouchMenu::handleDoubleTap(uint8_t pin) {
  if(!menuOpen) {
    menuOpen = true;
    taskManager->suspendTask("DisplayTask");
  }
  log_i(" --- T%d Double Tap", pin - 10);
}
void TouchMenu::handleSingleTap(uint8_t pin) {
  if (menuOpen) {
    std::vector<std::string> currentItemList = getItemList();
    if (confirmationRequired) {
      if (pin == 13) {
        currentMenuItem = (currentMenuItem + 1) % 5; // 5 options in confirmation menu
      } else if (pin == 11) {
        optionSelected = true;
      }
    } else {
      if (pin == 13) {
        currentMenuItem = (currentMenuItem + 1) % currentItemList.size();
      } else if (pin == 11) {
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
  } else if (pin == 11) {
    menuOpen = true;
    taskManager->suspendTask("DisplayTask");
  }
  log_i(" --- T%d Single Tap", pin == 11 ? 1 : 2);
}

void TouchMenu::displayMenu() {
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

void TouchMenu::setupInterrupts() {
  touchAttachInterrupt(11, gotTouch1, touchThreshold);
  touchAttachInterrupt(13, gotTouch2, touchThreshold);
}

void TouchMenu::update() {
  unsigned long now = millis();

  for (int i = 0; i < 2; i++) {
    bool* touchDetected = nullptr;
    uint8_t pin = 11 + i * 2; // This will give us pins 11 and 13

    switch (i) {
      case 0: touchDetected = &touch1detected; break;
      case 1: touchDetected = &touch2detected; break;
    }

    if (*touchDetected) {
      *touchDetected = false;
      if (touchInterruptGetLastStatus(pin)) {
        log_i(" --- T%d Touched", i + 1);
        
        handleSingleTap(pin);
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

bool TouchMenu::isMenuOpen() {
  return menuOpen;
}

bool TouchMenu::isDisplayOn() {
  return turnOff;
}

bool TouchMenu::showSensorData() {
  return sensorDataVisible;
}

bool TouchMenu::showWiFiInfo() {
  return WiFiInfoVisible;
}