#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <Arduino.h>
#include <Matrix.h>
#include <scd40.h>
#include <vector>
#include "AquariumSettings.h"
#include "Fish.h"
#include "Food.h"
#include "Plants.h"
#include "Water.h"
#include "BoidManager.h"
#include <Fonts\Font4x7Fixed.h>

class Aquarium {
 private:
  Matrix* matrix;
  SCD40* scd40;
  Water water;
  std::vector<std::unique_ptr<Fish>> fishArray;
  std::vector<std::unique_ptr<Plants>> plantArray;
  std::vector<std::unique_ptr<Food>> foodArray;
  BoidManager boidManager;
  
  // Demo settings
  bool demoMode;
  int demoStep;
  unsigned long demoStartTime;
  float demoTemperature;
  float demoHumidity;
  float demoCO2;

 public:
  // Constructor that initializes the matrix and water
  Aquarium(Matrix* m, SCD40* s) : matrix(m), water(matrix), scd40(s), boidManager(m), demoMode(false), demoStep(0) {
    initializeFish();
    initializePlants();
    boidManager.initializeBoids();
  }

  void startDemo() {
    demoMode = true;
    demoStep = 0;
    demoStartTime = millis();
    demoTemperature = 25.0f;
    demoHumidity = 50.0f;
    demoCO2 = 400.0f;
  }

  void updateDemo() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - demoStartTime;

    // Each step lasts 60 seconds
    const unsigned long stepDuration = 20000;
    
    // Calculate the current demo step
    demoStep = (elapsedTime / stepDuration) % 4;

