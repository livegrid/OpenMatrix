#pragma once

#include <vector>
#include "Boid.h"
#include "Matrix.h"
#include "AquariumSettings.h"
#include <SCD40Settings.h>

struct InteractionData;

class BoidManager {
private:
    std::vector<std::vector<Boid>> boidGroups;
    Matrix* matrix;
    PVector limits;

public:
    BoidManager(Matrix* m);
    void initializeBoids();
    void updateBoids(long co2 = 600, const InteractionData* interaction = nullptr);
    // Ring performance: boids hug evenly-spaced slots on per-group rings around `center`.
    void updateBoidsRing(long co2, PVector center, float baseRadius, float ringPhase,
                         const InteractionData* interaction = nullptr);
    // Fountain performance: rise when active; respawn below canvas after exiting the top.
    void updateBoidsFountain(long co2, PVector center, float width, float height, bool active);
    void renderBoids();
};