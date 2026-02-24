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
    void renderBoids();
};