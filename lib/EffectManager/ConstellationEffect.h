#pragma once

#include "Effect.h"
#include "PVector.h"
#include <Arduino.h>

// Forward declaration
class TOFSensor;

// Star structure - twinkling stars that react to TOF depth
struct ConstellationStar {
    float x, y;
    float baseBrightness;      // 30-70 range
    float twinkleSpeed;        // How fast it twinkles
    float twinkleOffset;       // Phase offset for twinkle
    uint8_t baseHue;           // HSB hue (180-260 cyan to purple)
    float currentHue;          // Smoothed hue state
    uint8_t saturation;        // HSB saturation (20-60)
    float currentBrightness;   // Smoothed brightness state
    
    void init(float px, float py);
    void updateBrightness(uint32_t time, float depthBoost);
};

class ConstellationEffect : public Effect {
private:
    // Constants
    static constexpr uint16_t MAX_STARS = 1200;
    static constexpr uint8_t TOF_GRID_SIZE = 8;
    
    // Stars (heap allocated to avoid stack overflow)
    ConstellationStar* stars;
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
    
    // Timing
    uint32_t frameCount;
    uint32_t lastUpdateTime;
    
    // Helper methods - TOF
    void updateTofData();
    void buildDepthField();
    void rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY);
    float getDepthBoostAt(float x, float y);
    
    // Helper methods - Stars
    void initStars();
    void updateStars();
    void drawStars();
    
public:
    ConstellationEffect(Matrix* matrix, TOFSensor* sensor = nullptr);
    ~ConstellationEffect();
    
    void reset() override;
    void update() override;
    const char* getName() const override;
    
    // Configuration
    void setTofSensor(TOFSensor* sensor);
    void setDetectionRange(int16_t minDist, int16_t maxDist);
    void setTofRotation(uint16_t rotation);
};
