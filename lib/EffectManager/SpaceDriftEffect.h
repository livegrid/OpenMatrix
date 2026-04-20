#pragma once

#include "Effect.h"
#include "../TOFSensor/TOFInteractionManager.h"
#include <Arduino.h>

class TOFSensor;

/** Visual "species" for drift bodies — drives both palette and draw treatment. */
enum class DriftBodyKind : uint8_t {
    Generic = 0,
    GasGiant,
    Ocean,
    Ice,
    Lava,
    Count_
};

struct DriftStar {
    float x;
    float y;
    uint8_t layer;
    uint8_t hue;
    uint8_t baseBrightness;
};

struct DriftCloud {
    float x;
    float y;
    float radius;
    uint8_t hue;
    int8_t driftX;
    int8_t driftY;
};

struct DriftBody {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    uint8_t hue;
    uint8_t kind;  // DriftBodyKind
    bool ringed;
    uint16_t ttl;
};

struct DriftAlien {
    float x;
    float y;
    float vx;
    float vy;
    uint8_t size;
    uint8_t hue;
    uint8_t blinkPhase;
    uint16_t ttl;
};

/** Huge distant planets — slow world drift + low parallax vs camera for depth. */
struct DriftFarBody {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    float parallax;
    uint8_t hue;
    uint8_t kind;  // DriftBodyKind
    bool ringed;
    uint16_t textureSeed;
};

/** Short-lived streak across the view (world-space, medium parallax). */
struct DriftRocket {
    bool active;
    float x;
    float y;
    float vx;
    float vy;
    uint8_t hue;
    uint16_t ttl;
};

class SpaceDriftEffect : public Effect {
private:
    static constexpr int16_t kTofEffectRotationDeg = 180;
    static constexpr uint8_t MAX_STARS = 104;
    static constexpr uint8_t MAX_CLOUDS = 6;
    static constexpr uint8_t MAX_FAR_BODIES = 8;
    static constexpr uint8_t MAX_BODIES = 10;
    static constexpr uint8_t MAX_ALIENS = 8;
    static constexpr uint8_t MAX_ROCKETS = 4;

    TOFSensor* tofSensor;
    TOFInteractionManager* tofInteraction;
    InteractionData tofInteractionData;
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;

    uint16_t matrixWidth;
    uint16_t matrixHeight;
    uint16_t playWidth;
    uint16_t playHeight;

    uint32_t frameCount;
    float camX;
    float camY;
    float camVX;
    float camVY;
    float axisX;
    float axisY;
    float idleDriftPhase;

    float worldSpanX;
    float worldSpanY;

    DriftStar stars[MAX_STARS];
    DriftCloud clouds[MAX_CLOUDS];
    DriftFarBody farBodies[MAX_FAR_BODIES];
    DriftBody bodies[MAX_BODIES];
    DriftAlien aliens[MAX_ALIENS];
    DriftRocket rockets[MAX_ROCKETS];
    uint16_t rocketSpawnCountdown;

    static uint32_t noiseSeed;
    static float randomFloat();

    void resetWorld();
    void updateTofData();
    void updateCameraFromInteraction();
    void updateWorldObjects();
    void applyPalmPush();

    void wrapAroundCamera(float& x, float& y, float factorX, float factorY);
    void respawnBodyNearCamera(DriftBody& b);
    void respawnAlienNearCamera(DriftAlien& a);
    void respawnFarBody(DriftFarBody& fb);
    void respawnBodyAtEdge(DriftBody& b);
    void respawnFarBodyAtEdge(DriftFarBody& fb);
    void trySpawnRocket();
    void spawnRocket(uint8_t slot);

    void drawGamePixel(int16_t gx, int16_t gy, const CRGB& color);
    void drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color);
    void drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color);
    void drawGameCircleClipped(int16_t gx, int16_t gy, int16_t r, const CRGB& color);
    // Draw a single horizontal row of a circle (for banded gas giants etc.).
    void drawCircleRow(int16_t sx, int16_t sy, int16_t r, int16_t y, const CRGB& color);

    void drawBackground();
    void drawFarBodies();
    void drawClouds();
    void drawStars();
    void drawBodies();
    void drawRockets();
    void drawAliens();
    void drawSpaceman();
    void drawAmbientHud();

    // Per-kind draw treatments for small bodies.
    void drawBodyGeneric(const DriftBody& b, int16_t sx, int16_t sy, int16_t r);
    void drawBodyGasGiant(const DriftBody& b, int16_t sx, int16_t sy, int16_t r);
    void drawBodyOcean(const DriftBody& b, int16_t sx, int16_t sy, int16_t r, uint8_t idx);
    void drawBodyIce(const DriftBody& b, int16_t sx, int16_t sy, int16_t r);
    void drawBodyLava(const DriftBody& b, int16_t sx, int16_t sy, int16_t r, uint8_t idx);

    // Per-kind draw treatments for far/huge bodies.
    void drawFarBodyGeneric(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r, int16_t pulse, uint8_t idx);
    void drawFarBodyGasGiant(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r, int16_t pulse);
    void drawFarBodyOcean(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r, int16_t pulse, uint8_t idx);
    void drawFarBodyIce(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r, int16_t pulse);
    void drawFarBodyLava(const DriftFarBody& fb, int16_t sx, int16_t sy, int16_t r, int16_t pulse, uint8_t idx);

    // Pick a visual kind and a kind-appropriate hue for a new body.
    void pickBodyKindAndHue(uint8_t& kind, uint8_t& hue);

public:
    SpaceDriftEffect(Matrix* matrix, TOFSensor* sensor = nullptr);

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
};
