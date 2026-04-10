#pragma once

#include <Arduino.h>
#include "Matrix.h"

// Shared 20x20 RGB332 spaceman sprites (0 = transparent) used by Space Invaders and Gravity Flap.
extern const uint8_t kSpacemanSpriteMove[20][20] PROGMEM;
extern const uint8_t kSpacemanSpriteBoost[20][20] PROGMEM;
extern const uint8_t kSpacemanSpriteHit[20][20] PROGMEM;

// Nearest-neighbor draw centered at (cx, cy); drawPixel(gx, gy, color) is in portrait game coords.
template <typename DrawPixelFn>
inline void drawSpacemanSpriteScaled(const uint8_t (*sprite)[20], int16_t cx, int16_t cy,
                                     int16_t drawW, int16_t drawH, DrawPixelFn&& drawPixel) {
    if (drawW < 1) drawW = 1;
    if (drawH < 1) drawH = 1;
    int16_t left = cx - drawW / 2;
    int16_t top = cy - drawH / 2;
    constexpr int16_t kSrc = 20;
    for (int16_t sy = 0; sy < drawH; sy++) {
        int16_t srcY = (sy * kSrc) / drawH;
        if (srcY < 0) srcY = 0;
        if (srcY >= kSrc) srcY = kSrc - 1;
        for (int16_t sx = 0; sx < drawW; sx++) {
            int16_t srcX = (sx * kSrc) / drawW;
            if (srcX < 0) srcX = 0;
            if (srcX >= kSrc) srcX = kSrc - 1;
            uint8_t px = pgm_read_byte(&sprite[srcY][srcX]);
            if (px == 0) continue;

            uint8_t r3 = (px >> 5) & 0x07;
            uint8_t g3 = (px >> 2) & 0x07;
            uint8_t b2 = px & 0x03;
            CRGB color((uint8_t)((r3 * 255) / 7), (uint8_t)((g3 * 255) / 7), (uint8_t)((b2 * 255) / 3));
            drawPixel(left + sx, top + sy, color);
        }
    }
}
