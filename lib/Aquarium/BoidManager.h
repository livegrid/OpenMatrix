#pragma once

#include <vector>
#include "Boid.h"
#include "Matrix.h"
#include "AquariumSettings.h"
#include <SCD40Settings.h>

class BoidManager {
private:
    std::vector<std::vector<Boid>> boidGroups;
    Matrix* matrix;
    PVector limits;
    // 1 = healthy green, 0 = white; uses the same CO2 health rates as Fish
    float health = 1.0f;

public:
    BoidManager(Matrix* m);
    void initializeBoids();
    void updateBoids(long co2 = 600);
    void applyAttractor(float x, float y, float strength = 0.15f);
    void renderBoids();
};