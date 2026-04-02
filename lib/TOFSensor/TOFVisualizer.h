#pragma once

#include <Arduino.h>
#include "TOFSensor.h"
#include "../Matrix/Matrix.h"

class TOFVisualizer {
private:
    TOFSensor* sensor;
    Matrix* matrix;
    
    // Distance mapping parameters
    int16_t minDistance;  // Minimum distance in mm (closer = warmer colors)
    int16_t maxDistance;  // Maximum distance in mm (farther = cooler colors)
    
    // Convert distance to color (heat map style)
    uint16_t distanceToColor(int16_t distance);
    
    // Map a value from one range to another
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    
public:
    TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay = nullptr);
    
    // Set matrix pointer after construction (for static allocation)
    void setMatrix(Matrix* matrixDisplay);
    
    void setDistanceRange(int16_t minDist, int16_t maxDist);
    
    // Draw the sensor data on the matrix
    void draw();
    
    // Draw with custom block size (deprecated - use draw() which auto-scales)
    void drawWithBlockSize(uint8_t blockSize);
    
    // Draw with separate X and Y block sizes for non-square displays
    void drawWithBlockSizes(uint8_t blockSizeX, uint8_t blockSizeY);
};
