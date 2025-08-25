#pragma once

#include <Arduino.h>
#include <Matrix.h>
#include <TaskManager.h>
#include <Fonts/Font4x5Fixed.h>
#include <vector>
#include <string>
#include <StateManager.h>
#include <NetWizard.h>
#include <WebServerManager.h>

// Forward declaration
class Aquarium;

class TouchMenu {
 private:
  Matrix* matrix;
  TaskManager* taskManager;
  StateManager* stateManager;
  Aquarium* aquarium;
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

  bool showBrightnessControl = false;
  bool manualBrightnessAdjustment = false;

  CRGB activeColor = CRGB(255, 255, 255);
  CRGB inactiveColor = CRGB(128, 128, 128);
  
  bool confirmationRequired = false;
  std::string itemToConfirm;

  unsigned long lastTapTime[3] = {0, 0, 0};
  static const unsigned long DOUBLE_TAP_INTERVAL = 500; // ms

  static TouchMenu* instance;
  static TouchMenu* getInstance();

  static void gotTouch1();
  static void gotTouch2();
  static void gotTouch3();

  std::string getSensorDataMenuText() const;
  std::string getTemperatureUnitText() const;
  std::vector<std::string> getItemList() const;
  void executeMenuItem(const std::string& selectedOption);
  void handleDoubleTap(uint8_t pin);
  void handleSingleTap(uint8_t pin);
  bool startDemo = false;
  bool showWiFiInfo = false;
  WebServerManager* webServerManager = nullptr;
  void displayWiFiInfo();
  void displayConfirmation(const char* message);
  void displayBrightnessControl();

 public:
  TouchMenu(Matrix* matrix, StateManager* stateManager, WebServerManager* webServerManager, Aquarium* aquarium = nullptr, long touchThreshold = 10000);
  void setupInterrupts();
  void update();
  bool isMenuOpen();
  bool isDisplayOn();
  bool showSensorData();
  void displayMenu();
  bool shouldStartDemo();
};