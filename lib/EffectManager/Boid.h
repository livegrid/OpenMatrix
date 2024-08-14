#pragma once

#include "MatrixSettings.h"
#include "Vector.h"

class Boid {
public:
    PVector location;
    PVector velocity;
    PVector acceleration;
    float maxforce;
    float maxspeed;
    float desiredseparation;
    float neighbordist;
    byte colorIndex;
    float mass;
    boolean enabled;

    Boid();
    Boid(float x, float y);

    static float randomf();
    static float mapfloat(float x, float in_min, float in_max, float out_min, float out_max);

    void run(Boid boids[], uint8_t boidCount);
    void update();
    void applyForce(PVector force);
    void repelForce(PVector obstacle, float radius);
    void flock(Boid boids[], uint8_t boidCount);
    PVector separate(Boid boids[], uint8_t boidCount);
    PVector align(Boid boids[], uint8_t boidCount);
    PVector cohesion(Boid boids[], uint8_t boidCount);
    PVector seek(PVector target);
    void arrive(PVector target);
    void wrapAroundBorders();
    void avoidBorders();
    bool bounceOffBorders(float bounce);
    void render();
};

static const uint8_t AVAILABLE_BOID_COUNT = 40;
extern Boid boids[AVAILABLE_BOID_COUNT];