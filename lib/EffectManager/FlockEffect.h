#pragma once

#include "Effect.h"
#include "Boid.h"

class FlockEffect : public Effect {
private:
    static constexpr int BOID_COUNT = 10;
    Boid boids[BOID_COUNT];
    Boid predator;
    PVector wind;
    uint8_t hue = 0;
    bool predatorPresent = true;

    unsigned long lastUpdateHueMs = 0;
    unsigned long lastUpdatePredatorMs = 0;

    void initializeBoids();
    void updateBoids();
    void updatePredator();
    void applyWind();

public:
    FlockEffect(Matrix* m);

    void update() override;
    const char* getName() const override;
    void reset();
};