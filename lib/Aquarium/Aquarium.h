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
#include "SeaFloor.h"
#include "Water.h"
#include "StateManager.h"
// #include "PlanktonField.h"  // TEST: plankton disabled
#include "Motion/MotionProfile.h"
#include "../TOFSensor/TOFSensor.h"
#include "../TOFSensor/TOFInteractionManager.h"

class Aquarium {
 private:
  Matrix* matrix;
  SCD40* scd40;
  StateManager* stateManager;
  Water water;
  std::vector<std::unique_ptr<Fish>> fishArray;
  std::vector<std::unique_ptr<Food>> foodArray;
  BoidManager boidManager;
  SeaFloor seaFloor;
  AquariumStateManager aquariumStateManager;
  unsigned long lastSaveTime;
  char buffer[100];
  // PlanktonField planktonField;  // TEST: plankton disabled
  // TOF sensor interaction
  TOFInteractionManager* interactionManager = nullptr;
  TOFSensor* tofSensor = nullptr;  // Store sensor pointer for lazy init
  bool aquariumPalmValid = false;
  float aquariumPalmX = 0.5f;
  float aquariumPalmY = 0.5f;
  float aquariumPalmVelocityX = 0;
  float aquariumPalmVelocityY = 0;
  unsigned long aquariumPalmLastSeenMs = 0;
  unsigned long aquariumPalmLastUpdateMs = 0;

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
        seaFloor(m),
        demoMode(false),
        demoStep(0),
        demoFinished(false) {}
        // planktonField(m) — TEST: plankton disabled

