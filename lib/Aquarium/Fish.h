#ifndef FISH_H
#define FISH_H

#include <Arduino.h>
#include <Matrix.h>
#include <PVector.h>

#include <chrono>
#include <random>
#include <utility>
#include <vector>

#include "AquariumSettings.h"
#include "Body/BodyVariations/BodyFactory.h"
#include "Food.h"
#include "Motion/MotionFactory.h"

class Fish {
  Matrix* matrix = nullptr;
  PVector pos;
  // Double so per-frame steps (~1e-8 of a 7-day life at 30 fps) are not
  // rounded away. In float those steps stall, then a slow frame jumps
  // past AGE_DEAD and the old wrap below turned the fish back into an egg.
  double age;
  float health;
  BodyFactory* bodyFactory = nullptr;
  uint8_t offspringCount = 0;

  std::unique_ptr<Motion>
      motion;  // Use smart pointer for automatic memory management
  std::unique_ptr<Body> body;

  Food* food = nullptr;

  struct BodyMotionType {
    String bodyType;
    String motionType;
    double probability;
  };

 public:
  struct FishDefinition {
    float age;
    float health;
    String bodyType;
    String headType;
    String tailType;
    String finType;
    String motionType;
    std::vector<CHSV> colors;
  };

  FishDefinition fishDefinition;
  const std::vector<CHSV>& getColorsHSV() const {
    return body->getColorPaletteHSV();
  }

  Fish(Matrix* matrix, const FishDefinition& fishDef)
      : matrix(matrix),
        pos(fishDef.age < AGE_EGG ? randomInsidePosition(matrix)
                                  : randomOutsideEdgePosition(matrix)),
        age(fishDef.age),
        health(fishDef.health) {  // Member initializer list
    BodyFactory bodyFactory(matrix);
    initializeAgingRate();

    this->body = std::unique_ptr<Body>(bodyFactory.createBody(
        fishDef.bodyType, fishDef.headType, fishDef.tailType, fishDef.finType));
    this->motion =
        MotionFactory::createMotion(fishDef.motionType, pos * PHYSICS_SCALE,
                                    matrix->getXResolution() * PHYSICS_SCALE,
                                    matrix->getYResolution() * PHYSICS_SCALE);

    fishDefinition = fishDef;
    // Unusable saved fish are dropped by the loader via isRestorable().
    if (!this->body || !this->motion) {
      log_e("Saved fish is not usable (body=%s, motion=%s)",
            fishDef.bodyType.c_str(), fishDef.motionType.c_str());
      return;
    }

    if (!fishDef.colors.empty()) {
      body->setColorPaletteHSV(fishDef.colors);
    }
  }

  Fish(Matrix* matrix, PVector pos = PVector(0, 0), float age = 0,
       float health = 1, const String& forcedType = "")
      : matrix(matrix), pos(pos), age(age), health(health) {
    BodyFactory bodyFactory(matrix);
    initializeAgingRate();

    // Default / unspecified spawn: enter from a random outside edge.
    // Do this before creating motion so physics starts at the edge position.
    if (pos.x == 0 && pos.y == 0) {
      this->pos = randomOutsideEdgePosition(matrix);
    }

    // 50% Fish, 10% each of the other 5 creatures.
    std::vector<BodyMotionType> types = {{"Fish", "Fish", 0.5},
                                         {"Turtle", "Turtle", 0.1},
                                         {"Squid", "Squid", 0.1},
                                         {"Octopus", "Octopus", 0.1},
                                         {"Star", "Star", 0.1},
                                         {"Snake", "Snake", 0.1}};

    BodyMotionType selectedType =
        forcedType.length() > 0 ? BodyMotionType{forcedType, forcedType, 1.0}
                                : selectType(types);

    this->body =
        std::unique_ptr<Body>(bodyFactory.createBody(selectedType.bodyType));
    if (!this->body) {
      // Fallback to a default body type if creation fails
      this->body = std::unique_ptr<Body>(bodyFactory.createBody("Fish"));
    }

    // Ensure the motion is created successfully
    this->motion = MotionFactory::createMotion(
        selectedType.motionType, this->pos * PHYSICS_SCALE,
        matrix->getXResolution() * PHYSICS_SCALE,
        matrix->getYResolution() * PHYSICS_SCALE);
    if (!this->motion) {
      // Fallback to a default motion type if creation fails
      this->motion = MotionFactory::createMotion(
          "Fish", this->pos * PHYSICS_SCALE,
          matrix->getXResolution() * PHYSICS_SCALE,
          matrix->getYResolution() * PHYSICS_SCALE);
    }

    fishDefinition = {static_cast<float>(age),
                      health,
                      selectedType.bodyType,
                      this->body->getHeadType(),
                      this->body->getTailType(),
                      this->body->getFinType(),
                      selectedType.motionType,
                      this->body->getColorPaletteHSV()};
  }
  // ~Fish(); // Destructor

  void setTankSize(uint16_t width, uint16_t height) {
    if (motion) {
      motion->setResolution(width * PHYSICS_SCALE, height * PHYSICS_SCALE);
      pos = motion->getPosition() / PHYSICS_SCALE;
    }
  }

