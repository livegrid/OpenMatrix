#pragma once

#include "Effect.h"
#include <Arduino.h>
#include "../TOFSensor/TOFInteractionManager.h"

class TOFSensor;

struct HopperAsteroid {
    float x;
    float radius;
};

struct HopperLane {
    bool safeLane;
    int8_t dir;
    float speed;
    uint8_t asteroidCount;
    uint8_t hue;
    HopperAsteroid asteroids[4];
};

struct HopperBlob {
    float x;
    bool valid;
};

class AsteroidHopperEffect : public Effect {
private:
    static constexpr int16_t kTofEffectRotationDeg = 180;
    static constexpr uint8_t MAX_LANES = 8;
    static constexpr uint8_t MAX_STARS = 18;

    TOFSensor* tofSensor;
    TOFInteractionManager* tofInteraction;
    InteractionData tofInteractionData;
    bool tofGridReady;
    bool handsRaised;
    bool palmActive;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;

    uint8_t minBlobCells;
    uint8_t missingBlobRecentFrames;
    uint32_t lastBlobFrame;
    float filteredBlobX;

    uint32_t frameCount;
    uint16_t playWidth;
    uint16_t playHeight;
    uint16_t matrixWidth;
    uint16_t matrixHeight;

    float playerX;
    float playerTargetX;
    uint8_t playerLane;
    uint8_t lives;
    uint16_t score;
    bool gameOver;

    bool prevHandsRaised;
    bool prevPalmActive;
    uint32_t hopCooldownUntil;
    uint32_t hopInputLatchedUntil;
    uint32_t nextHeldHopFrame;
    uint32_t shieldCooldownUntil;
    uint32_t shieldActiveUntil;
    uint32_t invulnerableUntil;

    HopperLane lanes[MAX_LANES];
    uint8_t laneCount;
    float laneSpacing;

    uint16_t playerWidth;
    uint16_t playerHeight;
    float playerSmoothing;

    uint32_t gameOverFrame;
    float speedScale;

    struct Star {
        uint16_t x;
        uint16_t y;
        uint8_t b;
    } stars[MAX_STARS];

    static uint32_t noiseSeed;
    static float randomFloat();

    void updateTofData();
    void resetGame();
    void resetRound(bool keepScoreAndLives);
    void initLanes();
    void initStars();
    void advanceDifficulty();

    void updatePlayer(const HopperBlob& blob);
    void updateLanes();
    void checkCollisions();
    void tryHopForward();
    void tryTriggerShield();

    float laneToY(uint8_t laneIdx) const;
    float wrapDistanceX(float a, float b) const;

    void drawGamePixel(int16_t gx, int16_t gy, const CRGB& color);
    void drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color);
    void drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color);
    void drawBackground();
    void drawLanes();
    void drawAsteroids();
    void drawPlayer();
    void drawHUD();
    void drawGoalGlow();
    void drawGameOver();

public:
    AsteroidHopperEffect(Matrix* matrix, TOFSensor* sensor = nullptr);

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
};
