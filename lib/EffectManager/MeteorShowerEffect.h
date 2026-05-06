#pragma once

#include "Effect.h"
#include "PVector.h"
#include <Arduino.h>
#include "../TOFSensor/TOFInteractionManager.h"

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
    void applySeparation(Meteor* meteors, uint8_t count, uint8_t selfIndex, float separationDistance,
                         float separationStrength, uint8_t maxNeighbors, uint8_t maxChecks);
    void applyFlocking(Meteor* meteors, uint8_t count, uint8_t selfIndex, float neighborDistance,
                       float alignmentStrength, float cohesionStrength, uint8_t maxNeighbors,
                       uint8_t maxChecks);
    void applyAttractor(const PVector& attractorPos, float strength, float radius);
    void applyAttractorY(const PVector& attractorPos, float strength, float radius);
    void update(float targetSpeed, float maxVerticalSpeed, float velocityDrag);
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
    static constexpr uint8_t MAX_METEORS = 96;
    static constexpr uint8_t MAX_PLANETS = 3;
    static constexpr uint8_t TOF_GRID_SIZE = 8;
    static constexpr uint8_t MAX_BG_STARS = 56;
    static constexpr uint8_t MAX_NEBULA_CLOUDS = 4;
    
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
    uint8_t baseSpawnBurst;
    uint8_t boostedSpawnBurst;
    uint8_t currentSpawnBurst;
    uint8_t baseMaxMeteors;
    uint8_t boostedMaxMeteors;
    uint8_t currentMaxMeteors;
    
    // Motion tuning
    float flowCorrectionStrength;
    float wobbleStrength;
    float wobbleSpeed;
    float separationDistance;
    float separationStrength;
    float flockNeighborDistance;
    float alignmentStrength;
    float cohesionStrength;
    float maxVerticalSpeed;
    float velocityDrag;
    uint8_t trailFadeAmount;
    float attractorStrength;
    float attractorRadius;
    float centerAttractorStrength;
    float centerAttractorRadius;
    float forwardAcceleration;
    
    // ToF sensor integration
    TOFSensor* tofSensor;
    TOFInteractionManager* tofInteraction;
    TOFAttractor tofAttractors[TOF_GRID_SIZE][TOF_GRID_SIZE];
    InteractionData tofInteractionData;
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;
    bool topRowActive;
    
    // Background planets
    Planet planets[MAX_PLANETS];
    uint8_t planetCount;
    uint8_t planetSpawnCounter;
    static constexpr uint8_t PLANET_SPAWN_INTERVAL = 120;
    
    // Timing
    uint32_t frameCount;
    bool rotateEffect180;
    float recentSpawnY[4];
    uint8_t spawnHistoryIndex;
    uint8_t maxActiveAttractors;
    uint8_t maxSeparationNeighbors;
    uint8_t maxFlockNeighbors;
    uint8_t maxNeighborChecks;
    uint8_t trailPersistence;
    uint8_t meteorCompositeThreshold;

    struct BgStar {
        float x;
        float y;
        float speed;
        float twinklePhase;
        uint8_t hue;
        uint8_t baseBrightness;
        uint8_t layer;
    };
    struct NebulaCloud {
        float x;
        float y;
        float radius;
        float driftX;
        float driftY;
        float pulsePhase;
        uint8_t hue;
    };
    BgStar stars[MAX_BG_STARS];
    NebulaCloud clouds[MAX_NEBULA_CLOUDS];
    
    // Helper methods
    void initMeteors();
    void initTofAttractors();
    void initPlanets();
    void initBackgroundElements();
    void updateTofData();
    void updateTofAttractors();
    void updateMeteors();
    void updatePlanets();
    void updateBackgroundElements();
    void spawnMeteor();
    void spawnPlanet();
    void drawGradientBackground();
    void drawBackgroundElements();
    void drawPlanets();
    void drawMeteors();
    void fadeTrails();
    void transformEffectCoordinate(int16_t inX, int16_t inY, int16_t& outX, int16_t& outY) const;
    void drawEffectPixel(int16_t x, int16_t y, const CRGB& color);
    void drawEffectCircle(int16_t x, int16_t y, int16_t r, const CRGB& color);
    void drawLayerPixel(GFX_Layer* layer, int16_t x, int16_t y, const CRGB& color);
    void drawLayerCircle(GFX_Layer* layer, int16_t x, int16_t y, int16_t r, const CRGB& color);

public:
    MeteorShowerEffect(Matrix* matrix, TOFSensor* sensor = nullptr);
    ~MeteorShowerEffect() override;
    
    void reset() override;
    void update() override;
    const char* getName() const override;
    
    // Configuration
    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
};
