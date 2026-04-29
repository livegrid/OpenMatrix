#include "BoidManager.h"
#include "../TOFSensor/TOFInteractionManager.h"

#include <random>

namespace {

struct BoidInteractionZone {
  bool valid = false;
  bool palmMode = false;
  PVector center;
  PVector flow;
  float radius = 0;
  float strength = 0;
};

BoidInteractionZone buildInteractionZone(const InteractionData* interaction, const PVector& limits) {
  BoidInteractionZone zone;
  if (!interaction) return zone;

  if (interaction->hasPalmHold && interaction->palmStrength > 0.01f) {
    zone.valid = true;
    zone.palmMode = true;
    zone.center = PVector(constrain(interaction->palmNormX, 0.0f, 1.0f) * (limits.x - 1.0f),
                          constrain(interaction->palmNormY, 0.0f, 1.0f) * (limits.y - 1.0f));
    zone.flow = PVector(interaction->palmVelocityX, interaction->palmVelocityY);
    zone.radius = max(min(limits.x, limits.y) * PALM_ZONE_RADIUS_FRACTION, 12.0f);
    zone.strength = interaction->palmStrength;
    return zone;
  }

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

  zone.valid = true;
  zone.strength = 1.0f;
  if (activeCount > 0) {
    float centerNormX = ((float)minX + (float)maxX + 1.0f) * 0.5f / 8.0f;
    float centerNormY = ((float)minY + (float)maxY + 1.0f) * 0.5f / 8.0f;
    zone.center = PVector(centerNormX * (limits.x - 1.0f), centerNormY * (limits.y - 1.0f));

    float halfW = ((float)(maxX - minX + 1) / 8.0f) * limits.x * 0.5f;
    float halfH = ((float)(maxY - minY + 1) / 8.0f) * limits.y * 0.5f;
    zone.radius = max(halfW, halfH) * (1.0f + BODY_ZONE_PADDING_FRACTION);
    zone.radius = max(zone.radius, min(limits.x, limits.y) * 0.18f);
  } else {
    zone.center = PVector(constrain(interaction->blobX, 0.0f, 1.0f) * (limits.x - 1.0f),
                          constrain(interaction->blobY, 0.0f, 1.0f) * (limits.y - 1.0f));
    zone.radius = max(min(limits.x, limits.y) * 0.25f, 12.0f);
  }
  return zone;
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

  for (auto& group : boidGroups) {
    for (auto& boid : group) {
      if (zone.valid) {
        if (zone.palmMode) {
          float flowMag = zone.flow.mag();
          if (flowMag > FOLLOW_DIRECTION_MIN_VELOCITY) {
            PVector flowForce = zone.flow / flowMag;
            float flowScale = constrain(flowMag, 0.25f, 1.0f);
            boid.applyForce(flowForce * BOID_TOF_ATTRACTION_FORCE * flowScale * zone.strength);
          }
        }

        float distanceToZone = boid.location.dist(zone.center);
        bool outsideZone = distanceToZone > zone.radius;
        bool palmNeedsBias = zone.palmMode && distanceToZone > zone.radius * 0.65f;
        if (distanceToZone > INTERACTION_DISTANCE_EPSILON &&
            (outsideZone || palmNeedsBias)) {
          float attractionWeight = BOID_TOF_ATTRACTION_FORCE * zone.strength;
          if (outsideZone) {
            float outside = min((distanceToZone - zone.radius) / max(zone.radius, 1.0f), 1.0f);
            attractionWeight *= 0.4f + 0.6f * outside;
          } else {
            attractionWeight *= FOLLOW_POSITION_BIAS;
          }
          boid.applyForce(boid.seek(zone.center) * attractionWeight);
        }
      }
      boid.run(group.data(), group.size(),speedMultiplier);
      boid.avoidBorders();
    }
  }
}

void BoidManager::renderBoids() {
  for (const auto& group : boidGroups) {
    for (const auto& boid : group) {
      // Use normalized velocity directly instead of expensive atan2/cos/sin
      float velMagSq = boid.velocity.x * boid.velocity.x + boid.velocity.y * boid.velocity.y;
      if (velMagSq > 0.0001f) {  // Avoid division by zero
        float invMag = 1.0f / sqrt(velMagSq);
        int x2 = boid.location.x + boid.velocity.x * invMag;
        int y2 = boid.location.y + boid.velocity.y * invMag;
        matrix->foreground->drawLine(boid.location.x, boid.location.y, x2, y2, CRGB(50, 200, 100));
      } else {
        // Stationary boid - just draw a pixel
        matrix->foreground->drawPixel(boid.location.x, boid.location.y, CRGB(50, 200, 100));
      }
    }
  }
}