#include "Boid.h"
#include <cmath>

Boid boids[AVAILABLE_BOID_COUNT];

Boid::Boid() : enabled(true) {}

Boid::Boid(float x, float y, PVector* limits) : 
    acceleration(0, 0),
    velocity(randomf(), randomf()),
    location(x, y),
    limits(*limits),
    maxspeed(1.5),
    maxforce(0.05),
    desiredseparation(4),
    neighbordist(8),
    colorIndex(0),
    mass(1.0),
    enabled(true) {}

float Boid::randomf() {
    return mapfloat(random(0, 255), 0, 255, -.5, .5);
}

float Boid::mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void Boid::run(Boid boids[], uint8_t boidCount) {
    flock(boids, boidCount);
    update();
    wrapAroundBorders();
}

void Boid::update(float speedMultiplier) {
    velocity += acceleration;
    velocity.limit(maxspeed * speedMultiplier);
    location += velocity;
    acceleration *= 0;
}

void Boid::applyForce(PVector force) {
    PVector f = force / mass;
    acceleration += f;
}

void Boid::repelForce(PVector obstacle, float radius) {
    PVector futureLocation = location + velocity;
    PVector dir = futureLocation - obstacle;
    float d = dir.mag();
    
    if (d <= radius) {
        dir.normalize();
        dir *= (maxspeed * (radius - d) / radius);
        PVector steer = dir - velocity;
        steer.limit(maxforce);
        applyForce(steer);
    }
}

void Boid::flock(Boid boids[], uint8_t boidCount) {
    PVector sep = separate(boids, boidCount);
    PVector ali = align(boids, boidCount);
    PVector coh = cohesion(boids, boidCount);
    
    sep *= 1.5;
    ali *= 1.0;
    coh *= 1.0;
    
    applyForce(sep);
    applyForce(ali);
    applyForce(coh);
}

PVector Boid::separate(Boid boids[], uint8_t boidCount) {
    PVector steer(0, 0);
    int count = 0;
    
    for (int i = 0; i < boidCount; i++) {
        if (!boids[i].enabled) continue;
        float d = location.dist(boids[i].location);
        if ((d > 0) && (d < desiredseparation)) {
            PVector diff = location - boids[i].location;
            diff.normalize();
            diff /= d;
            steer += diff;
            count++;
        }
    }
    
    if (count > 0) {
        steer /= (float)count;
    }
    
    if (steer.mag() > 0) {
        steer.normalize();
        steer *= maxspeed;
        steer -= velocity;
        steer.limit(maxforce);
    }
    return steer;
}

PVector Boid::align(Boid boids[], uint8_t boidCount) {
    PVector sum(0, 0);
    int count = 0;
    for (int i = 0; i < boidCount; i++) {
        if (!boids[i].enabled) continue;
        float d = location.dist(boids[i].location);
        if ((d > 0) && (d < neighbordist)) {
            sum += boids[i].velocity;
            count++;
        }
    }
    if (count > 0) {
        sum /= (float)count;
        sum.normalize();
        sum *= maxspeed;
        PVector steer = sum - velocity;
        steer.limit(maxforce);
        return steer;
    } else {
        return PVector(0, 0);
    }
}

PVector Boid::cohesion(Boid boids[], uint8_t boidCount) {
    PVector sum(0, 0);
    int count = 0;
    for (int i = 0; i < boidCount; i++) {
        if (!boids[i].enabled) continue;
        float d = location.dist(boids[i].location);
        if ((d > 0) && (d < neighbordist)) {
            sum += boids[i].location;
            count++;
        }
    }
    if (count > 0) {
        sum /= count;
        return seek(sum);
    } else {
        return PVector(0, 0);
    }
}

PVector Boid::seek(PVector target) {
    PVector desired = target - location;
    desired.normalize();
    desired *= maxspeed;
    PVector steer = desired - velocity;
    steer.limit(maxforce);
    return steer;
}

void Boid::arrive(PVector target) {
    PVector desired = target - location;
    float d = desired.mag();
    desired.normalize();
    if (d < 100) {
        float m = mapfloat(d, 0, 100, 0, maxspeed);
        desired *= m;
    } else {
        desired *= maxspeed;
    }
    PVector steer = desired - velocity;
    steer.limit(maxforce);
    applyForce(steer);
}

void Boid::wrapAroundBorders() {
    if (location.x < 0) location.x = limits.x - 1;
    if (location.y < 0) location.y = limits.y - 1;
    if (location.x >= limits.x) location.x = 0;
    if (location.y >= limits.y) location.y = 0;
}

void Boid::avoidBorders() {
    PVector desired(velocity.x, velocity.y);

    if (location.x < 8) desired.x = maxspeed;
    if (location.x > limits.x - 8) desired.x = -maxspeed;
    if (location.y < 8) desired.y = maxspeed;
    if (location.y > limits.y - 8) desired.y = -maxspeed;

    if (desired != velocity) {
        PVector steer = desired - velocity;
        steer.limit(maxforce);
        applyForce(steer);
    }
}

bool Boid::bounceOffBorders(float bounce) {
    bool bounced = false;
    
    if (location.x < 0) {
        location.x = 0;
        velocity.x *= -bounce;
        bounced = true;
    } else if (location.x >= limits.x) {
        location.x = limits.x - 1;
        velocity.x *= -bounce;
        bounced = true;
    }

    if (location.y < 0) {
        location.y = 0;
        velocity.y *= -bounce;
        bounced = true;
    } else if (location.y >= limits.y) {
        location.y = limits.y - 1;
        velocity.y *= -bounce;
        bounced = true;
    }

    return bounced;
}

void Boid::render() {
    // Implementation depends on your rendering system
    // For example:
    // drawPixel(location.x, location.y, colorIndex);
}