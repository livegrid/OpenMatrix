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
    uint16_t x, y;
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
    uint16_t alienSpacingX;
    uint16_t alienSpacingY;
    uint16_t alienStartY;
    uint16_t alienMoveSpeed;
    uint16_t alienDropAmount;
    uint8_t alienMoveInterval;
    float alienShootChance;
    float alienBulletSpeed;
    
    // Player configuration
    float playerX, playerY;
    float playerTargetX;
    uint16_t playerWidth, playerHeight;
    uint16_t playerYOffset;
    float playerSmoothing;
    uint8_t playerHitTimer;
    uint8_t playerHitFlipFrames;
    
    // Bullet configuration
    float bulletSpeed;
    uint16_t bulletWidth, bulletHeight;
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
    bool topRowActive;  // Used for shooting trigger
    bool handsRaised;
    uint8_t handRaiseFrames;
    uint8_t handLowerFrames;
    
    // Blob detection parameters
    uint8_t minBlobCells;
    uint8_t missingBlobRecentFrames;
    uint32_t lastBlobFrame;
    uint8_t lastBlobSize;
    float filteredBlobX;
    
    // Timing
    uint32_t frameCount;
    
    // Play area dimensions (portrait: width < height)
    uint16_t playWidth;
    uint16_t playHeight;
    
    // Actual matrix dimensions (landscape)
    uint16_t matrixWidth;
    uint16_t matrixHeight;
    
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
    
    // Portrait→landscape rotation helpers: game coords → matrix coords
    void drawGamePixel(int16_t gx, int16_t gy, const CRGB& color);
    void drawGameRect(int16_t gx, int16_t gy, int16_t gw, int16_t gh, const CRGB& color);
    void drawGameCircle(int16_t gx, int16_t gy, int16_t r, const CRGB& color);
    
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
    void setTofRotation(uint16_t rotation);
};
