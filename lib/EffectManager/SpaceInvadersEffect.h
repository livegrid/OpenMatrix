#pragma once

#include "Effect.h"
#include "PVector.h"
#include <Arduino.h>

// Forward declaration
class TOFSensor;

// Bullet structure
struct Bullet {
    float x, y;
    bool active;
    bool isPlayerBullet;  // true = player bullet (moves up), false = alien bullet (moves down)
};

// Alien structure
struct Alien {
    float x, y;
    bool alive;
    uint8_t type;      // Pattern type (0-2)
    uint8_t hue;       // Color hue
    uint8_t animFrame; // Animation frame (0 or 1)
};

// Background star
struct Star {
    uint8_t x, y;
    uint8_t brightness;
};

// Background planet (same as meteor shower)
struct GamePlanet {
    PVector pos;
    PVector vel;
    float size;
    uint8_t hue;
    bool hasRings;
};

// ToF blob detection result
struct BlobResult {
    float x;           // Center X position mapped to play area
    uint8_t size;      // Number of active cells
    bool valid;        // Whether a blob was detected
};

class SpaceInvadersEffect : public Effect {
private:
    // Game configuration
    static constexpr uint8_t MAX_PLAYER_BULLETS = 5;
    static constexpr uint8_t MAX_ALIEN_BULLETS = 10;
    static constexpr uint8_t MAX_ALIENS = 30;  // 5 columns x 4 rows max
    static constexpr uint8_t MAX_STARS = 20;
    static constexpr uint8_t MAX_PLANETS = 2;
    static constexpr uint8_t TOF_GRID_SIZE = 8;
    
    // Alien configuration
    uint8_t alienRows;
    uint8_t alienCols;
    uint8_t alienSpacingX;
    uint8_t alienSpacingY;
    uint8_t alienStartY;
    uint8_t alienMoveSpeed;
    uint8_t alienDropAmount;
    uint8_t alienMoveInterval;
    float alienShootChance;
    float alienBulletSpeed;
    
    // Player configuration
    float playerX, playerY;
    float playerTargetX;
    uint8_t playerWidth, playerHeight;
    uint8_t playerYOffset;
    float playerSmoothing;
    uint8_t playerHitTimer;
    uint8_t playerHitFlipFrames;
    
    // Bullet configuration
    float bulletSpeed;
    uint8_t bulletWidth, bulletHeight;
    uint8_t bulletCooldown;
    uint8_t bulletCooldownFrames;
    
    // Game state
    Alien aliens[MAX_ALIENS];
    uint8_t alienCount;
    int8_t alienDirection;
    uint8_t alienMoveCounter;
    
    Bullet playerBullets[MAX_PLAYER_BULLETS];
    Bullet alienBullets[MAX_ALIEN_BULLETS];
    
    uint16_t score;
    uint8_t lives;
    bool gameOver;
    bool gameWon;
    uint8_t level;
    
    // Background
    Star stars[MAX_STARS];
    GamePlanet planets[MAX_PLANETS];
    uint8_t planetCount;
    uint8_t planetSpawnCounter;
    
    // ToF sensor integration
    TOFSensor* tofSensor;
    int16_t tofGrid[TOF_GRID_SIZE][TOF_GRID_SIZE];
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;
    uint8_t tofRotation;
    bool topRowActive;  // Used for shooting trigger
    
    // Blob detection parameters
    uint8_t minBlobCells;
    uint8_t missingBlobRecentFrames;
    uint32_t lastBlobFrame;
    uint8_t lastBlobSize;
    
    // Timing
    uint32_t frameCount;
    
    // Play area dimensions (may differ from matrix for portrait orientation)
    uint8_t playWidth;
    uint8_t playHeight;
    
    // Alien pixel patterns (8x8)
    static const uint8_t alienPatternA[3][8];
    static const uint8_t alienPatternB[3][8];
    static const uint8_t alienHues[4];
    
    // Helper methods - ToF
    void updateTofData();
    void rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY);
    BlobResult findLargestBlob();
    void updateTopRowActive();
    
    // Helper methods - Game logic
    void resetGame();
    void resetPlayer();
    void resetAliens();
    void nextLevel();
    void updatePlayer(BlobResult& blob);
    void updateBullets();
    void updateAliens();
    void updateAlienBullets();
    void checkCollisions();
    void checkWinCondition();
    void fireBullet();
    
    // Helper methods - Background
    void initStars();
    void initPlanets();
    void spawnPlanet();
    void updatePlanets();
    
    // Helper methods - Drawing
    void drawGradientBackground();
    void drawStars();
    void drawPlanets();
    void drawAliens();
    void drawPixelAlien(Alien& alien);
    void drawBullets();
    void drawPlayer();
    void drawHUD();
    void drawGameOver();
    void drawWinScreen();
    
    // Utility
    static float randomFloat();
    
public:
    SpaceInvadersEffect(Matrix* matrix, TOFSensor* sensor = nullptr);
    
    void reset() override;
    void update() override;
    const char* getName() const override;
    
    // Configuration
    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
    void setTofRotation(uint8_t rotation);
};