    // Update demo values based on the current step
    switch (demoStep) {
      case 0: // Temperature swing
        demoTemperature = 25.0f + 25.0f * sin(2 * PI * elapsedTime / stepDuration);
        break;
      case 1: // Humidity swing
        demoHumidity = 50.0f + 40.0f * sin(2 * PI * elapsedTime / stepDuration);
        break;
      case 2: // CO2 swing
        demoCO2 = 400.0f + 1600.0f * sin(2 * PI * elapsedTime / stepDuration);
        break;
      case 3: // Return to normal
        demoMode = false;
        break;
    }
  }

  // Initialize fish and store them in a vector of unique pointers
  void initializeFish() {
    PVector centerPos(matrix->getXResolution() / 2,
                      matrix->getYResolution() / 2);
    for (int i = 0; i < NUM_FISH_START; i++) {
      fishArray.emplace_back(std::make_unique<Fish>(
          matrix, centerPos, random(0, 100) / 100.0));
    }
  }

  // Initialize plants and store them in a vector of unique pointers
  void initializePlants() {
    for (int i = 0; i < NUM_PLANTS; i++) {
      plantArray.emplace_back(std::make_unique<Plants>(
          matrix, matrix->getXResolution() * i / NUM_PLANTS,
          matrix->getYResolution() + 7));
    }
  }

  void addFood() {
    float x = random(0, matrix->getXResolution());
    foodArray.emplace_back(std::make_unique<Food>(matrix, x));

    float minDistance = std::numeric_limits<float>::max();
    Fish* closestFish = nullptr;

    for (const auto& fish : fishArray) {
      if (fish->getFood() == nullptr && fish->getAge() > AGE_EGG &&
          fish->getAge() < AGE_SENIOR) {
        float distance = fish->getPosition().dist(PVector(x, 0));
        if (distance < minDistance) {
          minDistance = distance;
          closestFish = fish.get();
        }
      }
    }

    if (closestFish) {
      closestFish->setFood(foodArray.back().get());
    }
  }

  // Update all plants in the aquarium
  void updatePlants() {
    float humidity = demoMode ? demoHumidity : (scd40->isFirstReadingReceived() ? scd40->getHumidity() : 50);
    for (auto& plant : plantArray) {
      plant->update(humidity);
    }
  }

  // Update the water environment
  void updateWater() {
    float temperature = demoMode ? demoTemperature : (scd40->isFirstReadingReceived() ? scd40->getTemperature() : 25);
    water.update(temperature);
  }

  // Update all fish in the aquarium
  void updateFish() {
    float co2 = demoMode ? demoCO2 : (scd40->isFirstReadingReceived() ? scd40->getCO2() : 400);
    for (auto it = fishArray.begin(); it != fishArray.end();) {
      bool destroy = (*it)->update(co2);
      if (destroy) {
        it = fishArray.erase(it);
      } else {
        (*it)->display();
        ++it;
      }
    }

    // Population control
    if (fishArray.size() < NUM_FISH_IDEAL) {
      // Attempt to create new fish
      for (auto& fish : fishArray) {
        if (fish->tryReproduce()) {
          PVector newPos = fish->getPosition();
          fishArray.emplace_back(
              std::make_unique<Fish>(matrix, newPos));
          break;  // Only add one fish per update cycle
        }
      }
    }
  }

  void updateFood() {
    for (auto it = foodArray.begin(); it != foodArray.end();) {
      (*it)->update();
      if ((*it)->isOffScreen() || (*it)->isEaten()) {
        it = foodArray.erase(it);
      } else {
        (*it)->display();
        ++it;
      }
    }
  }

  void handleTouchInput() {
    if (touchRead(13) / 1000 > 70) {
      addFood();
    }
  }

  // General update function that updates all components of the aquarium
  void update() {
    handleTouchInput();

    if (demoMode) {
      updateDemo();
      switch (demoStep) {
        case 0:
          updateWater();
          break;
        case 1:
          updateWater();
          updatePlants();
          break;
        case 2:
          updateWater();
          updateFish();
          updateWater();
          break;
      }
    } else {
      updateWater();
      boidManager.updateBoids();  // Update Boids
      boidManager.renderBoids();  // Render Boids
      updateFish();
      updateFood();
      updatePlants();
    }
  }

  void display(bool showSensorData) {
    // matrix->gfx_compositor->StackWithThreshold(*matrix->background, *matrix->foreground, 12);
    if (showSensorData) {
      if(scd40->isFirstReadingReceived()) {
        char buffer[100];
        float temperature = demoMode ? demoTemperature : scd40->getTemperature();
        float humidity = demoMode ? demoHumidity : scd40->getHumidity();
        float co2 = demoMode ? demoCO2 : scd40->getCO2();
        
        snprintf(buffer, sizeof(buffer), "%s\nTemp: %.1f C\nHumidity: %.0f %%\nCO2: %.0f ppm", 
                demoMode ? "DEMO" : "",
                temperature, humidity, co2);
        drawMultilineText(matrix->background, buffer, TOP, &Font4x7Fixed, CRGB(150,150,150));
      } else {
        drawMultilineText(matrix->background, "Sensor Warming Up...", TOP, &Font4x7Fixed, CRGB(150,150,150));
      }
    }

    matrix->gfx_compositor->Stack(*matrix->background, *matrix->foreground);
    matrix->foreground->reduceBrightness(20);
    // matrix->gfx_compositor->Stack(*matrix->background, *matrix->foreground);
    matrix->foreground->clear();

  }

  // Destructor to clean up resources
  ~Aquarium() {
    // Unique pointers automatically clean up
  }

  void drawMultilineText(GFX_Layer* layer, const char* text, textPosition textPos, const GFXfont* f, CRGB color) {
  layer->setFont(f);
  layer->setTextColor(layer->color565(color.r, color.g, color.b));

  // Split the text into lines
  std::vector<String> lines;
  String currentLine;
  for (const char* p = text; *p; p++) {
    if (*p == '\n') {
      lines.push_back(currentLine);
      currentLine = "";
    } else {
      currentLine += *p;
    }
  }
  if (currentLine.length() > 0) {
    lines.push_back(currentLine);
  }

  // Calculate total height and maximum width
  int16_t x1, y1;
  uint16_t w, h, maxWidth = 0, totalHeight = 0;
  for (const String& line : lines) {
    layer->getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);
    maxWidth = max(maxWidth, w);
    totalHeight += h;
  }

  // Calculate starting position
  int16_t startX = (layer->getWidth() - maxWidth) / 2;
  int16_t startY;
  if (textPos == TOP) {
    startY = h;
  } else if (textPos == BOTTOM) {
    startY = layer->getHeight() - totalHeight;
  } else { // MIDDLE
    startY = (layer->getHeight() - totalHeight) / 2 + h;
  }

  // Draw each line
  for (const String& line : lines) {
    layer->setCursor(startX, startY);
    layer->print(line);
    startY += h;
  }
}
};

#endif  // AQUARIUM_H