#ifndef WATER_H
#define WATER_H

#include "FastNoise.h"
#include <Matrix.h>
#include "AquariumSettings.h"

// Define the palette at global scope
DEFINE_GRADIENT_PALETTE( waterPalette ) {
    0,    0,  20,  80,  // 10°C: Cold
  112,    0, 100,  80,  // 24°C: Normal
  176,    0, 100,  80,  // 28°C: Normal (extended)
  255,   100,   0,  20   // 35°C: Warm
};

class Water {
  Matrix* matrix = nullptr;

  CRGB simplexColor = CRGB(0, 100, 100);
  CRGBPalette16 palette = waterPalette;

  FastNoiseLite noise;
  size_t currentRow = 0;
  static const size_t rowsPerUpdate = 4;
  size_t totalRows;
  size_t totalCols;

  uint8_t scale = 5;
  float simplexSpeed = .002f;

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

    uint8_t limitTemperature = constrain(temperature, 0, 50);
    uint8_t colorIndex = map(limitTemperature, 0, 50, 0, 245);
    simplexColor = ColorFromPalette(palette, colorIndex);

    float timeZ = (float)(millis() * simplexSpeed);

#if AQUARIUM_WATER_GOD_RAYS_ENABLED
    // God ray positions — computed once per batch, very slow drift
    float rayTime = (float)millis() * 0.000030f;
    float W = (float)totalCols;
    float rayX[3] = {
      W * 0.22f + sinf(rayTime * 1.00f)         * W * 0.09f,
      W * 0.52f + sinf(rayTime * 0.71f + 2.09f) * W * 0.09f,
      W * 0.80f + sinf(rayTime * 0.89f + 4.27f) * W * 0.09f,
    };
    float rayHalfW = W * 0.05f; // soft, wide beams
#endif

    // Small stack buffer for the current row batch only (rowsPerUpdate * totalCols floats)
    // For 192 cols × 4 rows = 768 floats = 3 KB — well within the display task stack
    float batchNoise[rowsPerUpdate * 256];
    size_t batchCols = totalCols < 256 ? totalCols : 256;

    size_t endRow = currentRow + rowsPerUpdate < totalRows
                    ? currentRow + rowsPerUpdate : totalRows;
    size_t batchRows = endRow - currentRow;

    noise.FillNoise2D(batchNoise, (int)batchCols, (int)batchRows,
                     0.0f, (float)(currentRow * scale), timeZ,
                     (float)scale, (float)scale);

    for (size_t row = currentRow; row < endRow; ++row) {
      size_t batchRow = row - currentRow;

      // Depth factor: 0.0 at top → 1.0 at bottom
      float depthT = (float)row / (float)(totalRows - 1);

      // Light attenuation with depth (bright at surface, dark at floor)
      uint8_t depthScale = (uint8_t)(255.0f * (1.0f - depthT * 0.55f));

#if AQUARIUM_WATER_GOD_RAYS_ENABLED
      // God rays fade out toward the bottom (gone past ~70% depth)
      float rayDepthFade = 1.0f - depthT * 1.45f;
      if (rayDepthFade < 0.0f) rayDepthFade = 0.0f;
#endif

      for (size_t col = 0; col < totalCols; ++col) {
        float n = batchNoise[col + batchRow * batchCols];
        uint8_t noiseFactor = (uint8_t)((n + 1.0f) * 127.5f);
        CRGB color = simplexColor;
        color.nscale8(noiseFactor);

        // Depth attenuation
        color.nscale8(depthScale);

#if AQUARIUM_WATER_GOD_RAYS_ENABLED
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
#endif

        matrix->background->drawPixel((uint16_t)col, (uint16_t)row, color);
      }
    }

    currentRow += rowsPerUpdate;
  }
};

#endif
