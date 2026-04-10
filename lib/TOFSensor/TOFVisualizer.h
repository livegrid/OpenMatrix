#pragma once

#include <Arduino.h>
#include "TOFSensor.h"
#include "TOFInteractionManager.h"
#include "../Matrix/Matrix.h"

class TOFVisualizer {
private:
    TOFSensor* sensor;
    Matrix* matrix;
    TOFInteractionManager interactionManager;
    int16_t minDistance;
    int16_t maxDistance;

    static constexpr bool kHideOutsideActiveBand = true;
    static constexpr uint16_t kStatusBarPixels = 14;

    uint16_t distanceToColor(int16_t distance);
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

    void drawHeatmapAndOverlays(const InteractionData& interaction, uint16_t plotW, uint16_t plotH);
    void drawStatusBar(const InteractionData& interaction);

public:
    TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay = nullptr);
    void setMatrix(Matrix* matrixDisplay);
    void setDistanceRange(int16_t minDist, int16_t maxDist);
    void draw();
    void drawWithBlockSize(uint8_t blockSize);
    void drawWithBlockSizes(uint8_t blockSizeX, uint8_t blockSizeY);
};
