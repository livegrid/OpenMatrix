#pragma once

#include "Effect.h"
#include <Arduino.h>
#include "../TOFSensor/TOFInteractionManager.h"

class TOFSensor;

struct FlapPlatform {
    float x, y, w;
    float speed;
    int8_t dir;
    uint8_t hue;
};

struct FlapBlob {
    float x, y;
    uint8_t size;
    bool valid;
};

class GravityFlapEffect : public Effect {
private:
    static constexpr uint8_t MAX_PLATFORMS = 9;
    static constexpr int16_t kTofEffectRotationDeg = 0;

    TOFSensor* tofSensor;
    TOFInteractionManager* tofInteraction;
    InteractionData tofInteractionData;
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;
    bool handsRaised;

    uint8_t minBlobCells;
    uint8_t missingBlobRecentFrames;
    uint32_t lastBlobFrame;
    float filteredBlobX;

    uint32_t frameCount;
    uint16_t playWidth;
    uint16_t playHeight;
    uint16_t matrixWidth;
    uint16_t matrixHeight;

    float playerX, playerY, playerVY;
    float playerTargetX;
    uint16_t playerWidth, playerHeight;
    float playerSmoothing;

    float gravity;
    float boostVelocity;
    float maxFallSpeed;
    float maxUpSpeed;
    uint16_t pulseCooldownFrames;
    float pulseDeltaGame;
    float platformHeight;
    float platformSpacing;
    float platformSpeedMin;
    float platformSpeedMax;
    uint16_t platformMinW;
    uint16_t platformMaxW;

    FlapPlatform platforms[MAX_PLATFORMS];
    uint8_t platformCount;

    float cameraY;
    float startY;
    uint16_t heightScore;
    float maxHeight;
    bool gameOver;

    float lastBlobGameY;
    bool haveLastBlobY;
    uint32_t lastPulseFrame;
    uint32_t lastBoostFrame;

    static uint32_t noiseSeed;
    static float randomFloat();

    void updateTofData();
    void resetGame();
    void initPlatforms();
    void spawnPlatform(uint8_t index, float y);
    void updatePlatforms();
    void ensurePlatformsBelowPlayer();
    void updatePlayer(FlapBlob& blob);
    bool detectBlobPulse(FlapBlob& blob);

    void drawGamePixel(int16_t gx, int16_t gy, const CRGB& color);
    void drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color);
    void drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color);
    void drawBackground();
    void drawWorld();
    void drawHUD();

public:
    GravityFlapEffect(Matrix* matrix, TOFSensor* sensor = nullptr);

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
};
