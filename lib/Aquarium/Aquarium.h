#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <Arduino.h>
#include <Fonts/Font4x7Fixed.h>
#include <Fonts/Font5x7Fixed.h>
#include <Matrix.h>
#include <scd40.h>

#include <vector>

#include "AquariumSettings.h"
#include "AquariumStateManager.h"
#include "BoidManager.h"
#include "Fish.h"
#include "Food.h"
#include "Plants.h"
#include "PlanktonField.h"
#include "Water.h"
#include "StateManager.h"
#include "../TOFSensor/TOFSensor.h"
#include "../TOFSensor/TOFInteractionManager.h"

class Aquarium {
 private:
  Matrix* matrix;
  SCD40* scd40;
  StateManager* stateManager;
  Water water;
  std::vector<std::unique_ptr<Fish>> fishArray;
  std::vector<std::unique_ptr<Plants>> plantArray;
  std::vector<std::unique_ptr<Food>> foodArray;
  BoidManager boidManager;
  PlanktonField planktonField;
  AquariumStateManager aquariumStateManager;
  unsigned long lastSaveTime;
  char buffer[100];

  // TOF sensor interaction
  TOFInteractionManager* interactionManager = nullptr;
  TOFSensor* tofSensor = nullptr;  // Store sensor pointer for lazy init

  // Demo settings
  bool demoMode;
  int demoStep;
  unsigned long demoStartTime;
  float demoTemperature;
  float demoHumidity;
  float demoCO2;
  bool demoFinished;

  enum class TextAlignment { LEFT, CENTER, RIGHT };

 public:
  Aquarium(Matrix* m, SCD40* s, StateManager* stateManager)
      : matrix(m),
        scd40(s),
        stateManager(stateManager),
        water(matrix),
        boidManager(m),
        planktonField(m),
        demoMode(false),
        demoStep(0),
        demoFinished(false) {}

  void begin() {
    loadState();
    initializePlants();
    boidManager.initializeBoids();
    planktonField.init();
  }

  bool isDemoFinished() const {
    return demoFinished;
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


  // Define step durations in milliseconds
  const unsigned long stepDurations[] = {
    5000,  // 0 - Welcome to Livegrid
    5000,  // 1 - Your very own aquatic ecosystem
    5000,  // 2 - You start with 5 fish
    5000,  // 3 - They will slowly grow, and reproduce
    5000,  // 4 - But they are affected by the environment
    5000,  // 5 - Temperature affects water color
    15000, // 6 - show
    5000,  // 7 - Humidity affects plant growth
    15000, // 8 - show
    5000,  // 9 - CO2 affects fish behavior
    30000, // 10 - show
    5000,  // 11 - If your environment is good, they will thrive
    5000,  // 12 - And soon you will have an amazing ecosystem
    5000,  // 13 - Take care of them by taking care of yourself
    5000   // 14 - Enjoy
  };

  // Calculate current step and time within step
  unsigned long totalDuration = 0;
  for (demoStep = 0; demoStep < sizeof(stepDurations) / sizeof(stepDurations[0]); demoStep++) {
    if (elapsedTime < totalDuration + stepDurations[demoStep]) {
      break;
    }
    totalDuration += stepDurations[demoStep];
  }

  unsigned long stepElapsedTime = elapsedTime - totalDuration;

  auto pausingSine = [](float t, float pauseDuration) {
    float sineValue = sin(t);
    if (abs(sineValue) < 0.1) {  // Pause near the midpoint
      return 0.0f;
    }
    return sineValue;
  };

  float t;

  // Update demo values and display text based on the current step
  switch (demoStep) {
    case 0:
      snprintf(buffer, sizeof(buffer), "Welcome to\nLivegrid");
      break;
    case 1:
      snprintf(buffer, sizeof(buffer), "Your very own\naquatic\necosystem");
      break;
    case 2:
      snprintf(buffer, sizeof(buffer), "You start\nwith 5 fish");
      break;
    case 3:
      snprintf(buffer, sizeof(buffer), "They will\nslowly grow,\nand reproduce");
      break;
    case 4:
      snprintf(buffer, sizeof(buffer), "But they are\naffected by\nthe\nenvironment");
      break;
    case 5:
      snprintf(buffer, sizeof(buffer), "Temperature\naffects\nwater color");
      updateWater();
      break;
    case 6:
      t = 2 * PI * stepElapsedTime / stepDurations[demoStep];
      demoTemperature = 25.0f + 25.0f * pausingSine(t, 0.2);
      snprintf(buffer, sizeof(buffer), "Temperature:\n%.0f C", demoTemperature);
      updateWater();
      break;
    case 7:
      snprintf(buffer, sizeof(buffer), "Humidity\naffects\nplant growth");
      updateWater();
      updatePlants();
      break;
    case 8:
      t = 2 * PI * stepElapsedTime / stepDurations[demoStep];
      demoHumidity = 50.0f + 40.0f * pausingSine(t, 0.2);
      snprintf(buffer, sizeof(buffer), "Humidity:\n%.0f %%", demoHumidity);
      updateWater();
      updatePlants();
      break;
     case 9:
      snprintf(buffer, sizeof(buffer), "CO2 affects\nfish behavior");
      break;
    case 10:
      t = 2 * PI * stepElapsedTime / stepDurations[demoStep];
      demoCO2 = 400.0f + 1600.0f * (0.5f + 0.5f * pausingSine(t, 0.2));
      snprintf(buffer, sizeof(buffer), "CO2:\n%.0f ppm", demoCO2);
      break;
    case 11:
      demoCO2 = 400;
      snprintf(buffer, sizeof(buffer), "If your\nenvironment\nis good,\nthey will\nthrive");
      break;
    case 12:
      snprintf(buffer, sizeof(buffer), "And soon you\nwill have an\namazing\necosystem");
      break;
    case 13:
      snprintf(buffer, sizeof(buffer), "Take care of\nthem by taking\ncare of\nyourself");
      break;
    case 14:
      snprintf(buffer, sizeof(buffer), "Enjoy !");
      break;
    default:
      demoMode = false;
      demoFinished = true;  // Set demoFinished to true when demo is complete
      saveState();
      return;
  }

  if (demoStep < 5 || demoStep > 8) {
    updateWater();
    boidManager.updateBoids(demoCO2);
    boidManager.renderBoids();
    updateFish();
    updateFood();
    updatePlants();
  }

  drawMultilineText(matrix->foreground, buffer, MIDDLE,
                    TextAlignment::CENTER, &Font5x7Fixed,
                    CRGB(150, 150, 150));
}

