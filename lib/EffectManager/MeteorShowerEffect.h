#pragma once

#include "Effect.h"
#include "PVector.h"
#include <Arduino.h>

// Forward declaration for optional TOF sensor integration
class TOFSensor;

// Meteor particle class - similar to p5.js Boid
class Meteor {
public:
    PVector position;
    PVector velocity;
    PVector acceleration;
    float speedMultiplier;
    float size;
    float noiseOffset;
    
    // Color (HSV-like)
    uint8_t hue;
    uint8_t saturation;
    uint8_t brightness;
    
    Meteor();
    void init(float x, float y, float targetSpeed);
    void resetForces();
    void applyBaseFlow(float targetSpeed, float flowCorrectionStrength, float forwardAcceleration);
    void applyWobble(uint32_t time, float wobbleStrength, float wobbleSpeed);
    void applySeparation(Meteor* meteors, uint8_t count, uint8_t selfIndex, float separationDistance, float separationStrength);
    void applyAttractor(PVector& attractorPos, float strength, float radius);
    void applyAttractorY(PVector& attractorPos, float strength, float radius);
    void update(float targetSpeed, float maxVerticalSpeed);
    bool isOffScreen();
    
    static float randomFloat();
    static float noise(float x); // Simple noise function
};

// ToF-based attractor point
struct TOFAttractor {
    PVector position;
    bool active;
};

// Background planet decoration
class Planet {
public:
    PVector pos;
    PVector vel;
    float size;
    uint8_t hue;
    bool hasRings;
    float ringRotation;
    
    Planet();
    void spawn(uint8_t maxWidth, uint8_t maxHeight);
    void update();
    bool isOffScreen(uint8_t maxWidth, uint8_t maxHeight);
};

class MeteorShowerEffect : public Effect {
private:
    // Constants
    static constexpr uint8_t MAX_METEORS = 60;
    static constexpr uint8_t MAX_PLANETS = 3;
    static constexpr uint8_t TOF_GRID_SIZE = 8;
    
    // Meteor system
    Meteor meteors[MAX_METEORS];
    uint8_t meteorCount;
    uint8_t spawnCounter;
    
    // Meteor behavior parameters
    float baseMeteorSpeed;
    float boostedMeteorSpeed;
    float currentMeteorSpeed;
    uint8_t baseSpawnRate;
    uint8_t boostedSpawnRate;
    uint8_t currentSpawnRate;
    uint8_t baseMaxMeteors;
    uint8_t boostedMaxMeteors;
    uint8_t currentMaxMeteors;
    
    // Motion tuning
    float flowCorrectionStrength;
    float wobbleStrength;
    float wobbleSpeed;
    float separationDistance;
    float separationStrength;
    float maxVerticalSpeed;
    uint8_t trailFadeAmount;
    float attractorStrength;
    float attractorRadius;
    float centerAttractorStrength;
    float centerAttractorRadius;
    float forwardAcceleration;
    
    // ToF sensor integration
    TOFSensor* tofSensor;
    TOFAttractor tofAttractors[TOF_GRID_SIZE][TOF_GRID_SIZE];
    int16_t tofGrid[TOF_GRID_SIZE][TOF_GRID_SIZE];
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;
    uint8_t tofRotation;
    bool topRowActive;
    
    // Background planets
    Planet planets[MAX_PLANETS];
    uint8_t planetCount;
    uint8_t planetSpawnCounter;
    static constexpr uint8_t PLANET_SPAWN_INTERVAL = 120;
    
    // Timing
    uint32_t frameCount;
    
    // Helper methods
    void initMeteors();
    void initTofAttractors();
    void initPlanets();
    void updateTofData();
    void updateTofAttractors();
    void updateMeteors();
    void updatePlanets();
    void spawnMeteor();
    void spawnPlanet();
    void drawGradientBackground();
    void drawPlanets();
    void drawMeteors();
    void fadeTrails();
    
    // Coordinate rotation for ToF
    void rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY);

public:
    MeteorShowerEffect(Matrix* matrix, TOFSensor* sensor = nullptr);
    
    void reset() override;
    void update() override;
    const char* getName() const override;
    
    // Configuration
    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
    void setTofRotation(uint8_t rotation);
};
