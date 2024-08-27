#ifndef MOTION_H
#define MOTION_H

#include <Arduino.h>
#include <Vector.h>
#include <FastNoise.h>
#include <AquariumSettings.h>
#include <SCD40Settings.h>
#include "Attractor.h"

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
  float sinAngle;
  long co2;

  uint16_t xResolution;
  uint16_t yResolution; 

  bool followingFood = false;

  std::vector<std::shared_ptr<Attractor>>* attractors;  // Pointer to a vector of shared pointers to Attractors

 public:
  Motion(PVector pos, uint16_t xResolution, uint16_t yResolution, std::vector<std::shared_ptr<Attractor>>* attractors)
      : pos(pos), xResolution(xResolution), yResolution(yResolution), attractors(attractors) {}

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

  float getSinAngle() {
    return sinAngle;
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
    sinAngle = sin(millis() * sinFrequency + angleOffset);
    float yOffset = sinAngle * sinAmplitude;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);
  }

  void sideSineMotion() {
    float theta = vel.heading() + PI / 2;
    float angle = sin(millis() * sinFrequency + angleOffset);
    float yOffset = angle * sinAmplitude;
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
    if(attractors->size() > 0) {
      separate();
      // align();
      cohere();
    }
  }

  void separate() {
    PVector steer(0, 0);
    for (std::shared_ptr<Attractor> other : *attractors) {
      float d = pos.dist(other->getPosition());
      if ((d > 0) && (d < SEPARATION_DISTANCE * PHYSICS_SCALE)) {
        PVector diff = pos - other->getPosition();
        diff.normalize();
        diff /= d;
        steer += diff;
      }
    }
    steer /= attractors->size();
    steer *= SEPARATION_WEIGHT;
    steer.limit(FLOCK_MAX_FORCE);
    applyForce(steer);
  }

  void align() {
    PVector sum(0, 0);
    for (std::shared_ptr<Attractor> other : *attractors) {
      float d = pos.dist(other->getPosition());
      if ((d > 0) && (d < ALIGNMENT_DISTANCE * PHYSICS_SCALE)) {
        sum += other->getVelocity();
      }
    }
    sum /= attractors->size();
    sum *= ALIGNMENT_WEIGHT;
    sum.limit(FLOCK_MAX_FORCE);
    applyForce(sum);
  }

  void cohere() {
    PVector sum(0, 0);
    for (std::shared_ptr<Attractor> other : *attractors) {
      float d = pos.dist(other->getPosition());
      if ((d > 0) && (d < COHESION_DISTANCE * PHYSICS_SCALE)) {
        sum += other->getPosition() - pos;
      }
    }
    sum /= attractors->size();
    sum *= COHESION_WEIGHT;
    sum.limit(FLOCK_MAX_FORCE);
    applyForce(sum);
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