  void loadState() {
    if (!aquariumStateManager.loadState(fishArray, matrix)) {
      log_w("Failed to load aquarium state, initializing with default values");
      initializeFish();
    }
  }

  void saveState() {
    aquariumStateManager.saveState(fishArray);
    log_i("Aquarium state saved");
  }

  void periodicSave() {
    unsigned long currentTime = millis();
    if (currentTime - lastSaveTime >= AQUARIUM_SAVE_INTERVAL*60000) {
      aquariumStateManager.saveState(fishArray);
      lastSaveTime = currentTime;
    }
  }

  // Initialize fish and store them in a vector of unique pointers
  void initializeFish() {
    PVector centerPos(matrix->getXResolution() / 2,
                      matrix->getYResolution() / 2);
    for (int i = 0; i < NUM_FISH_START; i++) {
      fishArray.emplace_back(
          std::make_unique<Fish>(matrix,
                                 PVector(random(0, matrix->getXResolution()),
                                         random(0, matrix->getYResolution())),
                                 0.5f));
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
    float humidity =
        demoMode
            ? demoHumidity
            : (scd40 && scd40->isFirstReadingReceived() ? scd40->getHumidity() : 50);
    for (auto& plant : plantArray) {
      plant->update(humidity);
    }
  }

  // Update the water environment
  void updateWater() {
    float temperature =
        demoMode
            ? demoTemperature
            : (scd40 && scd40->isFirstReadingReceived() ? scd40->getTemperature() : 25);
    
    water.update(temperature);
  }
  
  // Helper: Bilinear interpolation to get smooth depth value at any position
  float interpolateDepth(const InteractionData& interaction, float normX, float normY) {
    // Map normalized 0-1 coordinates to sensor grid (0-7)
    float sensorX = normX * 7.0f;
    float sensorY = normY * 7.0f;
    
    // Get grid cell indices
    int x0 = (int)sensorX;
    int y0 = (int)sensorY;
    int x1 = min(x0 + 1, 7);
    int y1 = min(y0 + 1, 7);
    
    // Clamp
    x0 = constrain(x0, 0, 7);
    y0 = constrain(y0, 0, 7);
    
    // Interpolation weights
    float fx = sensorX - x0;
    float fy = sensorY - y0;
    
    // Get depth values at corners
    float d00 = (float)interaction.depthMap[y0][x0];
    float d10 = (float)interaction.depthMap[y0][x1];
    float d01 = (float)interaction.depthMap[y1][x0];
    float d11 = (float)interaction.depthMap[y1][x1];
    
    // Handle invalid readings (0 or negative) by using neighbors
    if (d00 <= 0) d00 = TOF_MAX_DETECTION_DIST + 1;
    if (d10 <= 0) d10 = TOF_MAX_DETECTION_DIST + 1;
    if (d01 <= 0) d01 = TOF_MAX_DETECTION_DIST + 1;
    if (d11 <= 0) d11 = TOF_MAX_DETECTION_DIST + 1;
    
    // Bilinear interpolation
    float d0 = d00 * (1.0f - fx) + d10 * fx;
    float d1 = d01 * (1.0f - fx) + d11 * fx;
    return d0 * (1.0f - fy) + d1 * fy;
  }
  
  // Draw TOF silhouette with smooth bilinear interpolation
  // Fish and other elements are drawn after, so they appear in front
  void drawTOFSilhouette() {
    if (!interactionManager) return;
    
    InteractionData interaction = interactionManager->getInteractionData();
    
    // Debug logging
    static unsigned long lastLogTime = 0;
    if (millis() - lastLogTime > 2000) {
      log_i("Aquarium TOF: hasBlob=%d, blobX=%.2f, blobY=%.2f, vel=%.2f, depth[4][4]=%d", 
            interaction.hasBlob, interaction.blobX, interaction.blobY, 
            interaction.velocityMag, interaction.depthMap[4][4]);
      lastLogTime = millis();
    }
    
    uint16_t matrixWidth = matrix->getXResolution();
    uint16_t matrixHeight = matrix->getYResolution();
    
    // Draw pixel-by-pixel with bilinear interpolation for smooth silhouette
    for (uint16_t py = 0; py < matrixHeight; py++) {
      for (uint16_t px = 0; px < matrixWidth; px++) {
        // Map pixel to normalized 0-1 coordinates
        float normX = (float)px / (float)(matrixWidth - 1);
        float normY = (float)py / (float)(matrixHeight - 1);
        
        // Get interpolated depth at this position
        float depth = interpolateDepth(interaction, normX, normY);
        
        // Skip if out of detection range
        if (depth <= 0 || depth > TOF_MAX_DETECTION_DIST) {
          continue;
        }
        
        // Clamp minimum
        if (depth < TOF_MIN_DETECTION_DIST) {
          depth = TOF_MIN_DETECTION_DIST;
        }
        
        // Normalize depth (closer = 0, further = 1)
        float normalizedDepth = (depth - TOF_MIN_DETECTION_DIST) / 
                               (TOF_MAX_DETECTION_DIST - TOF_MIN_DETECTION_DIST);
        normalizedDepth = constrain(normalizedDepth, 0.0f, 1.0f);
        
        // Only show if within depth threshold
        if (normalizedDepth > SILHOUETTE_DEPTH_THRESHOLD) continue;
        
        // Calculate intensity: closer = more visible, with smooth falloff
        float intensity = (1.0f - normalizedDepth / SILHOUETTE_DEPTH_THRESHOLD);
        intensity = intensity * intensity;  // Quadratic falloff for softer edges
        intensity *= SILHOUETTE_OPACITY;
        
        // Create shadow color
        CRGB shadowColor = CRGB(
          (uint8_t)(SILHOUETTE_COLOR_R * intensity),
          (uint8_t)(SILHOUETTE_COLOR_G * intensity),
          (uint8_t)(SILHOUETTE_COLOR_B * intensity)
        );
        
        matrix->foreground->drawPixel(px, py, shadowColor);
      }
    }
  }

  // Update all fish in the aquarium
  void updateFish() {
    float co2;
    
    if (demoMode) {
      co2 = demoCO2;
    } else {
      co2 = (scd40 && scd40->isFirstReadingReceived()) ? scd40->getCO2() : 400;
    }
    
    // Get interaction data if available
    InteractionData* interaction = nullptr;
    InteractionData interactionData;
    if (interactionManager) {
      interactionData = interactionManager->getInteractionData();
      // Depth-map fallback: when blob detection fails, use centroid of valid depth cells
      if (!interactionData.hasBlob) {
        float sumX = 0, sumY = 0;
        int count = 0;
        for (uint8_t y = 0; y < 8; y++) {
          for (uint8_t x = 0; x < 8; x++) {
            int16_t d = interactionData.depthMap[y][x];
            if (d >= TOF_MIN_DETECTION_DIST && d <= TOF_MAX_DETECTION_DIST) {
              sumX += x + 0.5f;
              sumY += y + 0.5f;
              count++;
            }
          }
        }
        if (count > 0) {
          interactionData.hasBlob = true;
          interactionData.blobX = (sumX / count) / 8.0f;
          interactionData.blobY = (sumY / count) / 8.0f;
          interactionData.velocityMag = 0.0f;  // Unknown velocity -> attract
        }
      }
      interaction = &interactionData;
    }
    for (auto it = fishArray.begin(); it != fishArray.end();) {
      bool destroy = (*it)->update(co2, demoMode, interaction);
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
          fishArray.emplace_back(std::make_unique<Fish>(matrix, newPos));
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
    if (touchRead(13) / 1000 > FOOD_TOUCH_THRESHOLD) {
      // addFood();
    }
  }

  void updateSensorData(bool showSensorData) {
    if (showSensorData && !demoMode) {
      if (scd40 && scd40->isFirstReadingReceived()) {
        float temperature = scd40->getTemperature();
        float humidity = scd40->getHumidity();
        float co2 = scd40->getCO2();
        
        if (stateManager->getState()->temperatureUnit == TemperatureUnit::FAHRENHEIT) {
            // Convert to Fahrenheit
              temperature = stateManager->getState()->environment.temperature_fahrenheit.value;
              snprintf(buffer, sizeof(buffer),
                  "%s\nTemp: %.1f F\nHumidity: %.0f %%\nCO2: %.0f ppm", "",
                  temperature, humidity, co2);
          } else {
              snprintf(buffer, sizeof(buffer),
                  "%s\nTemp: %.1f C\nHumidity: %.0f %%\nCO2: %.0f ppm", "",
                  temperature, humidity, co2);
          }
        
        drawMultilineText(matrix->foreground, buffer, MIDDLE,
                          TextAlignment::CENTER, &Font4x7Fixed,
                          CRGB(150, 150, 150));
      } else {
        drawMultilineText(matrix->foreground, "Sensors\nWarming Up...", MIDDLE,
                          TextAlignment::CENTER, &Font4x7Fixed,
                          CRGB(150, 150, 150));
      }
    }
  }

  // General update function that updates all components of the aquarium
  void update(bool showSensorData = false) {
    handleTouchInput();

    // Lazily create interaction manager when sensor becomes active
    ensureInteractionManager();
    
    // Update interaction manager if available
    if (interactionManager) {
      interactionManager->update();
    }

    if (demoMode) {
      updateDemo();
    } else {
      updateWater();

      InteractionData interactionData;
      if (interactionManager) {
        interactionData = interactionManager->getInteractionData();
      } else {
        memset(&interactionData, 0, sizeof(interactionData));
        interactionData.hasBlob = false;
      }
      planktonField.update(interactionData);
      planktonField.draw();

      boidManager.updateBoids((scd40 && scd40->isFirstReadingReceived()) ? scd40->getCO2() : 400);
      boidManager.renderBoids();

      updateFish();
      // updateFood();

      updatePlants();
      updateSensorData(showSensorData);
      periodicSave();
    }
  }

  void display() {
    matrix->gfx_compositor->Stack(*matrix->background, *matrix->foreground);
    matrix->foreground->clear();
  }

  void setTofSensor(TOFSensor* sensor) {
    tofSensor = sensor;
    // Don't create interaction manager yet - will be created lazily in update()
    // This handles the case where sensor isn't active yet when this is called
  }
  
  // Lazily initialize interaction manager when sensor becomes active
  void ensureInteractionManager() {
    if (interactionManager) return;  // Already initialized
    
    if (tofSensor && tofSensor->isActive()) {
      log_i("Aquarium: Creating TOFInteractionManager - sensor is active");
      interactionManager = new TOFInteractionManager(tofSensor);
      interactionManager->calibrateBaseline();  // Calibrate background for subtraction
    }
  }

  // Destructor to clean up resources
  ~Aquarium() {
    // Unique pointers automatically clean up
    if (interactionManager) {
      delete interactionManager;
      interactionManager = nullptr;
    }
  }

  void drawMultilineText(GFX_Layer* layer, const char* text,
                         textPosition textPos, TextAlignment alignment,
                         const GFXfont* f, CRGB color) {
    layer->setFont(f);
    layer->setTextColor(layer->color565(color.r, color.g, color.b));

    // Split text into lines
    std::vector<String> lines;
    String currentLine;
    for (const char* c = text; *c; ++c) {
      if (*c == '\n') {
        lines.push_back(currentLine);
        currentLine = "";
      } else {
        currentLine += *c;
      }
    }
    if (!currentLine.isEmpty()) {
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

    // Calculate starting Y position
    int16_t startY;
    if (textPos == TOP) {
      startY = h;
    } else if (textPos == BOTTOM) {
      startY = layer->getHeight() - totalHeight;
    } else {  // MIDDLE
      startY = (layer->getHeight() - totalHeight) / 2 + h;
    }

    // Draw each line
    for (const String& line : lines) {
      int16_t lineX;
      layer->getTextBounds(line.c_str(), 0, 0, &x1, &y1, &w, &h);

      switch (alignment) {
        case TextAlignment::LEFT:
          lineX = 0;
          break;
        case TextAlignment::CENTER:
          lineX = (layer->getWidth() - w) / 2;
          break;
        case TextAlignment::RIGHT:
          lineX = layer->getWidth() - w;
          break;
      }

      layer->setCursor(lineX, startY);
      layer->print(line);
      startY += h;
    }
  }
};

#endif  // AQUARIUM_H