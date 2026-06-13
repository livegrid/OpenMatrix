#include "BoidManager.h"
#include "../TOFSensor/TOFInteractionManager.h"
#include "Motion/MotionProfile.h"

#include <random>

namespace {

struct BoidInteractionZone {
  bool hasBody = false;
  bool palmActive = false;
  PVector bodyCenter;
  PVector palmCenter;
  PVector flow;
  float radius = 0;
  float palmStrength = 0;
};

BoidInteractionZone buildInteractionZone(const InteractionData* interaction, const PVector& limits) {
  BoidInteractionZone zone;
  if (!interaction) return zone;

#if AQUARIUM_TOF_PALM_INTERACTION_ENABLED
  if (interaction->hasPalmHold && interaction->palmStrength > 0.01f) {
    zone.palmActive = true;
    zone.palmCenter = PVector(constrain(interaction->palmNormX, 0.0f, 1.0f) * (limits.x - 1.0f),
                              constrain(interaction->palmNormY, 0.0f, 1.0f) * (limits.y - 1.0f));
    zone.flow = PVector(interaction->palmVelocityX, interaction->palmVelocityY);
    zone.palmStrength = interaction->palmStrength;
  }
#endif

  if (!interaction->hasBlob) return zone;

  int minX = 8, minY = 8, maxX = -1, maxY = -1;
  int activeCount = 0;
  for (uint8_t y = 0; y < 8; y++) {
    for (uint8_t x = 0; x < 8; x++) {
      int16_t depth = interaction->depthMap[y][x];
      if (depth > TOF_MIN_DETECTION_DIST && depth < TOF_MAX_DETECTION_DIST) {
        if ((int)x < minX) minX = x;
        if ((int)y < minY) minY = y;
        if ((int)x > maxX) maxX = x;
        if ((int)y > maxY) maxY = y;
        activeCount++;
      }
    }
  }

  zone.hasBody = true;
  if (activeCount > 0) {
    float centerNormX = ((float)minX + (float)maxX + 1.0f) * 0.5f / 8.0f;
    float centerNormY = ((float)minY + (float)maxY + 1.0f) * 0.5f / 8.0f;
    zone.bodyCenter = PVector(centerNormX * (limits.x - 1.0f), centerNormY * (limits.y - 1.0f));

    float halfW = ((float)(maxX - minX + 1) / 8.0f) * limits.x * 0.5f;
    float halfH = ((float)(maxY - minY + 1) / 8.0f) * limits.y * 0.5f;
    zone.radius = max(halfW, halfH) * (1.0f + BODY_ZONE_PADDING_FRACTION);
    zone.radius = max(zone.radius, min(limits.x, limits.y) * 0.18f);
  } else {
    zone.bodyCenter = PVector(constrain(interaction->blobX, 0.0f, 1.0f) * (limits.x - 1.0f),
                              constrain(interaction->blobY, 0.0f, 1.0f) * (limits.y - 1.0f));
    zone.radius = max(min(limits.x, limits.y) * 0.25f, 12.0f);
  }
  return zone;
}

PVector getScreenSteerCenter(const PVector& limits) {
#if MOTION_PROFILE_DEBUG_ENABLED
  return PVector(MotionProfileDebug::getTargetScreenX((uint16_t)limits.x),
                 MotionProfileDebug::getTargetScreenY((uint16_t)limits.y));
#else
  return PVector(limits.x * 0.5f, limits.y * 0.5f);
#endif
}

PVector getBoidChaseTarget(const PVector& center, int slotIndex) {
  uint32_t slotHash = (uint32_t)(slotIndex + 1) * 2654435761u;
  float slotAngle = (float)(slotHash % 6283u) * 0.001f;
  return center + PVector(cosf(slotAngle) * BOID_CHASE_SLOT_SPREAD,
                          sinf(slotAngle) * BOID_CHASE_SLOT_SPREAD);
}

MotionProfile resolveBoidBaseProfile(const InteractionData* interaction) {
#if MOTION_PROFILE_DEBUG_ENABLED
  return MotionProfileDebug::getForcedProfile();
#else
  static bool hadBodyBlob = false;
  static unsigned long alertUntilMs = 0;

  if (!interaction) return MotionProfile::Wander;

  bool hasBody = interaction->hasBlob;
  if (!hadBodyBlob && hasBody) {
    alertUntilMs = millis() + STATE_ALERT_DURATION_MS;
  }
  hadBodyBlob = hasBody;

  static bool fleeLatched = false;
  static unsigned long fleeClearAfterMs = 0;
  unsigned long now = millis();

  if (!hasBody && interaction->distanceHint == TofDistanceHint::NoPerson) {
    fleeLatched = false;
    fleeClearAfterMs = 0;
    return MotionProfile::Wander;
  }

  if (hasBody && interaction->distanceHint == TofDistanceHint::TooClose) {
    fleeLatched = true;
    fleeClearAfterMs = 0;
    return MotionProfile::Flee;
  }
  if (fleeLatched) {
    if (hasBody && interaction->distanceHint == TofDistanceHint::Ok) {
      if (fleeClearAfterMs == 0) fleeClearAfterMs = now + TOF_FLEE_CLEAR_MS;
      if (now < fleeClearAfterMs) return MotionProfile::Flee;
      fleeLatched = false;
      fleeClearAfterMs = 0;
    } else {
      fleeClearAfterMs = 0;
      return MotionProfile::Flee;
    }
  }
#if AQUARIUM_TOF_PALM_INTERACTION_ENABLED
  if (interaction->hasPalmHold && interaction->palmStrength > 0.01f) {
    return MotionProfile::Chase;
  }
#endif
  if (millis() < alertUntilMs) {
    return MotionProfile::Alert;
  }
  if (hasBody) {
    return MotionProfile::Approach;
  }
  return MotionProfile::Wander;
#endif
}

float boidHoldEagerness(int slotIndex) {
  uint32_t h = (uint32_t)(slotIndex + 1) * 2654435761u;
  return 0.35f + (float)((h >> 8) % 65u) * (0.65f / 65.0f);
}

bool shouldBoidHold(int slotIndex, const Boid& boid, const BoidInteractionZone& zone,
                    unsigned long& holdUntil) {
  if (!zone.hasBody) return false;

  PVector slotTarget = getBoidChaseTarget(zone.bodyCenter, slotIndex);
  float holdRadius = zone.radius * PROFILE_HOLD_SLOT_RADIUS_FRAC * boidHoldEagerness(slotIndex);
  float distToSlot = boid.location.dist(slotTarget);
  float distToCenter = boid.location.dist(zone.bodyCenter);
  unsigned long now = millis();

  if (distToCenter > zone.radius || distToSlot >= holdRadius) {
    holdUntil = 0;
    return false;
  }

  if (holdUntil == 0 && distToSlot < holdRadius * 0.85f) {
    uint32_t h = (uint32_t)(slotIndex + 1) * 1597334677u;
    unsigned long dwell = PROFILE_HOLD_MIN_MS +
                        (unsigned long)((h >> 12) % (PROFILE_HOLD_MAX_MS - PROFILE_HOLD_MIN_MS + 1));
    holdUntil = now + dwell;
  }

  return holdUntil > 0 && now < holdUntil;
}

bool isBoidOffCanvas(const Boid& boid) {
  return boid.location.x < 0.0f || boid.location.y < 0.0f ||
         boid.location.x >= boid.limits.x || boid.location.y >= boid.limits.y;
}

void flockWeighted(Boid& boid, Boid* group, uint8_t count, float sepW, float aliW, float cohW) {
  PVector sep = boid.separate(group, count) * sepW;
  PVector ali = boid.align(group, count) * aliW;
  PVector coh = boid.cohesion(group, count) * cohW;
  boid.applyForce(sep);
  boid.applyForce(ali);
  boid.applyForce(coh);
}

void applyBoidProfile(Boid& boid, MotionProfile profile, const PVector& steerCenter,
                      const BoidInteractionZone& zone, int slotIndex, float co2SpeedMult,
                      Boid* group, uint8_t groupCount) {
  MotionProfileSpec spec = getMotionProfileSpec(profile);
  float speedMult = co2SpeedMult * spec.maxSpeedFrac;
  float sepW = 1.5f;
  float aliW = 1.0f;
  float cohW = 1.0f;

  if (spec.damping < 0.999f) {
    boid.velocity *= spec.damping;
  }

  PVector chaseTarget = getBoidChaseTarget(steerCenter, slotIndex);
  if (zone.palmActive && profile == MotionProfile::Chase) {
    chaseTarget = zone.palmCenter;
  } else if (zone.hasBody && profile != MotionProfile::Flee) {
    chaseTarget = getBoidChaseTarget(zone.bodyCenter, slotIndex);
  }

  switch (profile) {
    case MotionProfile::Hold:
      sepW = 1.2f;
      aliW = 0.35f;
      cohW = 0.15f;
      boid.arrive(chaseTarget);
      break;

    case MotionProfile::Alert:
      sepW = 0.6f;
      aliW = 0.25f;
      cohW = 0.15f;
      break;

    case MotionProfile::Approach:
      sepW = 1.4f;
      aliW = 0.8f;
      cohW = 0.4f;
      if (boid.location.dist(chaseTarget) > BOID_CHASE_ARRIVE_DIST) {
        boid.applyForce(boid.seek(chaseTarget) * BOID_CHASE_SEEK_WEIGHT * 0.55f);
      } else {
        boid.arrive(chaseTarget);
      }
      break;

    case MotionProfile::Chase:
      sepW = 2.2f;
      aliW = 0.5f;
      cohW = 0.1f;
      if (boid.location.dist(chaseTarget) > BOID_CHASE_ARRIVE_DIST) {
        boid.applyForce(boid.seek(chaseTarget) * BOID_CHASE_SEEK_WEIGHT);
      } else if (!zone.palmActive) {
        profile = MotionProfile::Hold;
        spec = getMotionProfileSpec(profile);
        speedMult = co2SpeedMult * spec.maxSpeedFrac;
        boid.arrive(chaseTarget);
      }
      break;

    case MotionProfile::Flee: {
      sepW = 0.2f;
      aliW = 0.0f;
      cohW = 0.0f;
      PVector threat = zone.hasBody ? zone.bodyCenter : steerCenter;
      PVector away = boid.location - threat;
      if (away.magSq() < 1.0f) {
        away = PVector(Boid::randomf(), Boid::randomf());
      }
      away.normalize();
      away *= BOID_FLEE_FORCE;
      boid.applyForce(away);
      boid.velocity += away * 0.15f;
      speedMult = co2SpeedMult * BOID_FLEE_SPEED_MULT;
      break;
    }

    case MotionProfile::Wander:
    default:
      break;
  }

  if (profile != MotionProfile::Flee) {
    flockWeighted(boid, group, groupCount, sepW, aliW, cohW);
  }

  float effMin = boid.maxspeed * spec.minSpeedFrac * co2SpeedMult;
  boid.velocity += boid.acceleration;
  if (effMin > 0.0f && boid.velocity.magSq() < effMin * effMin) {
    boid.velocity.setMag(effMin);
  }
  boid.velocity.limit(boid.maxspeed * speedMult);
  boid.location += boid.velocity;
  if (profile == MotionProfile::Flee && isBoidOffCanvas(boid)) {
    boid.velocity = PVector(0, 0);
  }
  boid.acceleration *= 0;
}

}  // namespace

