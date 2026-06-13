#pragma once

#include "Effect.h"
#include "PVector.h"
#include <Arduino.h>
#include "../TOFSensor/TOFInteractionManager.h"

class TOFSensor;

struct MeteorBullet {
    float x, y;
    bool active;
};

struct GameMeteor {
    float x, y;
    float vx, vy;
    float radius;
    uint8_t hue;
    bool active;
};

struct MeteorBlobResult {
    float x;
    uint8_t size;
    bool valid;
};

struct MeteorStar {
    uint16_t x, y;
    uint8_t brightness;
};

struct MeteorPlanet {
    PVector pos;
    PVector vel;
    float size;
    uint8_t hue;
    bool hasRings;
};

class MeteorShowerEffect : public Effect {
private:
    static constexpr uint8_t MAX_METEORS = 8;
    static constexpr uint8_t MAX_PLAYER_BULLETS = 5;
    static constexpr uint8_t MAX_STARS = 24;
    static constexpr uint8_t MAX_PLANETS = 2;

    // Play area (portrait game coords)
    uint16_t playWidth;
    uint16_t playHeight;
    uint16_t matrixWidth;
    uint16_t matrixHeight;

    // Player
    float playerX, playerY;
    float playerTargetX;
    uint16_t playerWidth, playerHeight;
    uint16_t playerYOffset;
    float playerSmoothing;
    uint8_t playerHitTimer;
    uint8_t playerHitFlipFrames;

    // Bullets
    MeteorBullet playerBullets[MAX_PLAYER_BULLETS];
    float bulletSpeed;
    uint16_t bulletWidth, bulletHeight;
    uint8_t bulletCooldown;
    uint8_t bulletCooldownFrames;

    // Meteors
    GameMeteor meteors[MAX_METEORS];
    uint8_t meteorSpawnCounter;
    uint8_t meteorSpawnInterval;
    uint8_t maxActiveMeteors;
    float meteorBaseSpeed;
    float baselineMeteorSpeed;
    uint8_t baselineSpawnInterval;
    uint8_t baselineMaxMeteors;

    // Game state
    uint16_t score;
    uint8_t lives;
    bool gameOver;
    uint32_t frameCount;
    uint32_t gameplayFrames;

    // Background
    MeteorStar stars[MAX_STARS];
    MeteorPlanet planets[MAX_PLANETS];
    uint8_t planetCount;
    uint8_t planetSpawnCounter;

    // ToF
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

    void updateTofData();
    void resetGame();
    void resetPlayer();
    void resetMeteors();
    void updateDifficultyRamp();
    void updatePlayer(MeteorBlobResult& blob);
    void updateBullets();
    void updateMeteors();
    void spawnMeteor();
    void checkCollisions();

    void initStars();
    void initPlanets();
    void spawnPlanet();
    void updatePlanets();

    void drawGamePixel(int16_t gx, int16_t gy, const CRGB& color);
    void drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color);
    void drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color);

    void drawGradientBackground();
    void drawStars();
    void drawPlanets();
    void drawMeteors();
    void drawBullets();
    void drawPlayer();
    void drawHUD();
    void drawGameOver();

    static float randomFloat();

public:
    MeteorShowerEffect(Matrix* matrix, TOFSensor* sensor = nullptr);
    ~MeteorShowerEffect() override;

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
};
