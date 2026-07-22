#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <Arduino.h>
#include <Fonts/Font4x7Fixed.h>
#include <Fonts/Font5x7Fixed.h>
#include <Matrix.h>
#include <scd40.h>
#include <SCD40Settings.h>

#include <vector>

#include "AquariumSettings.h"
#include "AquariumStateManager.h"
#include "AquariumLayout.h"
#include "BoidManager.h"
#include "Fish.h"
#include "Food.h"
#include "SeaFloor.h"
#include "Water.h"
#include "StateManager.h"
#include "PlanktonField.h"
#include "Motion/MotionProfile.h"
#include "../TOFSensor/TOFSensor.h"
#include "../TOFSensor/TOFInteractionManager.h"
#ifdef ADXL345_ENABLED
#include "../AutoRotate/AutoRotate.h"
#endif

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
  PlanktonField planktonField;
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
  uint8_t lastKnownRotation = 255;
#ifdef ADXL345_ENABLED
  AutoRotate* autoRotateSource = nullptr;
#endif

  // Demo settings
  bool demoMode;
  int demoStep;
  unsigned long demoStartTime;
  float demoTemperature;
  float demoHumidity;
  float demoCO2;
  bool demoFinished;
  /** Remote/video override: ramp CO2 toward CO2_REALBAD (or back to sensor) over 10s. */
  bool fakeHighCo2 = false;
  bool fakeCo2Ramping = false;
  float fakeCo2Applied = 400.0f;
  float fakeCo2RampFrom = 400.0f;
  float fakeCo2RampTo = 400.0f;
  unsigned long fakeCo2RampStartMs = 0;
  static constexpr unsigned long kFakeCo2RampMs = 10000UL;

  enum class TextAlignment { LEFT, CENTER, RIGHT };

  // Curated "performance" modes layered on top of the natural aquarium.
  enum class PerformanceMode : uint8_t { Natural, Ring, Orchestra, Fountain, Jump, Count };
  PerformanceMode performanceMode = PerformanceMode::Natural;
  float ringPhase = 0.0f;
  PVector ringAttractorTarget;  // smoothed goal (palm or screen center)
  PVector ringAttractor;        // formation center — fish/boids orbit here
  bool ringAttractorInit = false;
  float orchestraPhase = 0.0f;  // global micro-orbit phase for Orchestra mode
  bool fountainAnchorsPending = true;  // assign spawn anchors on next hands-up (no teleport)
  JumpGridActivityState jumpGridActivity{};
  float jumpPhase = 0.0f;
  unsigned long performanceModeLabelUntilMs = 0;
  static constexpr unsigned long kPerformanceModeLabelMs = 2500;

  static const char* performanceModeName(PerformanceMode mode) {
    switch (mode) {
      case PerformanceMode::Natural: return "Natural";
      case PerformanceMode::Ring: return "Ring";
      case PerformanceMode::Orchestra: return "Orchestra";
      case PerformanceMode::Fountain: return "Fountain";
      case PerformanceMode::Jump: return "Jump";
      default: return "?";
    }
  }

 public:
  const char* getPerformanceModeName() const { return performanceModeName(performanceMode); }

  void setPerformanceMode(PerformanceMode mode) {
    if (mode == performanceMode) return;
    performanceMode = mode;
    performanceModeLabelUntilMs = millis() + kPerformanceModeLabelMs;
    // Active formation modes re-assign targets every frame; clear so Natural starts clean.
    for (auto& fish : fishArray) {
      fish->clearFormationTarget();
      fish->setFountainMode(false);
    }
    if (mode == PerformanceMode::Fountain) fountainAnchorsPending = true;
    if (mode == PerformanceMode::Jump) {
      jumpGridActivity = {};
      jumpPhase = 0.0f;
    }
    log_i("Aquarium performance mode -> %s", performanceModeName(mode));
  }

  void cyclePerformanceMode(int direction) {
    int count = (int)PerformanceMode::Count;
    int next = (((int)performanceMode + direction) % count + count) % count;
    setPerformanceMode((PerformanceMode)next);
  }

  PerformanceMode getPerformanceMode() const { return performanceMode; }

  Aquarium(Matrix* m, SCD40* s, StateManager* stateManager)
      : matrix(m),
        scd40(s),
        stateManager(stateManager),
        water(matrix),
        boidManager(m),
        seaFloor(m),
        demoMode(false),
        demoStep(0),
        demoFinished(false),
        planktonField(m) {}

  void begin() {
    loadState();
    syncOrientation(true);
    seaFloor.generate();
    planktonField.init();
    boidManager.initializeBoids();
  }

