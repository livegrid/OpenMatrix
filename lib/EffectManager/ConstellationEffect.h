#pragma once

#include "Effect.h"
#include <Arduino.h>

#include "../TOFSensor/TOFInteractionManager.h"

class TOFSensor;

// Shooting star data
struct ShootingStar {
    int16_t x;
    int16_t y;
    int8_t dx;         // horizontal speed (negative = moving left)
    int8_t dy;         // slight vertical drift
    uint8_t length;    // trail length in pixels
    uint8_t life;      // frames remaining
    bool active;
};

class ConstellationEffect : public Effect {
private:
    static constexpr uint16_t MAX_STARS = 2000;
    static constexpr uint8_t TOF_GRID_SIZE = 8;

    // Brightness smoothing
    static constexpr uint8_t BRIGHTNESS_RISE_SHIFT = 2;   // ~25%/frame rise
    static constexpr uint8_t BRIGHTNESS_FADE_SHIFT = 4;   // ~6%/frame fade

    // Star colors: near-white with subtle blue tint
    static constexpr uint8_t HUE_STAR = 160;        // Blue-white base
    static constexpr uint8_t HUE_STAR_RANGE = 20;   // Tiny variation (150-170)
    static constexpr uint8_t SAT_DIM = 10;           // Dim stars: nearly white
    static constexpr uint8_t SAT_MEDIUM = 20;        // Medium stars: hint of color
    static constexpr uint8_t SAT_BRIGHT = 40;        // Bright stars: subtle blue tint
    static constexpr uint8_t HUE_ACTIVATED = 40;     // Warm gold when hand detected
    static constexpr uint8_t SAT_ACTIVATED = 80;     // More saturated when activated

    // Shooting stars
    static constexpr uint8_t MAX_SHOOTING_STARS = 3;
    static constexpr uint16_t SHOOT_SPAWN_CHANCE = 300; // 1/N chance per frame

    // Struct-of-arrays for star data
    uint16_t px[MAX_STARS];
    uint16_t py[MAX_STARS];
    uint8_t  brightness[MAX_STARS];
    uint8_t  currentHue[MAX_STARS];
    uint8_t  currentSat[MAX_STARS];   // Smoothed saturation for glow transition

    uint16_t starCount;

    // Screen dimensions
    uint16_t screenWidth;
    uint16_t screenHeight;

    // Shooting stars
    ShootingStar shootingStars[MAX_SHOOTING_STARS];

    // TOF sensor integration
    TOFSensor* tofSensor;
    TOFInteractionManager* tofInteraction;
    InteractionData tofInteractionData;
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;

    // Depth field (8x8 normalized 0.0-1.0)
    float depthField[TOF_GRID_SIZE][TOF_GRID_SIZE];

    // Sin lookup for twinkle
    static const uint8_t SIN_TABLE[256];

    // TOF helpers
    void updateTofData();
    void buildDepthField();
    float getDepthBoostAt(float x, float y);

    // Star helpers
    void initStars();
    void updateStars();
    void drawStars();
    uint8_t getBaseHue(uint16_t i) const;
    uint8_t getTwinklePhase(uint16_t i, uint32_t time) const;
    uint8_t getBaseBrightness(uint16_t i) const;
    uint8_t getBaseSaturation(uint16_t i) const;
    uint8_t getBrightnessClass(uint16_t i) const; // 0=dim, 1=medium, 2=bright

    // Shooting star helpers
    void updateShootingStars();
    void drawShootingStars();
    void spawnShootingStar();

public:
    ConstellationEffect(Matrix* matrix, TOFSensor* sensor = nullptr);

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
};
