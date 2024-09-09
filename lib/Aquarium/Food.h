#ifndef FOOD_H
#define FOOD_H

#include <Arduino.h>
#include <Matrix.h>
#include <PVector.h>

class Food {
 private:
  Matrix* matrix;
  PVector position;
  static constexpr float fallSpeed = 0.3;
  bool eaten = false;
  bool offScreen = false;

 public:
  Food(Matrix* m, float x)
    : matrix(m), position(x, 0) {}

  void update() {
    position.y += fallSpeed;
    if (position.y >= matrix->getYResolution()) {
      offScreen = true;  // Set the flag instead of relying on real-time calculation
    }
  }

  void display() {
    if(!eaten) {
      matrix->foreground->drawPixel(position.x, position.y, CRGB(255, 255, 0));  // Yellow color for food
    }
  }

  PVector getPosition() {
    return position;
  }

  bool isOffScreen() {
    return offScreen;
  }

  void eat() {
    eaten = true;
  }

  bool isEaten() {
    return eaten;
  }
};

#endif  // FOOD_H