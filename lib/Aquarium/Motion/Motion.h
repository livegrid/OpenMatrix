#ifndef MOTION_H
#define MOTION_H

#include <AquariumSettings.h>
#include <Arduino.h>
#include <FastNoise.h>
#include <PVector.h>
#include <SCD40Settings.h>

class Motion {
 protected:
  PVector pos, vel, acc;
  float angle;

  int8_t maxForce;
  float foodForce;
  int8_t maxSpeed;
  int8_t minSpeed;
  float maxforce;

  bool outOfBoundary = false;

  PVector sinDeviation;
  int8_t sinAmplitude;
  float sinFrequency;
  float angleOffset = random(TWO_PI);
  FastNoiseLite noise;
  float noiseAmplitude;
  float noiseFrequency;
  long co2;

  uint16_t xResolution;
  uint16_t yResolution;

  PVector foodDirection;

  bool followingFood = false;

 public:
  Motion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : pos(pos), xResolution(xResolution), yResolution(yResolution) {}

  virtual ~Motion() {}  // Virtual destructor for proper cleanup

  float lerp(float start, float end, float t) {
    return (1 - t) * start + t * end;
  }

  PVector getPosition() {
    return pos;
  }

  PVector getVelocity() {
    return vel;
  }

  float getAngle() {
    return angle;
  }

  virtual void doMotion() = 0;

  void update(float age = AGE_ADULT, long co2 = CO2_OK,
              bool stayInside = false) {
    this->co2 = co2;

    if (age < AGE_EGG) {
      vel = PVector(0, 0);
      return;  // Exit early if it's an egg
    }

    if (stayInside || co2 > CO2_BAD) {
      boundaryCheck(BOUNDARY_FORCE * 10);
    } else {
      boundaryCheck(BOUNDARY_FORCE);
    }
    if (!followingFood && !outOfBoundary) {
      doMotion();
    } else if (followingFood) {
      PVector foodForce = foodDirection - pos;
      foodForce.setMag(FOOD_FORCE);
      applyForce(foodForce);
    } else if (followingFood) {
      PVector foodForce = foodDirection - pos;
      foodForce.setMag(FOOD_FORCE);
      applyForce(foodForce);
    }

    PVector desiredVel = vel;

    desiredVel += acc;
    if (desiredVel.mag() < minSpeed) {
      desiredVel.setMag(minSpeed);
    }
    float maxSpeedCO2 = map(co2, CO2_BAD, CO2_REALBAD, maxSpeed, 0);
    if (maxSpeedCO2 < 0) {
      maxSpeedCO2 = 0;
    } else if (maxSpeedCO2 > maxSpeed) {
      maxSpeedCO2 = maxSpeed;
    }
    desiredVel.limit(maxSpeedCO2);

    vel = vel.lerp(vel, desiredVel, 1);
    pos += vel;
    acc *= 0;
    angle = vel.heading();

    followingFood = false;
  }

 protected:
  void applyForce(PVector force) {
    acc = acc + force;
  }

  void boundaryCheck(float boundaryForce = BOUNDARY_FORCE) {
    outOfBoundary = false;
    if (pos.x < BORDER_BUFFER) {
      applyForce(PVector(boundaryForce, 0));
      outOfBoundary = true;
    }
    if (pos.y < BORDER_BUFFER) {
      applyForce(PVector(0, boundaryForce));
      outOfBoundary = true;
    }
    if (pos.x > xResolution - BORDER_BUFFER) {
      applyForce(PVector(-boundaryForce, 0));
      outOfBoundary = true;
    }
    if (pos.y > yResolution - BORDER_BUFFER) {
      applyForce(PVector(0, -boundaryForce));
      outOfBoundary = true;
    }
  }

  void frontSineMotion() {
    float theta = vel.heading();
    float angle = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(angle) * sinAmplitude;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);
  }

  void sideSineMotion() {
    float theta = vel.heading() + PI / 2;
    float angle = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(angle) * sinAmplitude;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);
  }

  void noiseMotion() {
    float noiseValue = noise.GetNoise(pos.x, pos.y);
    float noiseAngle = noiseValue * TWO_PI;  // Map noise to a full circle
    PVector noiseForce = PVector::fromAngle(noiseAngle);
    noiseForce *=
        noiseAmplitude * noiseValue;  // Scale by noise strength and maxForce
    applyForce(noiseForce);
  }

 public:
  void followFood(PVector foodPos) {
    foodDirection = foodPos;
    followingFood = true;
  }
};

#endif  // MOVEMENT_H
