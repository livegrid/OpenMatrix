#include "TouchMenu.h"
#include "Aquarium.h"

TouchMenu* TouchMenu::instance = nullptr;

TouchMenu* TouchMenu::getInstance() { return instance; }

void TouchMenu::gotTouch1() { getInstance()->touch1detected = true; }
void TouchMenu::gotTouch2() { getInstance()->touch2detected = true; }

TouchMenu::TouchMenu(Matrix* matrix, StateManager* stateManager, WebServerManager* webServerManager, Aquarium* aquarium, long touchThreshold)
    : matrix(matrix), stateManager(stateManager), webServerManager(webServerManager), aquarium(aquarium), touchThreshold(touchThreshold) {
  instance = this;
}

std::string TouchMenu::getSensorDataMenuText() const {
  return sensorDataVisible ? "Hide SensorData" : "Show SensorData";
}

std::string TouchMenu::getTemperatureUnitText() const {
  TemperatureUnit currentUnit = stateManager->getState()->temperatureUnit;
  log_i("Getting temperature unit text. Current unit: %d", (int)currentUnit);
  return currentUnit == TemperatureUnit::CELSIUS ? "Temp> Celsius" : "Temp> Fahrenheit";
}

std::vector<std::string> TouchMenu::getItemList() const {
  std::vector<std::string> baseList = {"Go Back","WiFi Info", getSensorDataMenuText(), getTemperatureUnitText(), "Brightness", "Start Demo", "Turn Off", "Factory Reset"};
  return baseList;
}

void TouchMenu::executeMenuItem(const std::string& selectedOption) {
  if (selectedOption == "Go Back") {
    menuOpen = false;
  } else if (selectedOption == "Show SensorData" || selectedOption == "Hide SensorData") {
    sensorDataVisible = !sensorDataVisible;
  } else if (selectedOption == "Temp> Celsius" || selectedOption == "Temp> Fahrenheit") {
    // Log current state
    log_i("Current temperature unit: %d", (int)stateManager->getState()->temperatureUnit);
    TemperatureUnit newUnit = (selectedOption == "Temp> Fahrenheit") ? TemperatureUnit::CELSIUS : TemperatureUnit::FAHRENHEIT;
    stateManager->getState()->temperatureUnit = newUnit;
    stateManager->save();
    log_i("New temperature unit set to: %d", (int)stateManager->getState()->temperatureUnit);
  } else if (selectedOption == "Start Demo") {
    menuOpen = false;
    startDemo = true;
    currentMenuItem = 0;
  } else if (selectedOption == "Turn Off") {
    menuOpen = false;
    stateManager->getState()->power = false;
    currentMenuItem = 0;
  } else if (selectedOption == "WiFi Info") {
    showWiFiInfo = true;
    currentMenuItem = 0;
  } else if (selectedOption == "Brightness") {
    showBrightnessControl = true;
    currentMenuItem = 0;
  }
}

void TouchMenu::handleDoubleTap(uint8_t pin) {
  if (stateManager->getState()->power == false) {
    stateManager->getState()->power = true;
    return;
  }
  if(!menuOpen) {
    menuOpen = true;
  }
  log_i(" --- T%d Double Tap", pin - 10);
}

