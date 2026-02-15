#ifndef MOTION_H
#define MOTION_H

#include <AquariumSettings.h>
#include <Arduino.h>
#include <FastNoise.h>
#include <PVector.h>
#include <SCD40Settings.h>
#include "../TOFSensor/TOFInteractionManager.h"

class Motion {
 public:
  enum class InteractionState : uint8_t { IDLE, ALERT, SCARED, CURIOUS };

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

  // Interaction state machine
  InteractionState interactionState = InteractionState::IDLE;
  unsigned long stateStartTime = 0;
  unsigned long blobLostTime = 0;       // When blob was last seen (for IDLE recovery)
  unsigned long curiousPauseUntil = 0;  // Pause timer for CURIOUS pauses
  unsigned long curiousLastPauseCheck = 0;
  PVector lastBlobPos;                  // Cached blob position for SCARED flee direction

 public:
  Motion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : pos(pos), xResolution(xResolution), yResolution(yResolution) {}

  virtual ~Motion() {}

  float lerp(float start, float end, float t) {
    return (1 - t) * start + t * end;
  }

  PVector getPosition() { return pos; }
  PVector getVelocity() { return vel; }
  float getAngle() { return angle; }
  InteractionState getInteractionState() const { return interactionState; }

  virtual void doMotion() = 0;

  // Called from Fish::update() with the full interaction data
  void updateInteractionState(const InteractionData& interaction,
                              uint16_t physicsWidth, uint16_t physicsHeight) {
    unsigned long now = millis();
    float maxDist = (float)min(xResolution, yResolution) * INTERACTION_RADIUS_FRACTION;

    if (interaction.hasBlob) {
      PVector blobPos(interaction.blobX * physicsWidth,
                      interaction.blobY * physicsHeight);
      float distToBlob = (blobPos - pos).mag();
      blobLostTime = 0;  // Blob is present

      // Skip if out of interaction range
      if (distToBlob > maxDist || distToBlob < INTERACTION_DISTANCE_EPSILON) {
        return;
      }

      lastBlobPos = blobPos;

      switch (interactionState) {
        case InteractionState::IDLE:
          // Blob just appeared -- go ALERT
          interactionState = InteractionState::ALERT;
          stateStartTime = now;
          break;

        case InteractionState::ALERT:
          // After freeze duration, transition to SCARED
          if (now - stateStartTime >= STATE_ALERT_DURATION_MS) {
            interactionState = InteractionState::SCARED;
            stateStartTime = now;
          }
          break;

        case InteractionState::SCARED:
          // If blob is calm for long enough, transition to CURIOUS
          if (interaction.velocityMag < STATE_SCARED_CALM_VELOCITY &&
              interaction.presenceDuration >= STATE_SCARED_CALM_DURATION_S) {
            interactionState = InteractionState::CURIOUS;
            stateStartTime = now;
          }
          break;

        case InteractionState::CURIOUS:
          // Sudden fast movement snaps back to SCARED
          if (interaction.velocityMag > STATE_CURIOUS_SCARE_VELOCITY) {
            interactionState = InteractionState::SCARED;
            stateStartTime = now;
          }
          break;
      }
    } else {
      // No blob -- start recovery timer
      if (blobLostTime == 0) {
        blobLostTime = now;
      }
      if (interactionState != InteractionState::IDLE &&
          now - blobLostTime >= STATE_IDLE_RECOVERY_MS) {
        interactionState = InteractionState::IDLE;
        stateStartTime = now;
      }
    }
  }

