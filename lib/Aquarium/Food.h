#ifndef FOOD_H
#define FOOD_H

#include <Arduino.h>
#include <Matrix.h>
#include <Vector.h>

class Food {
 private:
  Matrix* matrix;
  PVector position;
  static constexpr float fallSpeed = 0.3;
  bool eaten = false;

 public:
  Food(Matrix* m, float x)
    : matrix(m), position(x, 0) {}

  void update() {
    position.y += fallSpeed;
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
    return position.y >= matrix->getYResolution();
  }

  void eat() {
    eaten = true;
  }

  bool isEaten() {
    return eaten;
  }
};

#endif  // FOOD_H