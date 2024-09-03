#ifndef MOTION_H
#define MOTION_H

#include <Arduino.h>
#include <Vector.h>
#include <FastNoise.h>
#include <AquariumSettings.h>
#include <SCD40Settings.h>

class Motion {
 protected:
  PVector pos, vel, acc;
  float angle;

  int8_t maxForce;
  float boundaryForce = BOUNDARY_FORCE;  // change to .2 later
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

  bool followingFood = false;

 public:
  Motion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : pos(pos), xResolution(xResolution), yResolution(yResolution) {}

  virtual ~Motion() {}                 // Virtual destructor for proper cleanup

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

  void update(float age = AGE_ADULT, long co2 = CO2_OK) {
    this->co2 = co2;
    boundaryCheck();
    if(age < AGE_EGG) {
      vel = PVector(0,0);
    }
    else if(age < AGE_TEEN) {
      flock();
      noiseMotion();
    }
    else if(!followingFood){
      if(!outOfBoundary) {
        doMotion();
      }
      followingFood = false;
    }

    PVector desiredVel = vel;
    
    desiredVel += acc;
    if(desiredVel.mag() < minSpeed) {
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
  }

protected:
  void applyForce(PVector force) {
    acc = acc + force;
  }

  void boundaryCheck() {
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

  void shiver() {
    float theta = vel.heading();
    float jitterAngle = (millis() % 100) * PI / 50;  // Generates a rapid, jerky motion
    float jitterStrength = 10;  // Strong, abrupt force
    PVector jitterForce = PVector::fromAngle(theta + jitterAngle) * jitterStrength;
    applyForce(jitterForce);
  }

  void noiseMotion() {
    float noiseValue = noise.GetNoise(pos.x, pos.y);
    float noiseAngle = noiseValue * TWO_PI;  // Map noise to a full circle
    PVector noiseForce = PVector::fromAngle(noiseAngle);
    noiseForce *= noiseAmplitude * noiseValue;  // Scale by noise strength and maxForce
    applyForce(noiseForce);
  }

  void flock() {
  }

  void separate() {
  }

  void align() {
  }

  void cohere() {
  }

public:
  void followFood(PVector foodDirection) {
    PVector forceToApply = foodDirection - pos;
    forceToApply.setMag(foodForce);
    applyForce(forceToApply);
    followingFood = true;
  }
};

#endif  // MOVEMENT_H