#ifdef ADXL345_ENABLED
  void setAutoRotate(AutoRotate* source) { autoRotateSource = source; }
#endif

  void syncOrientation(bool force = false) {
    uint8_t rot = lastKnownRotation < 4 ? lastKnownRotation : 0;
#ifdef ADXL345_ENABLED
    if (autoRotateSource && autoRotateSource->isSensorWorking()) {
      rot = autoRotateSource->getStableRotation();
    }
#endif

    if (matrix->getRotation() != 0) {
      matrix->setRotation(0);
    }

    if (!force && rot == lastKnownRotation) return;
    lastKnownRotation = rot;
    seaFloor.setOrientation(rot);
    if (interactionManager) {
      interactionManager->setHandRaiseOrientation(rot);
    }
  }

  bool isDemoFinished() const {
    return demoFinished;
  }

  bool isDemoMode() const {
    return demoMode;
  }

  void startDemo() {
    demoMode = true;
    demoFinished = false;
    demoStep = 0;
    demoStartTime = millis();
    demoTemperature = 25.0f;
    demoHumidity = 50.0f;
    demoCO2 = 400.0f;
  }

  void stopDemo() {
    if (!demoMode) {
      return;
    }
    demoMode = false;
    demoFinished = true;
    buffer[0] = '\0';
  }

  bool isFakeHighCo2() const { return fakeHighCo2; }

  void setFakeHighCo2(bool enabled) {
    fakeHighCo2 = enabled;
    fakeCo2RampFrom = fakeCo2Applied;
    fakeCo2RampTo = enabled ? (float)CO2_REALBAD : getSensorCo2();
    fakeCo2RampStartMs = millis();
    fakeCo2Ramping = true;
    log_i("Aquarium fake high CO2 %s — ramping %.0f -> %.0f ppm over %lu ms",
          fakeHighCo2 ? "on" : "off", fakeCo2RampFrom, fakeCo2RampTo,
          (unsigned long)kFakeCo2RampMs);
  }

  bool toggleFakeHighCo2() {
    setFakeHighCo2(!fakeHighCo2);
    return fakeHighCo2;
  }

  float getSensorCo2() const {
    return (scd40 && scd40->isFirstReadingReceived()) ? scd40->getCO2() : 400.0f;
  }

  void updateFakeCo2() {
    if (demoMode) {
      return;
    }
    if (fakeCo2Ramping) {
      const unsigned long elapsed = millis() - fakeCo2RampStartMs;
      if (elapsed >= kFakeCo2RampMs) {
        fakeCo2Applied = fakeCo2RampTo;
        fakeCo2Ramping = false;
      } else {
        const float t = (float)elapsed / (float)kFakeCo2RampMs;
        fakeCo2Applied = fakeCo2RampFrom + (fakeCo2RampTo - fakeCo2RampFrom) * t;
      }
    } else if (fakeHighCo2) {
      fakeCo2Applied = (float)CO2_REALBAD;
    } else {
      fakeCo2Applied = getSensorCo2();
    }
  }

  /** CO2 used by fish/boids: demo value, else smoothed applied value. */
  float getEffectiveCo2() const {
    if (demoMode) {
      return demoCO2;
    }
    return fakeCo2Applied;
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
    float co2 = getEffectiveCo2();

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

  void drawPerformanceModeOverlay() {
    unsigned long now = millis();
    if (performanceModeLabelUntilMs == 0 || now >= performanceModeLabelUntilMs) return;
    snprintf(buffer, sizeof(buffer), "Mode: %s", performanceModeName(performanceMode));
    drawMultilineText(matrix->foreground, buffer, TOP, TextAlignment::CENTER, &Font4x7Fixed,
                      CRGB(70, 210, 255));
  }

  void updateSensorData(bool showSensorData) {
    if (showSensorData && !demoMode) {
      if (scd40 && scd40->isFirstReadingReceived()) {
        float temperature = scd40->getTemperature();
        float humidity = scd40->getHumidity();
        float co2 = getEffectiveCo2();
        
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
    syncOrientation();
    updateFakeCo2();

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
      planktonField.update(interactionData);

      if (performanceMode == PerformanceMode::Ring) {
        updateRingPerformance(interactionData);
      } else if (performanceMode == PerformanceMode::Orchestra) {
        updateOrchestraPerformance(interactionData);
      } else if (performanceMode == PerformanceMode::Fountain) {
        updateFountainPerformance(interactionData);
      } else if (performanceMode == PerformanceMode::Jump) {
        updateJumpPerformance(interactionData);
      } else {
        updateWater();
        planktonField.draw(); 

        boidManager.updateBoids(getEffectiveCo2(), &interactionData);
        boidManager.renderBoids();

        updateFish(&interactionData);
        // updateFood();

        updatePlants();
      }
      updateSensorData(showSensorData);
      drawPerformanceModeOverlay();
      periodicSave();
    }
  }

  // The Ring: boids draw a small circular ring; fish orbit on concentric circles at several
  // radii. Interaction: a smoothed attractor drifts toward the palm; formation eases behind it.
  void updateRingPerformance(InteractionData& interaction) {
    float w = (float)matrix->getXResolution();
    float h = (float)matrix->getYResolution();
    PVector screenCenter(w * 0.5f, h * 0.5f);

    float baseRadius = min(w, h) * 0.5f - RING_MARGIN_PX;
    if (baseRadius < 1.0f) baseRadius = 1.0f;

    if (!ringAttractorInit) {
      ringAttractorTarget = screenCenter;
      ringAttractor = screenCenter;
      ringAttractorInit = true;
    }

    // Layer 1: ease the attractor *goal* toward palm or screen center (never snap).
    bool hasPalm = interaction.hasPalmHold && interaction.palmStrength > 0.01f;
    PVector goalTarget = screenCenter;
    if (hasPalm) {
      goalTarget = PVector(constrain(interaction.palmNormX, 0.0f, 1.0f) * w,
                           constrain(interaction.palmNormY, 0.0f, 1.0f) * h);
    }
    ringAttractorTarget += (goalTarget - ringAttractorTarget) * RING_ATTRACTOR_TARGET_SMOOTH;
    ringAttractorTarget.x =
        constrain(ringAttractorTarget.x, w * RING_ATTRACTOR_CLAMP_X0, w * RING_ATTRACTOR_CLAMP_X1);
    ringAttractorTarget.y =
        constrain(ringAttractorTarget.y, h * RING_ATTRACTOR_CLAMP_Y0, h * RING_ATTRACTOR_CLAMP_Y1);

    // Layer 2: formation center trails the goal (slower — the visible "pull" of the ring).
    float followRate = hasPalm ? RING_ATTRACTOR_FOLLOW : RING_ATTRACTOR_RETURN;
    ringAttractor += (ringAttractorTarget - ringAttractor) * followRate;

    float rotSpeed = RING_ROTATION_SPEED;
    if (hasPalm) {
      rotSpeed += interaction.palmVelocityMag * RING_VELOCITY_SPIN;
    }
    ringPhase += rotSpeed;

    static const float kLayerRadiusFrac[] = {0.50f, 0.68f, 0.84f, 0.98f};
    static const float kLayerSpeedMul[]   = {1.55f, 1.25f, 1.0f, 0.80f};
    constexpr int kLayers = 4;

    int n = (int)fishArray.size();
    for (int i = 0; i < n; i++) {
      int layer = i % kLayers;
      float r = baseRadius * kLayerRadiusFrac[layer];
      float theta = ringPhase * kLayerSpeedMul[layer] +
                    (TWO_PI * (float)i) / (float)(n > 0 ? n : 1);
      PVector slot(ringAttractor.x + r * cosf(theta), ringAttractor.y + r * sinf(theta));
      fishArray[i]->setFormationTarget(slot);
    }

    float co2 = getEffectiveCo2();
    float boidRadius = baseRadius * RING_BOID_RADIUS_FRAC;

    updateWater();
    boidManager.updateBoidsRing(co2, ringAttractor, boidRadius, ringPhase, &interaction);
    boidManager.renderBoids();
    updateFish(&interaction);
    updatePlants();
  }

  // Orchestra: each fish holds an assigned slot on a grid that mirrors the 8x8 TOF cells,
  // micro-orbiting its home so it stays alive (and keeps a defined heading). The conductor's
  // palm energizes nearby fish — they swing wider and the whole orchestra speeds up with hand
  // motion. Boids are a loose flock that follows the conductor's hand.
  void updateOrchestraPerformance(InteractionData& interaction) {
    float w = (float)matrix->getXResolution();
    float h = (float)matrix->getYResolution();

    bool hasPalm = interaction.hasPalmHold && interaction.palmStrength > 0.01f;
    PVector palmScreen(interaction.palmNormX * w, interaction.palmNormY * h);
    float influence = ORCH_INFLUENCE_FRAC * w;
    if (influence < 1.0f) influence = 1.0f;

    // Whole orchestra speeds up when the hand moves fast.
    float globalSwell = hasPalm ? constrain(interaction.palmVelocityMag, 0.0f, 1.0f) : 0.0f;
    orchestraPhase += ORCH_SPIN_BASE * (1.0f + ORCH_SPIN_VEL_GAIN * globalSwell);

    float mx = w * ORCH_MARGIN_X_FRAC;
    float my = h * ORCH_MARGIN_Y_FRAC;
    float cellW = (w - 2.0f * mx) / (float)ORCH_COLS;
    float cellH = (h - 2.0f * my) / (float)ORCH_ROWS;

    int n = (int)fishArray.size();
    for (int i = 0; i < n; i++) {
      int col = i % ORCH_COLS;
      int row = (i / ORCH_COLS) % ORCH_ROWS;
      PVector home(mx + ((float)col + 0.5f) * cellW, my + ((float)row + 0.5f) * cellH);

      // Proximity energy: 1 right at the hand, fading to 0 at the influence radius.
      float energy = 0.0f;
      if (hasPalm) {
        float dist = (home - palmScreen).mag();
        energy = 1.0f - constrain(dist / influence, 0.0f, 1.0f);
      }

      float microRadius = ORCH_MICRO_MIN + energy * (ORCH_MICRO_MAX - ORCH_MICRO_MIN);

      uint32_t hsh = (uint32_t)(i + 1) * 2654435761u;
      float offset = (float)(hsh % 6283u) * 0.001f;
      float mult = 0.85f + (float)((hsh >> 13) % 31u) * 0.01f;  // 0.85..1.15
      float dir = (col & 1) ? 1.0f : -1.0f;                      // alternate spin per column
      float angle = dir * orchestraPhase * mult + offset;

      PVector slot(home.x + microRadius * cosf(angle), home.y + microRadius * sinf(angle));
      fishArray[i]->setFormationTarget(slot);
    }

    float co2 = getEffectiveCo2();

    updateWater();
    // Boids follow the conductor: existing profile logic chases the palm when present.
    boidManager.updateBoids(co2, &interaction);
    boidManager.renderBoids();
    updateFish(&interaction);
    updatePlants();
  }

  // Fountain: hands raised — fish rise from wherever they are. Point attractors shape the
  // stream. Respawn only after exiting above the canvas; never teleport while visible.
  void updateFountainPerformance(InteractionData& interaction) {
    float w = (float)matrix->getXResolution();
    float h = (float)matrix->getYResolution();
    PVector center(w * 0.5f, h * 0.5f);
    bool active = interaction.handsRaised;

    for (auto& fish : fishArray) {
      fish->clearFormationTarget();
      fish->setFountainMode(active);
      if (!active) {
        fish->clearFountainSteering();
        fountainAnchorsPending = true;
      }
    }

    // First hands-up: set per-fish anchor points from current positions — do not teleport.
    if (active && fountainAnchorsPending) {
      for (auto& fish : fishArray) {
        PVector p = fish->getPosition();
        fish->setFountainAnchor(p.x, h + FOUNTAIN_RESPAWN_MARGIN_PX);
      }
      fountainAnchorsPending = false;
    }

    float co2 = getEffectiveCo2();

    updateWater();
    planktonField.draw();
    boidManager.updateBoidsFountain(co2, center, w, h, active);
    boidManager.renderBoids();

    InteractionData* fishInteraction = active ? nullptr : &interaction;

    std::vector<PVector> schoolPositions;
    schoolPositions.reserve(fishArray.size());
    for (const auto& fish : fishArray) {
      schoolPositions.push_back(fish->getPosition());
    }

    int fishIndex = 0;
    for (auto it = fishArray.begin(); it != fishArray.end();) {
      if (active) {
        (*it)->setFountainSteering(center, w, h);
      }

      bool destroy = (*it)->update(co2, demoMode, fishInteraction, &schoolPositions, fishIndex);

      if (active) {
        PVector p = (*it)->getPosition();
        if (p.y < -FOUNTAIN_RESPAWN_MARGIN_PX) {
          int n = (int)fishArray.size();
          float spawnX, spawnY;
          fountainSpawnPosition(fishIndex, n > 0 ? n : 1, w, h, spawnX, spawnY, true);
          (*it)->setPosition(PVector(spawnX, spawnY));
          (*it)->setFountainAnchor(spawnX, spawnY);
          (*it)->setVelocity(PVector(0, -FOUNTAIN_RESPAWN_RISE_SPEED));
        }
      }

      if (destroy) {
        it = fishArray.erase(it);
      } else {
        (*it)->display();
        ++it;
      }
      fishIndex++;
    }

    if (fishArray.size() < NUM_FISH_IDEAL) {
      for (auto& fish : fishArray) {
        if (fish->tryReproduce()) {
          PVector newPos = fish->getPosition();
          fishArray.emplace_back(std::make_unique<Fish>(matrix, newPos));
          break;
        }
      }
    }

    updatePlants();
  }

  // Jump: fish hold random homes inside a border buffer (orchestra-style formation) and
  // hop continuously. TOF grid |Δdepth| activity speeds up / amplifies the hops.
  void updateJumpPerformance(InteractionData& interaction) {
    float w = (float)matrix->getXResolution();
    float h = (float)matrix->getYResolution();

    float activity = stepJumpGridActivity(interaction.depthMap, TOF_MIN_DETECTION_DIST,
                                          TOF_MAX_DETECTION_DIST, jumpGridActivity);
    float spin = JUMP_SPIN_BASE + activity * (JUMP_SPIN_MAX - JUMP_SPIN_BASE);
    float hopAmp = JUMP_HOP_AMP_MIN + activity * (JUMP_HOP_AMP_MAX - JUMP_HOP_AMP_MIN);
    jumpPhase += spin;

    int n = (int)fishArray.size();
    for (int i = 0; i < n; i++) {
      float homeX, homeY;
      jumpHomePosition(i, w, h, homeX, homeY);

      uint32_t hsh = (uint32_t)(i + 1) * 2654435761u;
      float offset = (float)(hsh % 6283u) * 0.001f;
      float mult = 0.85f + (float)((hsh >> 13) % 31u) * 0.01f;
      float angle = jumpPhase * mult + offset;
      // Vertical-biased hop: sin drives Y, small cos for X sway.
      float sx = homeX + JUMP_HOP_WOBBLE_X * cosf(angle);
      float sy = homeY - hopAmp * fabsf(sinf(angle));  // hop up from home, land back
      fishArray[i]->setFormationTarget(PVector(sx, sy));
    }

    float co2 = getEffectiveCo2();

    updateWater();
    planktonField.draw();
    boidManager.updateBoids(co2, nullptr);
    boidManager.renderBoids();
    updateFish(&interaction);
    updatePlants();
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
      if (lastKnownRotation < 4) {
        interactionManager->setHandRaiseOrientation(lastKnownRotation);
      }
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