  void update(float age = AGE_ADULT, long co2 = CO2_OK,
              bool stayInside = false) {
    this->co2 = co2;

    if (age < AGE_EGG) {
      vel = PVector(0, 0);
      return;
    }

    if (stayInside || co2 > CO2_BAD) {
      boundaryCheck(BOUNDARY_FORCE * 10);
    } else {
      boundaryCheck(BOUNDARY_FORCE);
    }

    unsigned long now = millis();

    // Real food always takes priority
    if (followingFood) {
      PVector fForce = foodDirection - pos;
      fForce.setMag(FOOD_FORCE);
      applyForce(fForce);
    } else if (outOfBoundary) {
      // boundary forces already applied, skip motion
    } else {
      // Interaction state drives motion when no food
      switch (interactionState) {
        case InteractionState::IDLE:
          doMotion();
          break;

        case InteractionState::ALERT:
          // Freeze: dampen velocity, skip doMotion
          vel *= STATE_ALERT_DAMPING;
          break;

        case InteractionState::SCARED: {
          // Dart away from blob
          PVector away = pos - lastBlobPos;
          if (away.mag() > INTERACTION_DISTANCE_EPSILON) {
            away.setMag(STATE_SCARED_REPEL_FORCE);
            applyForce(away);
          }
          // No doMotion -- pure directional dart
          break;
        }

        case InteractionState::CURIOUS: {
          // Check for curious pauses
          if (now - curiousLastPauseCheck > STATE_CURIOUS_PAUSE_INTERVAL_MS) {
            curiousLastPauseCheck = now;
            if (random(STATE_CURIOUS_PAUSE_CHANCE) < 1) {
              curiousPauseUntil = now + random(STATE_CURIOUS_PAUSE_MIN_MS,
                                               STATE_CURIOUS_PAUSE_MAX_MS);
            }
          }

          if (now < curiousPauseUntil) {
            // Pausing -- gentle drift
            vel *= STATE_CURIOUS_PAUSE_DAMPING;
          } else {
            // Gentle follow toward blob; stop attracting when close to avoid clustering
            PVector toBlob = lastBlobPos - pos;
            float distToBlob = toBlob.mag();
            if (distToBlob > INTERACTION_DISTANCE_EPSILON &&
                distToBlob > CURIOUS_ATTRACTION_STOP_DISTANCE) {
              toBlob.setMag(FOOD_FORCE * STATE_CURIOUS_FOLLOW_FORCE_FRAC);
              applyForce(toBlob);
            }
            // Reduced organic motion (still alive, just subtle)
            doMotionScaled(STATE_CURIOUS_SIN_SCALE, STATE_CURIOUS_NOISE_SCALE);
          }
          break;
        }
      }
    }

    PVector desiredVel = vel;
    desiredVel += acc;

    float minSpeedSq = (float)minSpeed * minSpeed;
    if (desiredVel.magSq() < minSpeedSq) {
      desiredVel.setMag(minSpeed);
    }

    float effectiveMaxSpeed = maxSpeed;
    if (interactionState == InteractionState::SCARED) {
      effectiveMaxSpeed = maxSpeed * STATE_SCARED_SPEED_BOOST;
    }

    float maxSpeedCO2 = map(co2, CO2_BAD, CO2_REALBAD, effectiveMaxSpeed, 0);
    if (maxSpeedCO2 < 0) maxSpeedCO2 = 0;
    else if (maxSpeedCO2 > effectiveMaxSpeed) maxSpeedCO2 = effectiveMaxSpeed;
    desiredVel.limit(maxSpeedCO2);

    vel = desiredVel;
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
    float a = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(a) * sinAmplitude;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);
  }

  void sideSineMotion() {
    float theta = vel.heading() + PI / 2;
    float a = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(a) * sinAmplitude;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);
  }

  void noiseMotion() {
    float noiseValue = noise.GetNoise(pos.x, pos.y);
    float noiseAngle = noiseValue * TWO_PI;
    PVector noiseForce = PVector::fromAngle(noiseAngle);
    noiseForce *= noiseAmplitude * noiseValue;
    applyForce(noiseForce);
  }

  // Scaled version of doMotion for CURIOUS state (reduced amplitudes)
  void doMotionScaled(float sinScale, float noiseScale) {
    // Side sine with reduced amplitude
    float theta = vel.heading() + PI / 2;
    float a = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(a) * sinAmplitude * sinScale;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);

    // Noise with reduced amplitude
    float noiseValue = noise.GetNoise(pos.x, pos.y);
    float noiseAngle = noiseValue * TWO_PI;
    PVector noiseForce = PVector::fromAngle(noiseAngle);
    noiseForce *= noiseAmplitude * noiseScale * noiseValue;
    applyForce(noiseForce);
  }

 public:
  void followFood(PVector foodPos) {
    foodDirection = foodPos;
    followingFood = true;
  }
};

#endif  // MOVEMENT_H
