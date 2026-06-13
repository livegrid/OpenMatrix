#ifndef MOTION_PROFILE_H
#define MOTION_PROFILE_H

#include <AquariumSettings.h>
#include <Arduino.h>

enum class OrganicMotion : uint8_t { Off, Low, Full };

struct MotionProfileSpec {
  OrganicMotion organic;
  float sinScale;
  float noiseScale;
  float minSpeedFrac;  // 0 = may stop; 1 = species minSpeed
  float maxSpeedFrac;  // multiplier on species maxSpeed
  float damping;       // per-frame velocity multiplier (1 = none)
};

enum class MotionProfile : uint8_t {
  Wander = 0,
  Hold,
  Alert,
  Approach,
  Chase,
  Flee,
  Friend,
  Count
};

inline const char* motionProfileName(MotionProfile profile) {
  switch (profile) {
    case MotionProfile::Wander: return "Wander";
    case MotionProfile::Hold: return "Hold";
    case MotionProfile::Alert: return "Alert";
    case MotionProfile::Approach: return "Approach";
    case MotionProfile::Chase: return "Chase";
    case MotionProfile::Flee: return "Flee";
    case MotionProfile::Friend: return "Friend";
    default: return "Unknown";
  }
}

inline MotionProfileSpec getMotionProfileSpec(MotionProfile profile) {
  switch (profile) {
    case MotionProfile::Hold:
      return {OrganicMotion::Low, PROFILE_HOLD_SIN_SCALE, PROFILE_HOLD_NOISE_SCALE,
              PROFILE_HOLD_MIN_SPEED_FRAC, PROFILE_HOLD_MAX_SPEED_FRAC,
              PROFILE_HOLD_DAMPING};
    case MotionProfile::Alert:
      return {OrganicMotion::Off, 0.0f, 0.0f, 0.0f, PROFILE_ALERT_MAX_SPEED_FRAC,
              PROFILE_ALERT_DAMPING};
    case MotionProfile::Approach:
      return {OrganicMotion::Low, PROFILE_APPROACH_SIN_SCALE, PROFILE_APPROACH_NOISE_SCALE,
              PROFILE_APPROACH_MIN_SPEED_FRAC, PROFILE_APPROACH_MAX_SPEED_FRAC, 1.0f};
    case MotionProfile::Chase:
      return {OrganicMotion::Off, 0.0f, 0.0f, PROFILE_CHASE_MIN_SPEED_FRAC,
              PROFILE_CHASE_MAX_SPEED_FRAC, 1.0f};
    case MotionProfile::Flee:
      return {OrganicMotion::Off, 0.0f, 0.0f, PROFILE_FLEE_MIN_SPEED_FRAC,
              PROFILE_FLEE_MAX_SPEED_FRAC, PROFILE_FLEE_DAMPING};
    case MotionProfile::Friend:
      return {OrganicMotion::Low, PROFILE_FRIEND_SIN_SCALE, PROFILE_FRIEND_NOISE_SCALE,
              PROFILE_FRIEND_MIN_SPEED_FRAC, PROFILE_FRIEND_MAX_SPEED_FRAC, 1.0f};
    case MotionProfile::Wander:
    default:
      return {OrganicMotion::Full, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
  }
}

#if MOTION_PROFILE_DEBUG_ENABLED

class MotionProfileDebug {
 public:
  static void tick(uint16_t physicsWidth, uint16_t physicsHeight) {
    unsigned long now = millis();
    if (physicsWidth == 0) physicsWidth = 1;
    if (physicsHeight == 0) physicsHeight = 1;

    if (lastCycleMs == 0) {
      lastCycleMs = now;
      targetX = physicsWidth * 0.5f;
      targetY = physicsHeight * 0.5f;
      log_i("MotionProfile debug: cycling every %lu ms (start=%s)",
            (unsigned long)MOTION_PROFILE_DEBUG_CYCLE_MS, motionProfileName(forcedProfile));
      return;
    }

    if ((uint32_t)(now - lastCycleMs) < MOTION_PROFILE_DEBUG_CYCLE_MS) return;

    lastCycleMs = now;
    uint8_t next = ((uint8_t)forcedProfile + 1u) % (uint8_t)MotionProfile::Count;
    forcedProfile = (MotionProfile)next;
    log_i("MotionProfile debug -> %s", motionProfileName(forcedProfile));
  }

  static MotionProfile getForcedProfile() { return forcedProfile; }

  static float getTargetX() { return targetX; }
  static float getTargetY() { return targetY; }

  // Screen-space center (boids use pixel coords, fish use physics coords).
  static float getTargetScreenX(uint16_t screenWidth) {
    return screenWidth > 0 ? targetX / (float)PHYSICS_SCALE : 0.0f;
  }
  static float getTargetScreenY(uint16_t screenHeight) {
    return screenHeight > 0 ? targetY / (float)PHYSICS_SCALE : 0.0f;
  }

 private:
  static inline MotionProfile forcedProfile = MotionProfile::Wander;
  static inline unsigned long lastCycleMs = 0;
  static inline float targetX = 0.0f;
  static inline float targetY = 0.0f;
};

#endif  // MOTION_PROFILE_DEBUG_ENABLED

#endif  // MOTION_PROFILE_H
