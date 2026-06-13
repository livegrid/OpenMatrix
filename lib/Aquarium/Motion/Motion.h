#ifndef MOTION_H
#define MOTION_H

#include <AquariumSettings.h>
#include <Arduino.h>
#include <FastNoise.h>
#include <PVector.h>
#include <SCD40Settings.h>
#include "../TOFSensor/TOFInteractionManager.h"
#include "MotionProfile.h"

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

  // Interaction state machine (drives profile selection when debug is off)
  InteractionState interactionState = InteractionState::IDLE;
  MotionProfile activeProfile = MotionProfile::Wander;
  unsigned long stateStartTime = 0;
  unsigned long blobLostTime = 0;
  PVector lastBlobPos;
  PVector interactionCenter;
  PVector interactionTarget;
  PVector palmChaseTarget;
  PVector interactionFlow;
  float interactionRadius = 0;
  float interactionStrength = 0;
  bool interactionPalmMode = false;
  bool hasBodyBlob = false;
  TofDistanceHint distanceHint = TofDistanceHint::NoPerson;
  int interactionSlotIndex = 0;
  float holdEagerness = 0.0f;
  unsigned long holdUntil = 0;
  bool fleeLatched = false;
  unsigned long fleeClearAfterMs = 0;

  bool isOffCanvas() const {
    return pos.x < 0.0f || pos.y < 0.0f ||
           pos.x >= (float)xResolution || pos.y >= (float)yResolution;
  }

  bool shouldFlee() {
    unsigned long now = millis();
    if (isOffCanvas()) return false;
    if (isTooClose()) {
      fleeLatched = true;
      fleeClearAfterMs = 0;
      return true;
    }
    if (!fleeLatched) return false;

    if (hasBodyBlob && distanceHint == TofDistanceHint::Ok) {
      if (fleeClearAfterMs == 0) {
        fleeClearAfterMs = now + TOF_FLEE_CLEAR_MS;
      }
      if (now >= fleeClearAfterMs) {
        fleeLatched = false;
        fleeClearAfterMs = 0;
        return false;
      }
      return true;
    }

    fleeClearAfterMs = 0;
    return true;
  }

  bool hasInteractionTarget() const {
    return interactionRadius > 0.0f;
  }

  bool isInsideInteractionZone() const {
    if (!hasInteractionTarget()) return false;
    return (interactionCenter - pos).mag() <= interactionRadius;
  }

  void initHoldEagernessIfNeeded() {
    if (holdEagerness > 0.0f) return;
    uint32_t h = (uint32_t)(interactionSlotIndex + 1) * 2654435761u;
    holdEagerness = 0.35f + (float)((h >> 8) % 65u) * (0.65f / 65.0f);
  }

  float getHoldRadius() const {
    return interactionRadius * PROFILE_HOLD_SLOT_RADIUS_FRAC * holdEagerness;
  }

  bool shouldHoldAtSlot() {
    if (!hasInteractionTarget() || interactionPalmMode) return false;

    float distToTarget = (interactionTarget - pos).mag();
    float holdRadius = getHoldRadius();
    unsigned long now = millis();

    if (!isInsideInteractionZone() || distToTarget >= holdRadius) {
      holdUntil = 0;
      return false;
    }

    if (holdUntil == 0 && distToTarget < holdRadius * 0.85f) {
      uint32_t h = (uint32_t)(interactionSlotIndex + 1) * 1597334677u;
      unsigned long dwell = PROFILE_HOLD_MIN_MS +
                          (unsigned long)((h >> 12) % (PROFILE_HOLD_MAX_MS - PROFILE_HOLD_MIN_MS + 1));
      holdUntil = now + dwell;
    }

    return holdUntil > 0 && now < holdUntil;
  }

  PVector getDebugSteerTarget() const {
#if MOTION_PROFILE_DEBUG_ENABLED
    return PVector(MotionProfileDebug::getTargetX(), MotionProfileDebug::getTargetY());
#else
    return PVector(xResolution * 0.5f, yResolution * 0.5f);
#endif
  }

  PVector getChaseTarget() const {
    if (followingFood) return foodDirection;
    if (interactionPalmMode) return palmChaseTarget;
    if (hasInteractionTarget()) return interactionTarget;

    PVector center = getDebugSteerTarget();
    uint32_t slotHash = (uint32_t)(interactionSlotIndex + 1) * 2654435761u;
    float slotAngle = (float)(slotHash % 6283u) * 0.001f;
    float spread = PROFILE_CHASE_SLOT_SPREAD;
    return center + PVector(cosf(slotAngle) * spread, sinf(slotAngle) * spread);
  }

  PVector getFleeThreatPoint() const {
    if (hasBodyBlob) return interactionCenter;
    if (hasInteractionTarget()) return interactionCenter;
    return getDebugSteerTarget();
  }

  bool isTooClose() const {
    return hasBodyBlob && distanceHint == TofDistanceHint::TooClose;
  }

  bool isNearChaseTarget() const {
    return (getChaseTarget() - pos).mag() < PROFILE_CHASE_ARRIVE_DISTANCE;
  }

  void applySeekForce(const PVector& target, float forceMag) {
    PVector to = target - pos;
    if (to.magSq() <= INTERACTION_DISTANCE_EPSILON) return;
    to.setMag(forceMag);
    applyForce(to);
  }

  void applyFleeForce(const PVector& threat, float forceMag) {
    PVector away = pos - threat;
    if (away.magSq() <= INTERACTION_DISTANCE_EPSILON) {
      away = PVector::fromAngle(angle + PI);
    }
    away.setMag(forceMag);
    applyForce(away);
  }

  void applyProfileOrganic(const MotionProfileSpec& spec) {
    switch (spec.organic) {
      case OrganicMotion::Off:
        break;
      case OrganicMotion::Low:
        doMotionScaled(spec.sinScale, spec.noiseScale);
        break;
      case OrganicMotion::Full:
      default:
        doMotion();
        break;
    }
  }

  void applyApproachSteering() {
    PVector toTarget = interactionTarget - pos;
    float distToTarget = toTarget.mag();
    bool outsideZone = !isInsideInteractionZone();

    if (distToTarget > INTERACTION_DISTANCE_EPSILON) {
      float pullForce =
          outsideZone ? BODY_ZONE_EDGE_PULL_FORCE : BODY_ZONE_PULL_FORCE;
      bool shouldPull = outsideZone ||
                        distToTarget > interactionRadius * 0.55f;
      if (shouldPull) {
        toTarget.setMag(pullForce * interactionStrength);
        applyForce(toTarget);
      }
    }
  }

  void applyFriendSteering() {
#if AQUARIUM_TOF_PALM_INTERACTION_ENABLED
    float flowMag = interactionFlow.mag();
    if (flowMag > FOLLOW_DIRECTION_MIN_VELOCITY) {
      PVector flowForce = interactionFlow / flowMag;
      float flowScale = constrain(flowMag, 0.25f, 1.0f);
      flowForce *= FOLLOW_DIRECTION_FORCE * flowScale * interactionStrength * 0.5f;
      applyForce(flowForce);
    }
#endif
    if (hasInteractionTarget()) {
      PVector toTarget = interactionTarget - pos;
      if (toTarget.magSq() > INTERACTION_DISTANCE_EPSILON) {
        toTarget.setMag(FOLLOW_POSITION_BIAS * interactionStrength);
        applyForce(toTarget);
      }
    }
  }

  void applyProfileSteering(MotionProfile profile) {
    switch (profile) {
      case MotionProfile::Chase:
        if (!isNearChaseTarget()) {
          applySeekForce(getChaseTarget(), FOOD_FORCE);
        }
        break;

      case MotionProfile::Approach:
        if (hasInteractionTarget()) {
          applyApproachSteering();
        } else {
          applySeekForce(getDebugSteerTarget(), BODY_ZONE_PULL_FORCE);
        }
        break;

      case MotionProfile::Flee:
        applyFleeForce(getFleeThreatPoint(), PROFILE_FLEE_REPEL_FORCE);
        break;

      case MotionProfile::Friend:
        if (hasInteractionTarget()) {
          applyFriendSteering();
        } else {
          applySeekForce(getDebugSteerTarget(), FOLLOW_POSITION_BIAS);
        }
        break;

      case MotionProfile::Hold:
        if (hasInteractionTarget()) {
          applySeekForce(interactionTarget, BODY_ZONE_PULL_FORCE * 0.15f);
        }
        break;

      case MotionProfile::Alert:
      case MotionProfile::Wander:
      default:
        break;
    }
  }

  void applyProfileSpeedLimits(const MotionProfileSpec& spec, PVector& desiredVel) {
    float effMin = (float)minSpeed * spec.minSpeedFrac;
    float effMax = (float)maxSpeed * spec.maxSpeedFrac;

    if (effMin > 0.0f) {
      float effMinSq = effMin * effMin;
      if (desiredVel.magSq() < effMinSq) {
        desiredVel.setMag(effMin);
      }
    }

    float maxSpeedCO2 = map(co2, CO2_BAD, CO2_REALBAD, effMax, 0);
    if (maxSpeedCO2 < 0) maxSpeedCO2 = 0;
    else if (maxSpeedCO2 > effMax) maxSpeedCO2 = effMax;
    desiredVel.limit(maxSpeedCO2);
  }

  MotionProfile resolveProfile() {
    MotionProfile profile;
#if MOTION_PROFILE_DEBUG_ENABLED
    {
      MotionProfile forced = MotionProfileDebug::getForcedProfile();
      profile = forced;
    }
#else
    if (followingFood) {
      profile = MotionProfile::Chase;
    } else if (shouldFlee()) {
      profile = MotionProfile::Flee;
    } else if (interactionPalmMode) {
      profile = MotionProfile::Chase;
    } else if (interactionState == InteractionState::ALERT) {
      profile = MotionProfile::Alert;
    } else if (hasBodyBlob) {
      if (shouldHoldAtSlot()) {
        profile = MotionProfile::Hold;
      } else {
        profile = MotionProfile::Approach;
      }
    } else {
      profile = MotionProfile::Wander;
    }
#endif

    if (profile == MotionProfile::Chase && !interactionPalmMode && isNearChaseTarget()) {
      profile = MotionProfile::Hold;
    }
    return profile;
  }

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
  MotionProfile getMotionProfile() const { return activeProfile; }

  // Call after updateInteractionState() so steering/separation use this frame's profile.
  void refreshActiveProfile() { activeProfile = resolveProfile(); }

  virtual void doMotion() = 0;

  void updateInteractionState(const InteractionData& interaction,
                              uint16_t physicsWidth, uint16_t physicsHeight,
                              int selfIndex = -1, int schoolCount = 1) {
    unsigned long now = millis();
    (void)schoolCount;

    distanceHint = interaction.distanceHint;
    interactionSlotIndex = selfIndex >= 0 ? selfIndex : 0;
    initHoldEagernessIfNeeded();

    interactionPalmMode = false;
    interactionFlow = PVector(0, 0);
    interactionStrength = 1.0f;
#if AQUARIUM_TOF_PALM_INTERACTION_ENABLED
    if (interaction.hasPalmHold && interaction.palmStrength > 0.01f) {
      interactionPalmMode = true;
      interactionStrength = interaction.palmStrength;
      palmChaseTarget = PVector(constrain(interaction.palmNormX, 0.0f, 1.0f) * physicsWidth,
                                constrain(interaction.palmNormY, 0.0f, 1.0f) * physicsHeight);
      interactionFlow = PVector(interaction.palmVelocityX, interaction.palmVelocityY);
    }
#endif

    hasBodyBlob = interaction.hasBlob;
    if (hasBodyBlob) {
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

      blobLostTime = 0;
      lastBlobPos = interactionCenter;

      uint32_t slotHash = (uint32_t)(interactionSlotIndex + 1) * 2654435761u;
      float slotAngle = (float)(slotHash % 6283u) * 0.001f;
      float radiusJitter = 0.35f + (float)((slotHash >> 16) % 66u) * (0.65f / 65.0f);
      float slotRadius = interactionRadius * BODY_ZONE_SLOT_RADIUS_FRACTION * radiusJitter;
      interactionTarget = interactionCenter +
                          PVector(cosf(slotAngle) * slotRadius, sinf(slotAngle) * slotRadius);
      interactionTarget.x = constrain(interactionTarget.x, 0.0f, (float)physicsWidth);
      interactionTarget.y = constrain(interactionTarget.y, 0.0f, (float)physicsHeight);

      if (interactionState == InteractionState::IDLE) {
        interactionState = InteractionState::ALERT;
        stateStartTime = now;
      } else if (interactionState == InteractionState::ALERT &&
                 now - stateStartTime >= STATE_ALERT_DURATION_MS) {
        interactionState = InteractionState::CURIOUS;
        stateStartTime = now;
      } else if (interactionState != InteractionState::ALERT) {
        interactionState = InteractionState::CURIOUS;
      }
    } else {
      interactionRadius = 0;
      if (blobLostTime == 0) {
        blobLostTime = now;
      }
      if (interactionState != InteractionState::IDLE &&
          now - blobLostTime >= STATE_IDLE_RECOVERY_MS) {
        interactionState = InteractionState::IDLE;
        stateStartTime = now;
        holdUntil = 0;
      }
    }

    if (!hasBodyBlob) {
      fleeLatched = false;
      fleeClearAfterMs = 0;
    }
  }

  void update(float age = AGE_ADULT, long co2 = CO2_OK,
              bool stayInside = false) {
    this->co2 = co2;

    if (age < AGE_EGG) {
      vel = PVector(0, 0);
      return;
    }

    activeProfile = resolveProfile();
    const bool fleeing = activeProfile == MotionProfile::Flee;

    if (!fleeing) {
      if (stayInside || co2 > CO2_BAD) {
        boundaryCheck(BOUNDARY_FORCE * 10);
      } else {
        boundaryCheck(BOUNDARY_FORCE);
      }
    } else {
      outOfBoundary = false;
    }

    MotionProfileSpec spec = getMotionProfileSpec(activeProfile);

    if (spec.damping < 0.999f) {
      vel *= spec.damping;
    }

    if (activeProfile == MotionProfile::Flee) {
      PVector away = pos - getFleeThreatPoint();
      if (away.magSq() <= INTERACTION_DISTANCE_EPSILON) {
        away = PVector::fromAngle(angle + PI);
      } else {
        away.normalize();
      }
      vel += away * PROFILE_FLEE_VEL_BOOST;
    }

    if (!outOfBoundary) {
      applyProfileSteering(activeProfile);
      applyProfileOrganic(spec);
    }

    PVector desiredVel = vel;
    desiredVel += acc;
    applyProfileSpeedLimits(spec, desiredVel);

    vel = desiredVel;
    pos += vel;

    if (fleeing && isOffCanvas()) {
      vel = PVector(0, 0);
      fleeLatched = false;
      fleeClearAfterMs = 0;
    }

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

  void doMotionScaled(float sinScale, float noiseScale) {
    float theta = vel.heading() + PI / 2;
    float a = (millis() * sinFrequency + angleOffset);
    float yOffset = sin(a) * sinAmplitude * sinScale;
    PVector sinusoidalForce = PVector::fromAngle(theta);
    sinusoidalForce *= yOffset;
    applyForce(sinusoidalForce);

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
