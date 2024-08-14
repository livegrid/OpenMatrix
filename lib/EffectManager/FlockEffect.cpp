#include "FlockEffect.h"

FlockEffect::FlockEffect(Matrix* m) : Effect(m) {
    reset();
}

void FlockEffect::update() {
    m_matrix->background->dim(230);

    updateBoids();
    updatePredator();
    applyWind();

    // Update hue
    if (millis() - lastUpdateHueMs > 200) {
        lastUpdateHueMs = millis();
        hue++;
    }

    // Toggle predator presence
    if (millis() - lastUpdatePredatorMs > 30000) {
        lastUpdatePredatorMs = millis();
        predatorPresent = !predatorPresent;
    }
}

const char* FlockEffect::getName() const {
    return "Flock";
}

void FlockEffect::reset() {
    initializeBoids();
    predatorPresent = random(2) >= 1;
    if (predatorPresent) {
        predator = Boid(VIRTUAL_RES_X / 2, VIRTUAL_RES_Y / 2);
        predator.maxspeed = 0.385;
        predator.maxforce = 0.020;
        predator.neighbordist = 16.0;
        predator.desiredseparation = 0.0;
    }
}

void FlockEffect::initializeBoids() {
    for (int i = 0; i < BOID_COUNT; i++) {
        boids[i] = Boid(random(VIRTUAL_RES_X), random(VIRTUAL_RES_Y));
        boids[i].maxspeed = 0.380;
        boids[i].maxforce = 0.015;
    }
}

void FlockEffect::updateBoids() {
    for (int i = 0; i < BOID_COUNT; i++) {
        Boid* boid = &boids[i];

        if (predatorPresent) {
            boid->repelForce(predator.location, 10);
        }

        boid->run(boids, BOID_COUNT);
        boid->wrapAroundBorders();
        PVector location = boid->location;
        m_matrix->background->drawPixel(location.x, location.y, baseColor);
    }
}

void FlockEffect::updatePredator() {
    if (predatorPresent) {
        predator.run(boids, BOID_COUNT);
        predator.wrapAroundBorders();
        CRGB color = CRGB(baseColor.g, baseColor.r, baseColor.b);
        PVector location = predator.location;
        m_matrix->background->drawPixel(location.x, location.y, color);
    }
}

void FlockEffect::applyWind() {
    if (random(256) > 250) {
        wind.x = Boid::randomf() * 0.015;
        wind.y = Boid::randomf() * 0.015;
        for (int i = 0; i < BOID_COUNT; i++) {
            boids[i].applyForce(wind);
        }
    }
}