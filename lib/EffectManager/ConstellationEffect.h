#pragma once

#include "Effect.h"
#include <Arduino.h>

// Forward declaration
class TOFSensor;

// Phase 5: Struct-of-arrays, ~14 KB for 2000 stars (vs ~38 KB for 1200 with old design).
// Personality (hue, twinkle speed, saturation) derived from index hash at draw/update time.

class ConstellationEffect : public Effect {
private:
    // Constants (memory layout: 2000 * 6 = 12 KB for core arrays)
    static constexpr uint16_t MAX_STARS = 2000;
    static constexpr uint8_t TOF_GRID_SIZE = 8;

    // Brightness smoothing (integer, like PlanktonField)
    static constexpr uint8_t BRIGHTNESS_RISE_SHIFT = 2;   // ~25%/frame rise
    static constexpr uint8_t BRIGHTNESS_FADE_SHIFT = 4;  // ~6%/frame fade

    // Hue range: cyan to blue/purple (FastLED 120-173)
    static constexpr uint8_t HUE_BASE = 120;
    static constexpr uint8_t HUE_RANGE = 54;
    static constexpr uint8_t HUE_ACTIVATED = 12;         // Orange-red when hand detected
    static constexpr uint8_t SAT_BASE = 51;              // ~20%
    static constexpr uint8_t SAT_RANGE = 103;            // up to ~60%

    // Struct-of-arrays: no padding waste, cache-friendly
    uint16_t px[MAX_STARS];
    uint16_t py[MAX_STARS];
    uint8_t  brightness[MAX_STARS];
    uint8_t  currentHue[MAX_STARS];  // Smoothed hue for activation glow transition

    uint16_t starCount;

    // Screen dimensions
    uint16_t screenWidth;
    uint16_t screenHeight;

    // TOF sensor integration
    TOFSensor* tofSensor;
    int16_t tofGrid[TOF_GRID_SIZE][TOF_GRID_SIZE];
    bool tofGridReady;
    int16_t minDetectionDistance;
    int16_t maxDetectionDistance;
    uint16_t tofRotation;

    // Depth field for smooth activation (8x8 normalized values 0-1)
    float depthField[TOF_GRID_SIZE][TOF_GRID_SIZE];

    // Sin lookup for twinkle (256 bytes, 0-255 maps to twinkle multiplier ~64-255)
    static const uint8_t SIN_TABLE[256];

    // Helper methods - TOF
    void updateTofData();
    void buildDepthField();
    void rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY);
    float getDepthBoostAt(float x, float y);

    // Helper methods - Stars (derive personality from index)
    void initStars();
    void updateStars();
    void drawStars();
    uint8_t getBaseHue(uint16_t i) const;
    uint8_t getTwinklePhase(uint16_t i, uint32_t time) const;
    uint8_t getSaturation(uint16_t i) const;

public:
    ConstellationEffect(Matrix* matrix, TOFSensor* sensor = nullptr);

    void reset() override;
    void update() override;
    const char* getName() const override;

    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
    void setTofRotation(uint16_t rotation);
};
