#include "TOFVisualizer.h"

TOFVisualizer::TOFVisualizer(TOFSensor* tofSensor, Matrix* matrixDisplay) 
    : sensor(tofSensor), matrix(matrixDisplay) {
    // Default range: 100mm (close) to 2000mm (far)
    minDistance = 100;
    maxDistance = 2000;
}

void TOFVisualizer::setDistanceRange(int16_t minDist, int16_t maxDist) {
    minDistance = minDist;
    maxDistance = maxDist;
}

float TOFVisualizer::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t TOFVisualizer::distanceToColor(int16_t distance) {
    if (distance < 0) {
        // No detection - show black
        return matrix->background->color565(0, 0, 0);
    }
    
    // Constrain distance to range
    if (distance < minDistance) distance = minDistance;
    if (distance > maxDistance) distance = maxDistance;
    
    // Map distance to hue (0-360 degrees)
    // Close = Red (0°), Medium = Yellow/Green (120°), Far = Blue (240°)
    float hue = mapFloat(distance, minDistance, maxDistance, 0, 240);
    
    // Convert HSV to RGB (with full saturation and value)
    float h = hue / 60.0;
    float s = 1.0;
    float v = 1.0;
    
    int i = (int)h;
    float f = h - i;
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));
    
    float r, g, b;
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0; break;
    }
    
    // Convert to RGB565
    uint8_t r8 = (uint8_t)(r * 255);
    uint8_t g8 = (uint8_t)(g * 255);
    uint8_t b8 = (uint8_t)(b * 255);
    
    return matrix->background->color565(r8, g8, b8);
}

void TOFVisualizer::draw() {
    drawWithBlockSize(8);
}

void TOFVisualizer::rotateCoordinates(uint8_t x, uint8_t y, uint8_t& outX, uint8_t& outY) {
    // Rotate coordinates based on ROTATION setting
    // For 8x8 grid, valid rotations are 0, 90, 180, 270 degrees
    switch (ROTATION) {
        case 90:
            // 90° clockwise: (x, y) -> (7-y, x)
            outX = 7 - y;
            outY = x;
            break;
        case 180:
            // 180°: (x, y) -> (7-x, 7-y)
            outX = 7 - x;
            outY = 7 - y;
            break;
        case 270:
            // 270° clockwise (90° counter-clockwise): (x, y) -> (y, 7-x)
            outX = y;
            outY = 7 - x;
            break;
        case 0:
        default:
            // No rotation: (x, y) -> (x, y)
            outX = x;
            outY = y;
            break;
    }
}

void TOFVisualizer::drawWithBlockSize(uint8_t blockSize) {
    if (!sensor->isActive()) {
        // Sensor not active - clear screen and show error
        matrix->clearScreen();
        return;
    }
    
    // Clear the background
    matrix->background->fillScreen(matrix->background->color565(0, 0, 0));
    
    // Draw each sensor zone as a block on the matrix
    for (uint8_t y = 0; y < 8; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            // Get distance for this zone
            int16_t distance = sensor->getDistance(x, y);
            
            // Convert distance to color
            uint16_t color = distanceToColor(distance);
            
            // Apply rotation to display coordinates
            uint8_t px, py;
            rotateCoordinates(x, y, px, py);
            
            // Draw block on matrix
            px = px * blockSize;
            py = py * blockSize;
            
            // Fill the block
            matrix->background->fillRect(px, py, blockSize, blockSize, color);
        }
    }
}
