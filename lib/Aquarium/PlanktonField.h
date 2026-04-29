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
    static constexpr uint16_t MAX_PLANKTON = PLANKTON_MAX_COUNT;
    static constexpr uint8_t TOF_GRID_SIZE = PLANKTON_TOF_GRID_SIZE;

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
            uint8_t target = 0;
            if (depthBoost > PLANKTON_ACTIVATION_FLOOR) {
                float activated = (depthBoost - PLANKTON_ACTIVATION_FLOOR) /
                                  (1.0f - PLANKTON_ACTIVATION_FLOOR);
                activated = constrain(activated, 0.0f, 1.0f);
                target = (uint8_t)(PLANKTON_PUNCH_MIN_BRIGHTNESS +
                                   activated * (255 - PLANKTON_PUNCH_MIN_BRIGHTNESS));
            }

            int16_t diff = (int16_t)target - (int16_t)brightness[i];
            if (diff > 0) {
                int16_t rise = max(1, diff >> PLANKTON_BRIGHTNESS_RISE_SHIFT);
                brightness[i] = min(255, (int)brightness[i] + rise);
            } else if (diff < 0) {
                int16_t fade = min(-1, diff >> PLANKTON_BRIGHTNESS_FADE_SHIFT);
                brightness[i] = max(0, (int)brightness[i] + fade);
            }
        }
    }

    void draw() {
        if (!matrix || planktonCount == 0) return;

        for (uint16_t i = 0; i < planktonCount; i++) {
            if (brightness[i] < PLANKTON_MIN_DRAW_BRIGHTNESS) continue;

            uint16_t x = px[i];
            uint16_t y = py[i];
            if (x >= screenWidth || y >= screenHeight) continue;

            uint8_t hue = PLANKTON_HUE_BASE + (uint8_t)((i * 2654435761u) >> 24) % PLANKTON_HUE_RANGE;
            uint8_t sat = PLANKTON_SAT_BASE + (uint8_t)((i * 2654435761u) >> 20) % PLANKTON_SAT_RANGE;
            if (brightness[i] > 170) {
                uint8_t desaturate = (uint8_t)((brightness[i] - 170) / 2);
                sat = sat > desaturate ? sat - desaturate : 80;
            }
            CRGB color;
            hsv2rgb_rainbow(CHSV(hue, sat, brightness[i]), color);
            matrix->foreground->drawPixel(x, y, color);

            if (brightness[i] > 190 && (((millis() >> 4) + i * 17u) & 0x1F) < 3) {
                uint16_t sx = x + 1 < screenWidth ? x + 1 : x;
                CRGB sparkle = color;
                sparkle.nscale8_video(120);
                matrix->foreground->drawPixel(sx, y, sparkle);
            }
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
        const int16_t range = TOF_MAX_DETECTION_DIST - TOF_MIN_DETECTION_DIST;
        for (uint8_t y = 0; y < TOF_GRID_SIZE; y++) {
            for (uint8_t x = 0; x < TOF_GRID_SIZE; x++) {
                int16_t depth = interaction.depthMap[y][x];
                float value = 0;
                if (depth > TOF_MIN_DETECTION_DIST && depth < TOF_MAX_DETECTION_DIST) {
                    // Closer = stronger glow: 1.0 at min distance, 0.0 at max
                    value = 1.0f - (float)(depth - TOF_MIN_DETECTION_DIST) / (float)range;
                    if (value < 0) value = 0;
                    if (value > 1.0f) value = 1.0f;
                }
                depthField[y][x] = value;
            }
        }
    }

    float getDepthBoostAt(float x, float y) {
        float gx = (screenWidth > 1) ? (x / (float)(screenWidth - 1)) * (float)(TOF_GRID_SIZE - 1) : 0;
        float gy = (screenHeight > 1) ? (y / (float)(screenHeight - 1)) * (float)(TOF_GRID_SIZE - 1) : 0;

        int x0 = (int)gx;
        int y0 = (int)gy;
        int x1 = min((int)(TOF_GRID_SIZE - 1), x0 + 1);
        int y1 = min((int)(TOF_GRID_SIZE - 1), y0 + 1);
        float tx = gx - x0;
        float ty = gy - y0;

        x0 = max(0, min((int)(TOF_GRID_SIZE - 1), x0));
        y0 = max(0, min((int)(TOF_GRID_SIZE - 1), y0));

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
