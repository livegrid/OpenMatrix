#ifndef AQUARIUM_H
#define AQUARIUM_H

#include <Arduino.h>
#include <Matrix.h>
#include <scd40.h>

#include "AquariumSettings.h"
#include "Attractor.h"
#include "Fish.h"
#include "Food.h"
#include "Plants.h"
#include "Water.h"

class Aquarium {
 private:
  Matrix* matrix;
  SCD40* scd40;
  Water water;
  std::vector<std::unique_ptr<Fish>> fishArray;
  std::vector<std::unique_ptr<Plants>> plantArray;
  std::vector<std::unique_ptr<Food>> foodArray;
  std::vector<std::shared_ptr<Attractor>> attractorArray;

 public:
  // Constructor that initializes the matrix and water
  Aquarium(Matrix* m, SCD40* s) : matrix(m), water(matrix), scd40(s) {
    initializeFish();
    initializePlants();
    initializeAttractors();
  }

  // Initialize fish and store them in a vector of unique pointers
  void initializeFish() {
    PVector centerPos(matrix->getXResolution() / 2,
                      matrix->getYResolution() / 2);
    for (int i = 0; i < NUM_FISH_START; i++) {
      fishArray.emplace_back(std::make_unique<Fish>(
          matrix, &attractorArray, centerPos, random(0, 100) / 100.0));
    }
  }

  // Initialize plants and store them in a vector of unique pointers
  void initializePlants() {
    for (int i = 0; i < NUM_PLANTS; i++) {
      plantArray.emplace_back(std::make_unique<Plants>(
          matrix, matrix->getXResolution() * i / NUM_PLANTS,
          matrix->getYResolution() + 5));
    }
  }

  void initializeAttractors() {
    for (int i = 0; i < NUM_ATTRACTORS; ++i) {
      attractorArray.emplace_back(std::make_unique<Attractor>(
          matrix, matrix->getXResolution(), matrix->getYResolution()));
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
    for (auto& plant : plantArray) {
      plant->update(scd40->getHumidity());
    }
  }

  // Update the water environment
  void updateWater() {
    if (scd40->isConnected()) {
      water.update(scd40->getTemperature());
    } else {
      water.update();
    }
  }

  // Update all fish in the aquarium

  void updateFish() {
    for (auto it = fishArray.begin(); it != fishArray.end();) {
      bool destroy = (*it)->update(scd40->getCO2());
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
              std::make_unique<Fish>(matrix, &attractorArray, newPos));
          break;  // Only add one fish per update cycle
        }
      }
    }
  }

  void updateFood() {
    for (auto it = foodArray.begin(); it != foodArray.end();) {
      (*it)->update();
      (*it)->display();
      if ((*it)->isOffScreen() || (*it)->isEaten()) {
        it = foodArray.erase(it);
      } else {
        ++it;
      }
    }
  }

  void updateAttractors() {
    for (auto& attractor : attractorArray) {
      attractor->update();
    }
  }

  void handleTouchInput() {
    if (touchRead(12) / 1000 > 50) {
      addFood();
    }
  }

  // General update function that updates all components of the aquarium
  void update() {
    handleTouchInput();
    updateWater();
    updateFish();
    updateFood();
    updatePlants();
    updateAttractors();
  }

  // Destructor to clean up resources
  ~Aquarium() {
    // Unique pointers automatically clean up
  }
};

#endif  // AQUARIUM_H