  void begin() {
    loadState();
    seaFloor.generate();
    // planktonField.init();  // TEST: plankton disabled
    boidManager.initializeBoids();
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
    boidManager.updateBoids(demoCO2, nullptr);
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

  // Update and draw the seafloor
  void updatePlants() {
    float humidity =
        demoMode
            ? demoHumidity
            : (scd40 && scd40->isFirstReadingReceived() ? scd40->getHumidity() : 50);
    seaFloor.update(humidity);
    seaFloor.draw();
  }

  // Update the water environment
  void updateWater() {
    float temperature =
        demoMode
            ? demoTemperature
            : (scd40 && scd40->isFirstReadingReceived() ? scd40->getTemperature() : 25);
    
    water.update(temperature);
  }

  void augmentAquariumInteraction(InteractionData& interaction) {
#if !AQUARIUM_TOF_PALM_INTERACTION_ENABLED
    aquariumPalmValid = false;
    aquariumPalmVelocityX = 0;
    aquariumPalmVelocityY = 0;
    interaction.hasPalmHold = false;
    interaction.palmNormX = 0.5f;
    interaction.palmNormY = 0.5f;
    interaction.palmVelocityX = 0;
    interaction.palmVelocityY = 0;
    interaction.palmVelocityMag = 0;
    interaction.palmStrength = 0;
    return;
#else
    unsigned long now = millis();
    float dt = aquariumPalmLastUpdateMs == 0 ? 0.033f : (now - aquariumPalmLastUpdateMs) / 1000.0f;
    if (dt <= 0.0f || dt > 1.0f) dt = 0.033f;
    aquariumPalmLastUpdateMs = now;

    if (interaction.hasPalm) {
      float rawX = ((float)interaction.palmX + 0.5f) / 8.0f;
      float rawY = ((float)interaction.palmY + 0.5f) / 8.0f;
      rawX = constrain(rawX, 0.0f, 1.0f);
      rawY = constrain(rawY, 0.0f, 1.0f);

      if (!aquariumPalmValid) {
        aquariumPalmX = rawX;
        aquariumPalmY = rawY;
        aquariumPalmVelocityX = 0;
        aquariumPalmVelocityY = 0;
        aquariumPalmValid = true;
      } else {
        float prevX = aquariumPalmX;
        float prevY = aquariumPalmY;
        aquariumPalmX += (rawX - aquariumPalmX) * PALM_POSITION_SMOOTH;
        aquariumPalmY += (rawY - aquariumPalmY) * PALM_POSITION_SMOOTH;

        float instVx = (aquariumPalmX - prevX) / dt;
        float instVy = (aquariumPalmY - prevY) / dt;
        aquariumPalmVelocityX += (instVx - aquariumPalmVelocityX) * PALM_VELOCITY_SMOOTH;
        aquariumPalmVelocityY += (instVy - aquariumPalmVelocityY) * PALM_VELOCITY_SMOOTH;
      }

      aquariumPalmLastSeenMs = now;
    } else if (aquariumPalmValid) {
      unsigned long missingMs = now - aquariumPalmLastSeenMs;
      if (missingMs <= PALM_HOLD_MS) {
        aquariumPalmVelocityX *= PALM_VELOCITY_DECAY;
        aquariumPalmVelocityY *= PALM_VELOCITY_DECAY;
      } else {
        aquariumPalmValid = false;
        aquariumPalmVelocityX = 0;
        aquariumPalmVelocityY = 0;
      }
    }

    if (aquariumPalmValid) {
      unsigned long missingMs = interaction.hasPalm ? 0 : now - aquariumPalmLastSeenMs;
      float holdStrength = 1.0f;
      if (missingMs > 0) {
        holdStrength = 1.0f - ((float)missingMs / (float)PALM_HOLD_MS);
        holdStrength = constrain(holdStrength, 0.0f, 1.0f);
      }

      interaction.hasPalmHold = holdStrength > 0.0f;
      interaction.palmNormX = aquariumPalmX;
      interaction.palmNormY = aquariumPalmY;
      interaction.palmVelocityX = aquariumPalmVelocityX;
      interaction.palmVelocityY = aquariumPalmVelocityY;
      interaction.palmVelocityMag = sqrtf(aquariumPalmVelocityX * aquariumPalmVelocityX +
                                          aquariumPalmVelocityY * aquariumPalmVelocityY);
      interaction.palmStrength = holdStrength;
    } else {
      interaction.hasPalmHold = false;
      interaction.palmNormX = 0.5f;
      interaction.palmNormY = 0.5f;
      interaction.palmVelocityX = 0;
      interaction.palmVelocityY = 0;
      interaction.palmVelocityMag = 0;
      interaction.palmStrength = 0;
    }
#endif  // AQUARIUM_TOF_PALM_INTERACTION_ENABLED
  }
  
  // Draw a live TOF shadow on the foreground before fish/plants, while water
  // stays chunked on the background layer.
  void drawTOFShadowOverlay(const InteractionData& interaction) {
    if (!interaction.hasBlob) return;

    uint16_t matrixWidth = matrix->getXResolution();
    uint16_t matrixHeight = matrix->getYResolution();

    for (uint8_t gy = 0; gy < 8; gy++) {
      uint16_t y0 = (uint16_t)((uint32_t)gy * matrixHeight / 8u);
      uint16_t y1 = (uint16_t)((uint32_t)(gy + 1u) * matrixHeight / 8u);
      uint16_t h = y1 > y0 ? y1 - y0 : 1;

      for (uint8_t gx = 0; gx < 8; gx++) {
        int16_t depth = interaction.depthMap[gy][gx];
        if (depth <= TOF_MIN_DETECTION_DIST || depth >= TOF_MAX_DETECTION_DIST) continue;

        float normalizedDepth = (depth - TOF_MIN_DETECTION_DIST) /
                                (float)(TOF_MAX_DETECTION_DIST - TOF_MIN_DETECTION_DIST);
        normalizedDepth = constrain(normalizedDepth, 0.0f, 1.0f);
        if (normalizedDepth > SILHOUETTE_DEPTH_THRESHOLD) continue;

        float intensity = 1.0f - normalizedDepth / SILHOUETTE_DEPTH_THRESHOLD;
        intensity = intensity * intensity * SILHOUETTE_OPACITY;
        if (intensity < 0.10f) continue;

        uint16_t x0 = (uint16_t)((uint32_t)gx * matrixWidth / 8u);
        uint16_t x1 = (uint16_t)((uint32_t)(gx + 1u) * matrixWidth / 8u);
        uint16_t w = x1 > x0 ? x1 - x0 : 1;

        CRGB shadowColor(
            (uint8_t)(SILHOUETTE_COLOR_R * intensity),
            (uint8_t)(SILHOUETTE_COLOR_G * intensity),
            (uint8_t)(SILHOUETTE_COLOR_B * intensity));
        matrix->foreground->fillRect((int16_t)x0, (int16_t)y0, w, h, shadowColor);
      }
    }
  }

  // Update all fish in the aquarium
  void updateFish(InteractionData* interactionOverride = nullptr) {
#if MOTION_PROFILE_DEBUG_ENABLED
    MotionProfileDebug::tick(matrix->getXResolution() * PHYSICS_SCALE,
                             matrix->getYResolution() * PHYSICS_SCALE);
#endif
    float co2;
    
    if (demoMode) {
      co2 = demoCO2;
    } else {
      co2 = (scd40 && scd40->isFirstReadingReceived()) ? scd40->getCO2() : 400;
    }
    
    InteractionData* interaction = nullptr;
    InteractionData interactionData;
    if (interactionOverride) {
      interaction = interactionOverride;
    } else if (interactionManager) {
      interactionData = interactionManager->getInteractionData();
      augmentAquariumInteraction(interactionData);
      interaction = &interactionData;
    }

    // Snapshot school positions for lightweight separation logic inside Fish::update.
    std::vector<PVector> schoolPositions;
    schoolPositions.reserve(fishArray.size());
    for (const auto& fish : fishArray) {
      schoolPositions.push_back(fish->getPosition());
    }

    int fishIndex = 0;
    for (auto it = fishArray.begin(); it != fishArray.end();) {
      bool destroy = (*it)->update(co2, demoMode, interaction, &schoolPositions, fishIndex);
      if (destroy) {
        it = fishArray.erase(it);
      } else {
        (*it)->display();
        ++it;
      }
      fishIndex++;
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
      InteractionData interactionData;
      if (interactionManager) {
        interactionData = interactionManager->getInteractionData();
      } else {
        memset(&interactionData, 0, sizeof(interactionData));
        interactionData.hasBlob = false;
      }
      augmentAquariumInteraction(interactionData);
      // planktonField.update(interactionData);  // TEST: plankton disabled

      updateWater();
      // planktonField.draw();  // TEST: plankton disabled

      boidManager.updateBoids((scd40 && scd40->isFirstReadingReceived()) ? scd40->getCO2() : 400,
                              &interactionData);
      boidManager.renderBoids();

      updateFish(&interactionData);
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

    // Walk the string in place, using small stack buffers - no String /
    // std::vector churn. Supports up to 8 lines of 63 chars each; ample
    // for our messages ("Sensors\nWarming Up...", temperature notices, etc.)
    constexpr size_t kMaxLines = 8;
    constexpr size_t kLineBufSize = 64;
    char lines[kMaxLines][kLineBufSize];
    uint16_t lineWidths[kMaxLines] = {0};
    size_t lineCount = 0;

    int16_t x1, y1;
    uint16_t w = 0, h = 0;
    uint16_t lineHeight = 0;
    uint16_t totalHeight = 0;

    const char* p = text;
    while (*p && lineCount < kMaxLines) {
      size_t len = 0;
      while (p[len] && p[len] != '\n' && len + 1 < kLineBufSize) ++len;
      memcpy(lines[lineCount], p, len);
      lines[lineCount][len] = '\0';

      layer->getTextBounds(lines[lineCount], 0, 0, &x1, &y1, &w, &h);
      lineWidths[lineCount] = w;
      if (h > lineHeight) lineHeight = h;
      totalHeight += h;

      ++lineCount;
      p += len;
      if (*p == '\n') ++p;
    }

    int16_t startY;
    if (textPos == TOP) {
      startY = lineHeight;
    } else if (textPos == BOTTOM) {
      startY = layer->getHeight() - totalHeight + lineHeight;
    } else {  // MIDDLE
      startY = (layer->getHeight() - totalHeight) / 2 + lineHeight;
    }

    for (size_t i = 0; i < lineCount; ++i) {
      int16_t lineX = 0;
      switch (alignment) {
        case TextAlignment::LEFT:   lineX = 0; break;
        case TextAlignment::CENTER: lineX = (layer->getWidth() - lineWidths[i]) / 2; break;
        case TextAlignment::RIGHT:  lineX = layer->getWidth() - lineWidths[i]; break;
      }
      layer->setCursor(lineX, startY);
      layer->print(lines[i]);
      startY += lineHeight;
    }
  }
};

#endif  // AQUARIUM_H