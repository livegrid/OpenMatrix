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
  PVector interactionCenter;
  PVector interactionTarget;
  PVector interactionFlow;
  float interactionRadius = 0;
  float interactionStrength = 0;
  bool interactionPalmMode = false;
  int interactionSlotIndex = 0;

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
                              uint16_t physicsWidth, uint16_t physicsHeight,
                              int selfIndex = -1, int schoolCount = 1) {
    unsigned long now = millis();
    (void)schoolCount;

    bool hasInteraction = false;
#if AQUARIUM_TOF_PALM_INTERACTION_ENABLED
    interactionPalmMode = interaction.hasPalmHold && interaction.palmStrength > 0.01f;
#else
    interactionPalmMode = false;
#endif
    interactionStrength = interactionPalmMode ? interaction.palmStrength : 1.0f;
    interactionSlotIndex = selfIndex >= 0 ? selfIndex : 0;

    if (interactionPalmMode) {
      interactionCenter = PVector(constrain(interaction.palmNormX, 0.0f, 1.0f) * physicsWidth,
                                  constrain(interaction.palmNormY, 0.0f, 1.0f) * physicsHeight);
      interactionRadius = max((float)min(physicsWidth, physicsHeight) * PALM_ZONE_RADIUS_FRACTION,
                              (float)(12 * PHYSICS_SCALE));
      interactionFlow = PVector(interaction.palmVelocityX, interaction.palmVelocityY);
      hasInteraction = true;
    } else if (interaction.hasBlob) {
      int minX = 8, minY = 8, maxX = -1, maxY = -1;
      int activeCount = 0;
      for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
          int16_t depth = interaction.depthMap[y][x];
          if (depth > TOF_MIN_DETECTION_DIST && depth < TOF_MAX_DETECTION_DIST) {
            if ((int)x < minX) minX = x;
            if ((int)y < minY) minY = y;
            if ((int)x > maxX) maxX = x;
            if ((int)y > maxY) maxY = y;
            activeCount++;
          }
        }
      }

      if (activeCount > 0) {
        float centerNormX = ((float)minX + (float)maxX + 1.0f) * 0.5f / 8.0f;
        float centerNormY = ((float)minY + (float)maxY + 1.0f) * 0.5f / 8.0f;
        interactionCenter = PVector(centerNormX * physicsWidth, centerNormY * physicsHeight);

        float halfW = ((float)(maxX - minX + 1) / 8.0f) * physicsWidth * 0.5f;
        float halfH = ((float)(maxY - minY + 1) / 8.0f) * physicsHeight * 0.5f;
        interactionRadius = max(halfW, halfH) * (1.0f + BODY_ZONE_PADDING_FRACTION);
        interactionRadius = max(interactionRadius, (float)min(physicsWidth, physicsHeight) * 0.18f);
      } else {
        interactionCenter = PVector(interaction.blobX * physicsWidth,
                                    interaction.blobY * physicsHeight);
        interactionRadius = max((float)min(physicsWidth, physicsHeight) * 0.25f,
                                (float)(12 * PHYSICS_SCALE));
      }
      interactionFlow = PVector(0, 0);
      hasInteraction = true;
    }

    if (hasInteraction) {
      blobLostTime = 0;  // A body or held palm is present

      // Give every fish a deterministic personal slot in the interaction zone.
      // That keeps the school near the person/palm without collapsing to one point.
      uint32_t slotHash = (uint32_t)(interactionSlotIndex + 1) * 2654435761u;
      float angle = (float)(slotHash % 6283u) * 0.001f;
      float radiusJitter = 0.35f + (float)((slotHash >> 16) % 66u) * (0.65f / 65.0f);
      float slotRadius = interactionRadius *
                         (interactionPalmMode ? 0.55f : BODY_ZONE_SLOT_RADIUS_FRACTION) *
                         radiusJitter;
      interactionTarget = interactionCenter +
                          PVector(cosf(angle) * slotRadius, sinf(angle) * slotRadius);
      interactionTarget.x = constrain(interactionTarget.x, 0.0f, (float)physicsWidth);
      interactionTarget.y = constrain(interactionTarget.y, 0.0f, (float)physicsHeight);

      // Low-resolution TOF works best as a soft zone/flow cue, not as fear/freeze states.
      if (interactionState != InteractionState::CURIOUS) {
        interactionState = InteractionState::CURIOUS;
        stateStartTime = now;
      }
    } else {
      interactionPalmMode = false;
      interactionStrength = 0;
      interactionFlow = PVector(0, 0);

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
          // ALERT is currently bypassed by updateInteractionState(); keep fallback behavior stable.
          doMotion();
          break;

        case InteractionState::SCARED: {
          // SCARED is currently bypassed by updateInteractionState(); keep fallback behavior stable.
          doMotion();
          break;
        }

        case InteractionState::CURIOUS: {
          if (interactionPalmMode) {
            float flowMag = interactionFlow.mag();
            if (flowMag > FOLLOW_DIRECTION_MIN_VELOCITY) {
              PVector flowForce = interactionFlow / flowMag;
              float flowScale = constrain(flowMag, 0.25f, 1.0f);
              flowForce *= FOLLOW_DIRECTION_FORCE * flowScale * interactionStrength;
              applyForce(flowForce);
            }
          }

          PVector toTarget = interactionTarget - pos;
          float distToTarget = toTarget.mag();
          float distToCenter = (interactionCenter - pos).mag();
          bool outsideZone = distToCenter > interactionRadius;

          if (distToTarget > INTERACTION_DISTANCE_EPSILON) {
            float pullForce = interactionPalmMode
                                  ? FOLLOW_POSITION_BIAS
                                  : (outsideZone ? BODY_ZONE_EDGE_PULL_FORCE : BODY_ZONE_PULL_FORCE);
            bool shouldPull = outsideZone || distToTarget > interactionRadius * 0.55f;
            if (shouldPull) {
              toTarget.setMag(pullForce * interactionStrength);
              applyForce(toTarget);
            }
          }

          // Keep organic motion visible while leaving room for palm/body guidance.
          doMotionScaled(FOLLOW_IDLE_WOBBLE_SIN, FOLLOW_IDLE_WOBBLE_NOISE);
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
  void applyExternalForce(const PVector& force) {
    applyForce(force);
  }

  void followFood(PVector foodPos) {
    foodDirection = foodPos;
    followingFood = true;
  }
};

#endif  // MOVEMENT_H