BoidManager::BoidManager(Matrix* m)
    : matrix(m), limits(m->getXResolution(), m->getYResolution()) {}

void BoidManager::initializeBoids() {
  boidGroups.resize(BOID_GROUPS);

  for (int group = 0; group < BOID_GROUPS; group++) {
    int numBoids = random(NUM_BOIDS);
    for (int i = 0; i < numBoids; i++) {
      boidGroups[group].emplace_back(random(0, matrix->getXResolution()),
                                     random(0, matrix->getYResolution()),
                                     &limits);
      boidGroups[group][i].maxspeed = random(BOID_MAX_SPEED) / 10.0f;
      boidGroups[group][i].maxforce = random(BOID_MAX_FORCE) / 10.0f;
    }
  }
}

void BoidManager::updateBoids(long co2, const InteractionData* interaction) {
  float speedMultiplier = map(co2, CO2_BAD, CO2_REALBAD, 100.0f, 0.0f);
  speedMultiplier = constrain(speedMultiplier, 0.0f, 100.0f);
  speedMultiplier /= 100.0f;

  BoidInteractionZone zone = buildInteractionZone(interaction, limits);
  MotionProfile baseProfile = resolveBoidBaseProfile(interaction);
  PVector steerCenter = getScreenSteerCenter(limits);
  int globalSlot = 0;

  static unsigned long boidHoldUntil[96] = {};

  for (auto& group : boidGroups) {
    uint8_t groupCount = (uint8_t)group.size();
    for (auto& boid : group) {
      int holdIdx = globalSlot % 96;
      MotionProfile profile = baseProfile;

      if (profile == MotionProfile::Approach &&
          shouldBoidHold(globalSlot, boid, zone, boidHoldUntil[holdIdx])) {
        profile = MotionProfile::Hold;
      }

      if (profile == MotionProfile::Flee && isBoidOffCanvas(boid)) {
        profile = MotionProfile::Wander;
      }

      applyBoidProfile(boid, profile, steerCenter, zone, globalSlot, speedMultiplier,
                       group.data(), groupCount);

      if (profile != MotionProfile::Flee) {
        boid.avoidBorders();
      }
      globalSlot++;
    }
  }
}

void BoidManager::renderBoids() {
  for (const auto& group : boidGroups) {
    for (const auto& boid : group) {
      float velMagSq = boid.velocity.x * boid.velocity.x + boid.velocity.y * boid.velocity.y;
      if (velMagSq > 0.0001f) {
        float invMag = 1.0f / sqrt(velMagSq);
        int x2 = boid.location.x + boid.velocity.x * invMag;
        int y2 = boid.location.y + boid.velocity.y * invMag;
        matrix->foreground->drawLine(boid.location.x, boid.location.y, x2, y2, CRGB(50, 200, 100));
      } else {
        matrix->foreground->drawPixel(boid.location.x, boid.location.y, CRGB(50, 200, 100));
      }
    }
  }
}
