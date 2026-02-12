#ifndef PLANKTON_FIELD_H
#define PLANKTON_FIELD_H

#include <Arduino.h>
#include "Matrix.h"
#include "AquariumSettings.h"
#include "../TOFSensor/TOFInteractionManager.h"

// Plankton particles: invisible until presence detected, then glow. Static positions.
// Drawn on foreground to avoid water's chunked background updates overwriting them.
class PlanktonField {
public:
    static constexpr uint16_t MAX_PLANKTON = 2000;
    static constexpr uint8_t TOF_GRID_SIZE = 8;

    PlanktonField(Matrix* matrix)
        : matrix(matrix),
          screenWidth(matrix ? matrix->getXResolution() : 0),
          screenHeight(matrix ? matrix->getYResolution() : 0),
          planktonCount(0) {}

    void init() {
        if (!matrix) return;
        screenWidth = matrix->getXResolution();
        screenHeight = matrix->getYResolution();
        planktonCount = MAX_PLANKTON;

        for (uint16_t i = 0; i < planktonCount; i++) {
            px[i] = random(0, screenWidth);
            py[i] = random(0, screenHeight);
            brightness[i] = 0;
        }
    }

    void update(const InteractionData& interaction) {
        if (!matrix || planktonCount == 0) return;

        buildDepthField(interaction);

        for (uint16_t i = 0; i < planktonCount; i++) {
            float depthBoost = getDepthBoostAt((float)px[i], (float)py[i]);
            uint8_t target = (uint8_t)(depthBoost * 255);

            int16_t diff = (int16_t)target - (int16_t)brightness[i];
            if (diff > 0) {
                int16_t rise = max(1, diff >> 2);
                brightness[i] = min(255, (int)brightness[i] + rise);
            } else if (diff < 0) {
                int16_t fade = min(-1, diff >> 4);
                brightness[i] = max(0, (int)brightness[i] + fade);
            }
        }
    }

    void draw() {
        if (!matrix || planktonCount == 0) return;

        for (uint16_t i = 0; i < planktonCount; i++) {
            if (brightness[i] == 0) continue;

            uint16_t x = px[i];
            uint16_t y = py[i];
            if (x >= screenWidth || y >= screenHeight) continue;

            uint8_t hue = 96 + (uint8_t)((i * 2654435761u) >> 24) % 40;
            uint8_t sat = 100 + (uint8_t)((i * 2654435761u) >> 20) % 60;
            CRGB color;
            hsv2rgb_rainbow(CHSV(hue, sat, brightness[i]), color);
            matrix->foreground->drawPixel(x, y, color);
        }
    }

private:
    Matrix* matrix;
    uint16_t screenWidth;
    uint16_t screenHeight;
    uint16_t planktonCount;

    uint16_t px[MAX_PLANKTON];
    uint16_t py[MAX_PLANKTON];
    uint8_t brightness[MAX_PLANKTON];

    float depthField[TOF_GRID_SIZE][TOF_GRID_SIZE];

    void buildDepthField(const InteractionData& interaction) {
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                int16_t depth = interaction.depthMap[y][x];
                float value = 0;
                if (depth > TOF_MIN_DETECTION_DIST && depth < TOF_MAX_DETECTION_DIST) {
                    value = 1.0f;
                }
                depthField[y][x] = value;
            }
        }
    }

    float getDepthBoostAt(float x, float y) {
        float gx = (screenWidth > 1) ? (x / (float)(screenWidth - 1)) * 7.0f : 0;
        float gy = (screenHeight > 1) ? (y / (float)(screenHeight - 1)) * 7.0f : 0;

        int x0 = (int)gx;
        int y0 = (int)gy;
        int x1 = min(7, x0 + 1);
        int y1 = min(7, y0 + 1);
        float tx = gx - x0;
        float ty = gy - y0;

        x0 = max(0, min(7, x0));
        y0 = max(0, min(7, y0));

        float v00 = depthField[y0][x0];
        float v10 = depthField[y0][x1];
        float v01 = depthField[y1][x0];
        float v11 = depthField[y1][x1];

        float v0 = v00 + (v10 - v00) * tx;
        float v1 = v01 + (v11 - v01) * tx;
        return v0 + (v1 - v0) * ty;
    }
};

#endif