void TouchMenu::handleSingleTap(uint8_t pin) {
  if (!stateManager->getState()->power) {
    stateManager->getState()->power = true;
    stateManager->save();
    return;
  }
  if (menuOpen) {
    if (showBrightnessControl) {
      if (pin == 11) { // Use pin 11 to adjust brightness
        if (currentMenuItem == 1) { // Toggle between Auto and Manual
          stateManager->getState()->autobrightness = !stateManager->getState()->autobrightness;
          stateManager->save();
        } else if (currentMenuItem == 2 && !stateManager->getState()->autobrightness) { // Only adjust brightness if brightness control is selected and in manual mode
          int newBrightness = stateManager->getState()->brightness + 10;
          if (newBrightness > 255) newBrightness = 50; // Wrap around to 50 when it reaches over 255
          stateManager->getState()->brightness = newBrightness;
          matrix->setBrightness(newBrightness);
        } else if (currentMenuItem == 0) { // Go Back
          showBrightnessControl = false;
          currentMenuItem = 0;
        }
      } else if (pin == 13) { // Use pin 13 to scroll through the menu
        currentMenuItem = (currentMenuItem + 1) % (stateManager->getState()->autobrightness ? 2 : 3); // Toggle between Go Back, Auto/Manual, and Adjust if in manual mode
      }
      return;
    }
    if (showWiFiInfo) {
      if (confirmationRequired) {
        if (pin == 13) {
          currentMenuItem = (currentMenuItem + 1) % 5; // Scroll through all 5 options
        } else if (pin == 11) {
          optionSelected = true;
        }
      }
      else {
        if (pin == 13) {
          currentMenuItem = (currentMenuItem + 1) % 2; // Toggle between Go Back and Reset WiFi
        } else if (pin == 11) {
          if (currentMenuItem == 0) { // Go Back
            showWiFiInfo = false;
          } else if (currentMenuItem == 1) { // Reset WiFi
            confirmationRequired = true;
            itemToConfirm = "Reset WiFi Settings?";
            currentMenuItem = 0; // Default to "No"
          }
        }
      }
      return;
    }
    
    std::vector<std::string> currentItemList = getItemList();
    if (confirmationRequired) {
      if (pin == 13) {
        currentMenuItem = (currentMenuItem + 1) % 5; // 2 options in confirmation menu
      } else if (pin == 11) {
        optionSelected = true;
      }
    } else {
      if (pin == 13) {
        currentMenuItem = (currentMenuItem + 1) % currentItemList.size();
      } else if (pin == 11) {
        if (currentItemList[currentMenuItem] == "Factory Reset") {
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
  }
  log_i(" --- T%d Single Tap", pin == 11 ? 1 : 2);
}

void TouchMenu::displayMenu() {
  matrix->background->setFont(&Font4x5Fixed);
  matrix->background->setTextSize(1);
  matrix->background->clear();
  matrix->background->setCursor(0,5);
  
  if (showWiFiInfo) {
    if (confirmationRequired) {
      displayConfirmation("Reset WiFi?");
    } else {
      displayWiFiInfo();
    }
  } else if (showBrightnessControl) {
    displayBrightnessControl();
  } else if (confirmationRequired) {
    displayConfirmation("Factory Reset?");
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

  if (optionSelected) {
    if (confirmationRequired) {
      if (currentMenuItem == 2) { // Yes is now the middle option
        if (itemToConfirm == "Factory Reset") {
          webServerManager->nw.reset();
          LittleFS.remove("/aquarium_state.json");
          LittleFS.remove("/state.json");
          ESP.restart();
        } else if (itemToConfirm == "Reset WiFi Settings?") {
          webServerManager->nw.reset();
          ESP.restart();
        }
      }
      confirmationRequired = false;
      itemToConfirm = "";
      showWiFiInfo = false; // Ensure we exit WiFi info after confirmation
      currentMenuItem = 0;
    } else {
      std::string selectedOption = getItemList()[currentMenuItem];
      log_i("Selected option: %s", selectedOption.c_str());
      executeMenuItem(selectedOption);
      if (selectedOption != "Go Back") {
        currentMenuItem = 0; // Reset to "Go Back" after executing any other option
      }
    }
    optionSelected = false;
  }
}

void TouchMenu::displayConfirmation(const char* message) {
  matrix->background->setTextColor(inactiveColor);
  matrix->background->println(message);
  matrix->background->println("");
  
  const char* options[] = {"No", "No", "Yes", "No", "No"};
  for (int i = 0; i < 5; i++) {
    matrix->background->setTextColor(currentMenuItem == i ? activeColor : inactiveColor);
    matrix->background->print(">");
    matrix->background->println(options[i]);
  }
}


void TouchMenu::displayWiFiInfo() {
  matrix->background->setTextColor(inactiveColor);
  matrix->background->println("WiFi Info:");
  
  // Get WiFi information from NetWizard
  NetWizard* nw = &webServerManager->nw;
  NetWizardConnectionStatus status = nw->getConnectionStatus();
  
  switch (status) {
    case NetWizardConnectionStatus::CONNECTED:
      matrix->background->setTextColor(activeColor);
      matrix->background->println("Status: Connected");
      matrix->background->println(("IP: " + nw->localIP().toString()).c_str());
      matrix->background->setTextColor(inactiveColor);
      matrix->background->println(("SSID: " + String(nw->getSSID())).c_str());
      break;
    case NetWizardConnectionStatus::DISCONNECTED:
      matrix->background->setTextColor(activeColor);
      matrix->background->println("Status: Disconnected");
      matrix->background->setTextColor(inactiveColor);
      break;
    case NetWizardConnectionStatus::CONNECTING:
      matrix->background->setTextColor(activeColor);
      matrix->background->println("Status: Connecting...");
      matrix->background->setTextColor(inactiveColor);
      break;
    default:
      matrix->background->setTextColor(activeColor);
      matrix->background->println("Status: Unknown");
      matrix->background->setTextColor(inactiveColor);
      break;
  }
  
  matrix->background->println("");
  matrix->background->setTextColor(currentMenuItem == 0 ? activeColor : inactiveColor);
  matrix->background->println(">Go Back");
  matrix->background->setTextColor(currentMenuItem == 1 ? activeColor : inactiveColor);
  matrix->background->println(">Reset WiFi");
}

void TouchMenu::displayBrightnessControl() {
  matrix->background->setTextColor(inactiveColor);
  matrix->background->println("Brightness:");
  matrix->background->println("");

  // Add Go Back option
  matrix->background->setTextColor(currentMenuItem == 0 ? activeColor : inactiveColor);
  matrix->background->print(">");
  matrix->background->println("Go Back");

  // Show either Auto or Manual based on the current state
  const char* modeOption = stateManager->getState()->autobrightness ? "Auto" : "Manual";
  matrix->background->setTextColor(currentMenuItem == 1 ? activeColor : inactiveColor);
  matrix->background->print(">");
  matrix->background->println(modeOption);

  // Show brightness adjustment only if in manual mode
  if (!stateManager->getState()->autobrightness) {
    matrix->background->setTextColor(currentMenuItem == 2 ? activeColor : inactiveColor);
    matrix->background->print(">");
    matrix->background->println("Adjust");

    int brightness = stateManager->getState()->brightness;
    matrix->background->setTextColor(activeColor);
    matrix->background->println("");
    matrix->background->println(("Current: " + String(brightness) + "/255").c_str());
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
        
        // Handle food functionality for pin 13 when menu is not open
        if (pin == 13 && !menuOpen && aquarium) {
          aquarium->onTouchStarted();
        } else {
          handleSingleTap(pin);
        }
        lastTapTime[i] = now;
      } else {
        log_i(" --- T%d Released", i + 1);
        
        // Handle food release for pin 13 when menu is not open
        if (pin == 13 && !menuOpen && aquarium) {
          aquarium->onTouchReleased();
        }
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

bool TouchMenu::shouldStartDemo() {
  if (startDemo) {
    startDemo = false;
    return true;
  }
  return false;
}