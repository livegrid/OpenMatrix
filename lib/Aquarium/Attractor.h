#ifndef ATTRACTOR_H
#define ATTRACTOR_H

#include <Arduino.h>
#include <Matrix.h>
#include <Vector.h>

#include <memory>
#include <vector>
#include "AquariumSettings.h"

class Attractor {
 private:
  PVector pos, vel, acc;

  uint16_t xResolution;
  uint16_t yResolution; 
  Matrix* matrix;

  bool outOfBoundary = false;

  float angleOffset = random(TWO_PI);

  float sinAngle;
 public:
  Attractor(Matrix* matrix, uint16_t xRes, uint16_t yRes) : matrix(matrix){
    xResolution = xRes * PHYSICS_SCALE;
    yResolution = yRes * PHYSICS_SCALE;
    pos = PVector(xResolution / 2, yResolution / 2);
  }

  void update() {
    boundaryCheck();
    if(!outOfBoundary) {
      sideSineMotion();
    }
    vel += acc;
    vel.setMag(constrain(vel.mag(), ATTRACTOR_MIN_SPEED, ATTRACTOR_MAX_SPEED));
    pos += vel;
    acc *= 0;
    // matrix->foreground->fillCircle(pos.x/PHYSICS_SCALE,pos.y/PHYSICS_SCALE, 2, CRGB(255, 0, 0));
  }

  PVector getPosition() const {
    return pos;
  }
  
  PVector getVelocity() const {
    return vel;
  }

protected:
  void applyForce(PVector force) {
    acc = acc + force;
  }

  void boundaryCheck() {
    outOfBoundary = false;
    if (pos.x < BORDER_BUFFER) {
      applyForce(PVector(BOUNDARY_FORCE, 0));
      outOfBoundary = true;
    }
    if (pos.y < BORDER_BUFFER) {
      applyForce(PVector(0, BOUNDARY_FORCE));
      outOfBoundary = true;
    }
    if (pos.x > xResolution - BORDER_BUFFER) {
      applyForce(PVector(-BOUNDARY_FORCE, 0));
      outOfBoundary = true;
    }
    if (pos.y > yResolution - BORDER_BUFFER) {
      applyForce(PVector(0, -BOUNDARY_FORCE));
      outOfBoundary = true;
    }
  }

  void sideSineMotion() {
    float theta = vel.heading() + PI / 2;
    float angle = sin(millis() * ATTRACTOR_SIN_FREQUENCY + angleOffset);
    float yOffset = angle * ATTRACTOR_SIN_AMPLITUDE;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    sinusoidalForce.limit(MAX_FORCE);
    applyForce(sinusoidalForce);
  }
};

#endif  // ATTRACTOR_H