#ifndef WATER_H
#define WATER_H

#include "FastNoise.h"
#include <Matrix.h>
#include "AquariumSettings.h"

class Water {
  Matrix* matrix = nullptr;

  CRGB simplexColor = CRGB(0, 100, 100);
  // One hand-picked color per slot across 10–35°C. Teal is held at the
  // comfortable point, and the last slot is full red at 34–35°C.
  CRGBPalette16 palette = CRGBPalette16(
      CRGB(0, 28, 72),    // 10°C deep blue
      CRGB(0, 32, 80),
      CRGB(0, 40, 88),
      CRGB(0, 52, 96),
      CRGB(0, 64, 96),
      CRGB(0, 84, 92),
      CRGB(0, 96, 86),
      CRGB(0, 100, 80),   // 21°C anchor
      CRGB(0, 100, 80),   // keep the anchor broad
      CRGB(12, 96, 72),   // first step off teal
      CRGB(28, 88, 56),
      CRGB(56, 76, 36),
      CRGB(80, 60, 20),
      CRGB(96, 36, 12),
      CRGB(100, 16, 8),   // deep red
      CRGB(100, 0, 0)     // 34–35°C full red
  );

  FastNoiseLite noise;
  size_t currentRow = 0;
  static const size_t rowsPerUpdate = 4;
  size_t totalRows;
  size_t totalCols;

  uint8_t scale = 5;
  float simplexSpeed = .002f;
  static constexpr uint8_t kOverallBrightness = 150;

 public:
  Water(Matrix* matrix): matrix(matrix) {
    totalRows = matrix->getYResolution();
    totalCols = matrix->getXResolution();
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFrequency(0.01f);
  }

  void update(long temperature = 25) {
    if (currentRow >= totalRows) {
      currentRow = 0;
      return;
    }

    // The palette blend wraps the last slot back toward slot 0, so 34–35°C
    // was mixing cold blue into the red and going dim. Hold the endpoints.
    if (temperature <= 10) {
      simplexColor = palette[0];
    } else if (temperature >= 34) {
      simplexColor = palette[15];
    } else {
      uint8_t colorIndex = map(temperature, 10, 35, 0, 255);
      colorIndex = min<uint8_t>(colorIndex, 235);
      simplexColor = ColorFromPalette(palette, colorIndex, 255, LINEARBLEND);
    }

    float timeZ = (float)(millis() * simplexSpeed);

    // God ray positions — computed once per batch, very slow drift
    float rayTime = (float)millis() * 0.000030f;
    float W = (float)totalCols;
    float rayX[3] = {
      W * 0.22f + sinf(rayTime * 1.00f)         * W * 0.09f,
      W * 0.52f + sinf(rayTime * 0.71f + 2.09f) * W * 0.09f,
      W * 0.80f + sinf(rayTime * 0.89f + 4.27f) * W * 0.09f,
    };
    float rayHalfW = W * 0.05f; // soft, wide beams

    // Small stack buffer for the current row batch only (rowsPerUpdate * totalCols floats)
    // For 192 cols × 4 rows = 768 floats = 3 KB — well within the display task stack
    float batchNoise[rowsPerUpdate * 256];
    size_t batchCols = totalCols < 256 ? totalCols : 256;

    size_t endRow = currentRow + rowsPerUpdate < totalRows
                    ? currentRow + rowsPerUpdate : totalRows;
    size_t batchRows = endRow - currentRow;

    for (size_t by = 0; by < batchRows; ++by) {
      for (size_t bx = 0; bx < batchCols; ++bx) {
        float nx = (float)(bx * scale);
        float ny = (float)((currentRow + by) * scale);
        batchNoise[bx + by * batchCols] = noise.GetNoise(nx, ny, timeZ);
      }
    }

    for (size_t row = currentRow; row < endRow; ++row) {
      size_t batchRow = row - currentRow;

      // Depth factor: 0.0 at top → 1.0 at bottom
      float depthT = (float)row / (float)(totalRows - 1);

      uint8_t depthScale = (uint8_t)(255.0f * (1.0f - depthT * 0.36f));

      float rayDepthFade = 1.0f - depthT * 1.15f;
      if (rayDepthFade < 0.0f) rayDepthFade = 0.0f;

      for (size_t col = 0; col < totalCols; ++col) {
        float n = batchNoise[col + batchRow * batchCols];
        float nt = (n + 1.0f) * 0.5f;
        uint8_t noiseFactor = (uint8_t)(78.0f + nt * 177.0f);
        CRGB color = simplexColor;
        color.nscale8(noiseFactor);

        // Depth attenuation
        color.nscale8(depthScale);

        // God ray contribution — quadratic falloff per ray, additive cyan-white light
        if (rayDepthFade > 0.0f) {
          float rayIntensity = 0.0f;
          float x = (float)col;
          for (int r = 0; r < 3; r++) {
            float t = fabsf(x - rayX[r]) / rayHalfW;
            if (t < 2.5f) rayIntensity += 1.0f - t * t * 0.16f; // smooth falloff
          }
          if (rayIntensity > 1.0f) rayIntensity = 1.0f;
          rayIntensity *= rayDepthFade;

          uint8_t boost = (uint8_t)(rayIntensity * 70.0f);
          color.r = qadd8(color.r, (uint8_t)(boost * 5 / 10));
          color.g = qadd8(color.g, (uint8_t)(boost * 8 / 10));
          color.b = qadd8(color.b, boost);
        }

        color.nscale8(kOverallBrightness);
        matrix->background->drawPixel((uint16_t)col, (uint16_t)row, color);
      }
    }

    currentRow += rowsPerUpdate;
  }
};

#endif
