#pragma once

#include <Arduino.h>
#include "TOFSensor.h"
#include "../Aquarium/AquariumSettings.h"

struct InteractionData {
    bool hasBlob;
    float blobX, blobY;        // Normalized 0-1 position
    uint8_t blobSize;          // Number of active cells
    float velocityMag;         // Movement speed (pixels/frame)
    float velocityX, velocityY; // Direction
    int16_t depthMap[8][8];    // Processed depth for silhouette
};

class TOFInteractionManager {
private:
    TOFSensor* sensor;
    
    // Background subtraction
    int16_t baseline[8][8];
    bool baselineReady;
    unsigned long baselineCalibrationTime;
    static constexpr unsigned long BASELINE_CALIBRATION_DURATION = 2000; // 2 seconds
    
    // Current frame processing
    bool activeCells[8][8];
    int16_t currentDepth[8][8];
    
    // Blob tracking
    float lastBlobX, lastBlobY;
    float smoothedVelocityX, smoothedVelocityY;
    unsigned long lastUpdateTime;
    
    // Helper methods
    void rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY);
    void updateBaseline();
    void detectActiveCells();
    bool findLargestBlob(uint8_t& blobSize, float& blobX, float& blobY);
    void updateVelocity(float blobX, float blobY);
    
public:
    TOFInteractionManager(TOFSensor* tofSensor);
    
    void update();
    InteractionData getInteractionData() const;
    void calibrateBaseline();
    bool isBaselineReady() const { return baselineReady; }
};
