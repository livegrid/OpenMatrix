#ifndef FISH_H
#define FISH_H

#include <Arduino.h>
#include <Vector.h>
#include <Matrix.h>
#include <Sprites.h>
#include <random>
#include <vector>
#include <utility>
#include <chrono>
#include "Motion/MotionFactory.h"
#include "Body/BodyVariations/BodyFactory.h"
#include "Food.h"

//Probably not a good idea to create a new BodyFactory for every fish

class Fish {
  PVector pos;
  float age;
  Matrix* matrix = nullptr;
  BodyFactory* bodyFactory = nullptr;

  std::unique_ptr<Motion> motion;  // Use smart pointer for automatic memory management
  std::unique_ptr<Body> body;

  Food* food = nullptr;

  struct BodyMotionType {
    std::string bodyType;
    std::string motionType;
    double probability;
  };

public:
  Fish(Matrix* matrix, PVector pos = PVector(0,0), float age = 0, uint8_t health = 1)
  : matrix(matrix), pos(pos), age(age){ // Member initializer list{ // Default constructor
    BodyFactory bodyFactory(matrix);
    initializeAgingRate();
    // this->body = std::unique_ptr<Body>(bodyFactory.createRandomBody());

    std::vector<BodyMotionType> types = {
      {"Fish", "Fish", 0.5},
      {"Star", "Star", 0.3},
      {"Turtle", "Turtle", 0.2},
      {"Snake", "Snake", 0.3}
    };
    
    auto selectedType = selectType(types);

    this->body = std::unique_ptr<Body>(bodyFactory.createBody(selectedType.bodyType));
    this->motion = MotionFactory::createMotion(selectedType.motionType, pos * PHYSICS_SCALE, matrix->getXResolution() * PHYSICS_SCALE, matrix->getYResolution() * PHYSICS_SCALE);
    bodyFactory.~BodyFactory();
  }
  // ~Fish(); // Destructor

  bool update(long co2 = 600) {
    motion->update(age, co2);
    pos = motion->getPosition() / PHYSICS_SCALE;
    body->update(pos, motion->getVelocity(), motion->getAngle(), age, co2);
    updateAge(co2);
    if(food != nullptr) {
      PVector foodDistance = food->getPosition() - pos;
      if(food->isOffScreen()) {
          food = nullptr;
      }
      else if(foodDistance.mag() < 1) {
        food->eat();
        food = nullptr;
      }
      else {
        motion->followFood(food->getPosition() * PHYSICS_SCALE);
        // matrix->foreground->drawLine(pos.x,pos.y,food->getPosition().x,food->getPosition().y,255,0,0); 
      }
    }
    if (age > 1.0) age = 0;
    return age > 1.0;
  }

  bool tryReproduce() {
    // Simple reproduction chance based on age
    // Fish can only reproduce if they're between 0.3 and 0.7 age
    if (age > 0.5 && age < 0.9) {
      auto seed = std::chrono::system_clock::now().time_since_epoch().count();
      std::mt19937 gen(seed);
      std::uniform_real_distribution<> dis(0.0, 1.0);
      return dis(gen) < 0.01;  // 1% chance of reproduction per update
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
    body->display();
  }

  float getAge() {
    return age;
  }

  void setFood(Food* assignedFood) {
    // motion->setFood(food);
    food = assignedFood;
  }

  Food* getFood() {
    return food;
  }

private:
  unsigned long lastUpdateTime = millis();
  float agingRate;
  
  void initializeAgingRate() {
    float baseRate = 1.0f / (FISH_LIFESPAN_DAYS * 24 * 60 * 60 * 1000);  // Convert days to milliseconds
    float variation = FISH_LIFESPAN_VARIATION * ((float)random(200) / 100.0f - 1.0f);  // Random variation between -20% and +20%
    agingRate = baseRate * (1.0f + variation);
  }

  void updateAge(long co2) {
    unsigned long currentTime = millis();
    float timeDiff = currentTime - lastUpdateTime;
    if (co2 < 2000) {
      age += timeDiff * agingRate;
    }
    lastUpdateTime = currentTime;
  }

  BodyMotionType selectType(const std::vector<BodyMotionType>& types) {
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);

    // Extract probabilities
    std::vector<double> probabilities;
    for (const auto& type : types) {
      probabilities.push_back(type.probability);
    }

    std::discrete_distribution<> dist(probabilities.begin(), probabilities.end());
    return types[dist(gen)];
  }
};

#endif // FISH_H
