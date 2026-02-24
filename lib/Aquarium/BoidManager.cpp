#include "BoidManager.h"
#include "../TOFSensor/TOFInteractionManager.h"

#include <random>

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

  const bool hasAttractor = interaction && interaction->hasBlob;
  PVector attractorPos;
  float attractionRadius = min(limits.x, limits.y) * BOID_TOF_ATTRACTION_RADIUS_FRACTION;
  if (hasAttractor) {
    attractorPos.x = constrain(interaction->blobX, 0.0f, 1.0f) * (limits.x - 1.0f);
    attractorPos.y = constrain(interaction->blobY, 0.0f, 1.0f) * (limits.y - 1.0f);
  }

  for (auto& group : boidGroups) {
    for (auto& boid : group) {
      if (hasAttractor) {
        float distanceToBlob = boid.location.dist(attractorPos);
        if (distanceToBlob > INTERACTION_DISTANCE_EPSILON &&
            distanceToBlob <= attractionRadius) {
          // Blend falloff so boids close to blob react more strongly.
          float proximity = 1.0f - (distanceToBlob / attractionRadius);
          float attractionWeight = BOID_TOF_ATTRACTION_FORCE * (0.2f + 0.8f * proximity);
          boid.applyForce(boid.seek(attractorPos) * attractionWeight);
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