  bool update(long co2 = 600, bool stayInside = false) {
    if (!motion) {
      log_e(
          "Error: motion is null in Fish::update for fish type: %s, motion "
          "type: %s",
          fishDefinition.bodyType.c_str(), fishDefinition.motionType.c_str());
      // Attempt to recreate the motion object
      this->motion = MotionFactory::createMotion(
          fishDefinition.motionType, pos * PHYSICS_SCALE,
          matrix->getXResolution() * PHYSICS_SCALE,
          matrix->getYResolution() * PHYSICS_SCALE);
      if (!this->motion) {
        log_e("Failed to recreate motion for fish type: %s, motion type: %s",
              fishDefinition.bodyType.c_str(),
              fishDefinition.motionType.c_str());
        // If recreation fails, return early to prevent crash
        return false;
      } else {
        log_i(
            "Successfully recreated motion for fish type: %s, motion type: %s",
            fishDefinition.bodyType.c_str(), fishDefinition.motionType.c_str());
      }
    }
    motion->update(age, co2, stayInside);
    pos = motion->getPosition() / PHYSICS_SCALE;
    updateAge(co2);
    updateHealth(co2);
    body->update(pos, motion->getVelocity(), motion->getAngle(), age, health,
                 motion->getStrokePhase(), motion->getTurnRate());
    if (food != nullptr) {
      PVector foodDistance = food->getPosition() - pos;
      if (food->isOffScreen()) {
        food = nullptr;
      } else if (foodDistance.mag() < 1) {
        food->eat();
        food = nullptr;
      } else {
        motion->followFood(food->getPosition() * PHYSICS_SCALE);
        // matrix->foreground->drawLine(pos.x,pos.y,food->getPosition().x,food->getPosition().y,
        // CRGB(255,0,0));
      }
    }
    // True means Aquarium::updateFish should erase this fish.
    // Do not wrap age back to 0 first: that made this check unreachable,
    // so fish respawned as eggs instead of dying.
    return age >= AGE_DEAD;
  }

  bool tryReproduce() {
    // Fish can only reproduce if they're between 0.5 and 0.9 age and have less
    // than 2 offspring
    if (age > 0.5 && age < 0.9 && offspringCount < 2) {
      auto seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::mt19937 gen(seed);
      std::uniform_real_distribution<> dis(0.0, 1.0);
      if (dis(gen) < 0.01) {  // 1% chance of reproduction per update
        offspringCount++;     // Increment offspring count
        return true;
      }
    }
    return false;
  }

  PVector getPosition() const {
    return PVector(pos.x, pos.y);
  }

  PVector getVelocity() const {
    return motion->getVelocity();
  }

  void display() {
    if (age < AGE_EGG)
      body->displayEgg();
    else
      body->display();
  }

  double getAge() const {
    return age;
  }

  void setFood(Food* assignedFood) {
    food = assignedFood;
  }

  Food* getFood() {
    return food;
  }

  bool isRestorable() const {
    return body != nullptr && motion != nullptr;
  }

  float getHealth() const {
    return health;
  }
  String getBodyType() const {
    return fishDefinition.bodyType;
  }
  String getHeadType() const {
    return fishDefinition.headType;
  }
  String getTailType() const {
    return fishDefinition.tailType;
  }
  String getFinType() const {
    return fishDefinition.finType;
  }
  String getMotionType() const {
    return fishDefinition.motionType;
  }

 private:
  unsigned long lastUpdateTime = millis();
  double agingRate = 0;

  static PVector randomInsidePosition(Matrix* matrix) {
    const long w = max(1L, (long)matrix->getXResolution());
    const long h = max(1L, (long)matrix->getYResolution());
    const long marginX = w > 6 ? 3 : 0;
    const long marginY = h > 6 ? 3 : 0;
    return PVector((float)random(marginX, w - marginX),
                   (float)random(marginY, h - marginY));
  }

  static PVector randomOutsideEdgePosition(Matrix* matrix) {
    const float w = (float)matrix->getXResolution();
    const float h = (float)matrix->getYResolution();
    const float margin = FISH_SPAWN_EDGE_MARGIN;
    switch (random(0, 4)) {
      case 0:  // top
        return PVector(random(0, (long)w), -margin);
      case 1:  // bottom
        return PVector(random(0, (long)w), h - 1.0f + margin);
      case 2:  // left
        return PVector(-margin, random(0, (long)h));
      default:  // right
        return PVector(w - 1.0f + margin, random(0, (long)h));
    }
  }

  void initializeAgingRate() {
    const double lifespanMs =
        (double)FISH_LIFESPAN_DAYS * 24.0 * 60.0 * 60.0 * 1000.0;
    const double baseRate = 1.0 / lifespanMs;
    // random(200)/100 - 1 is in [-1, 0.99), then scaled by the variation.
    const double variation = (double)FISH_LIFESPAN_VARIATION *
                             ((double)random(200) / 100.0 - 1.0);
    double multiplier = 1.0 + variation;
    // Below this, age barely moves. A zero multiplier never reaches death.
    if (multiplier < 0.05) {
      multiplier = 0.05;
    }
    agingRate = baseRate * multiplier;
  }

  void updateAge(long co2) {
    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastUpdateTime;
    if (co2 < 2000) {
      age += (double)timeDiff * agingRate;
    }
    lastUpdateTime = currentTime;
  }

  void updateHealth(long co2) {
    if (co2 >= CO2_REALBAD) {
      health -= HEALTH_REDUCTION_RATE_REALBAD;
    } else if (co2 >= CO2_BAD) {
      health -= HEALTH_REDUCTION_RATE_BAD;
    } else if (co2 < CO2_BAD) {
      health += HEALTH_INCREASE_RATE_GOOD;
    }
    health = max(0.0f, min(1.0f, health));
  }

  BodyMotionType selectType(const std::vector<BodyMotionType>& types) {
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);

    // Extract probabilities
    std::vector<double> probabilities;
    for (const auto& type : types) {
      probabilities.push_back(type.probability);
    }

    std::discrete_distribution<> dist(probabilities.begin(),
                                      probabilities.end());
    return types[dist(gen)];
  }
};

#endif  // FISH_H
