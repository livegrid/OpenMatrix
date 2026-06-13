#pragma once

#include <Arduino.h>
#include "Matrix.h"

// Proximity warning sprite — portrait-oriented, 64 px wide.
constexpr uint16_t kStepBackSpriteWidth = 64;
constexpr uint16_t kStepBackSpriteHeight = 93;
constexpr uint8_t kStepBackSpriteRowBytes = 8;

extern const uint8_t kStepBackSpriteBitmap[] PROGMEM;

// Draw at top-left (gx0, gy0) in portrait game coords; unset bits are transparent.
template <typename DrawPixelFn>
inline void drawStepBackSprite(int16_t gx0, int16_t gy0, const CRGB& color, DrawPixelFn&& drawPixel) {
    for (uint16_t y = 0; y < kStepBackSpriteHeight; y++) {
        const uint8_t* row = &kStepBackSpriteBitmap[(size_t)y * kStepBackSpriteRowBytes];
        for (uint8_t bx = 0; bx < kStepBackSpriteRowBytes; bx++) {
            uint8_t byte = pgm_read_byte(&row[bx]);
            if (!byte) continue;
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (byte & (0x80 >> bit)) {
                    int16_t sx = (int16_t)(bx * 8 + bit);
                    // Flip X so text reads correctly after game→matrix rotation (mx=gy, my=gx).
                    int16_t gx = gx0 + (int16_t)(kStepBackSpriteWidth - 1 - sx);
                    drawPixel(gx, gy0 + (int16_t)y, color);
                }
            }
        }
    }
}

// Draw centered at (cx, cy) in portrait game coords.
template <typename DrawPixelFn>
inline void drawStepBackSpriteCentered(int16_t cx, int16_t cy, const CRGB& color, DrawPixelFn&& drawPixel) {
    int16_t gx0 = cx - (int16_t)kStepBackSpriteWidth / 2;
    int16_t gy0 = cy - (int16_t)kStepBackSpriteHeight / 2;
    drawStepBackSprite(gx0, gy0, color, drawPixel);
}

// Dim the framebuffer and draw the proximity warning centered in the play area.
template <typename DrawPixelFn>
inline void drawStepBackProximityWarning(uint16_t playWidth, uint16_t playHeight, Matrix* matrix,
                                         DrawPixelFn&& drawPixel) {
    matrix->background->dim(160);
    drawStepBackSpriteCentered((int16_t)(playWidth / 2), (int16_t)(playHeight / 2), CRGB(255, 40, 30), drawPixel);
}
