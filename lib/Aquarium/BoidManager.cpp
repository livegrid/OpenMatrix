#include "BoidManager.h"

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

void BoidManager::updateBoids(long co2) {
  float speedMultiplier = map(co2, CO2_BAD, CO2_REALBAD, 100.0f, 0.0f);
  speedMultiplier = constrain(speedMultiplier, 0.0f, 100.0f);
  speedMultiplier /= 100.0f;
  for (auto& group : boidGroups) {
    for (auto& boid : group) {
      boid.run(group.data(), group.size(),speedMultiplier);
      boid.wrapAroundBorders();
    }
  }
}

void BoidManager::renderBoids() {
  for (const auto& group : boidGroups) {
    for (const auto& boid : group) {
      matrix->foreground->setPixel(boid.location.x, boid.location.y, 255, 255,
                                   255);
    }
  }
}