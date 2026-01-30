#ifndef WATER_H
#define WATER_H

#include <FastNoise.h> 
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

  CRGB** updateBuffer = nullptr;
  size_t currentRow = 0;
  static const size_t rowsPerUpdate = 4;
  size_t totalRows;
  size_t totalCols;

  uint8_t scale = 20;
  float simplexSpeed = .002;

 public:
  Water(Matrix* matrix): matrix(matrix) {
    totalRows = matrix->getYResolution();
    totalCols = matrix->getXResolution();
    updateBuffer = new CRGB*[totalRows];
    for (size_t i = 0; i < totalRows; i++) {
      updateBuffer[i] = new CRGB[totalCols];
    }
  }

  ~Water() {
    // Clean up updateBuffer
    for (size_t i = 0; i < totalRows; i++) {
      delete[] updateBuffer[i];
    }
    delete[] updateBuffer;
  }

  void update(long temperature = 25) {

    // If we've filled the entire buffer, update the matrix background
    if (currentRow >= totalRows) {
      for (size_t i = 0; i < totalRows; ++i) {
        CRGB* rowBuffer = updateBuffer[i];  // Cache row pointer
        for (size_t j = 0; j < totalCols; ++j) {
          matrix->background->drawPixel(j, i, rowBuffer[j]);
        }
      }
      currentRow = 0;  // Reset for the next cycle
      return;
    }

    uint8_t limitTemperature = constrain(temperature, 0, 50);
    uint8_t colorIndex = map(limitTemperature, 0, 50, 0, 245);
    simplexColor = ColorFromPalette(palette, colorIndex);
    
    // Cache time component outside inner loop
    uint32_t timeComponent = static_cast<uint32_t>(millis() * simplexSpeed);
    
    // Update a portion of the buffer
    size_t endRow = min(currentRow + rowsPerUpdate, totalRows);
    for (size_t row = currentRow; row < endRow; ++row) {
      uint16_t scaledRow = row * scale;
      CRGB* rowBuffer = updateBuffer[row];  // Cache row pointer
      for (size_t col = 0; col < totalCols; ++col) {
        uint8_t noiseFactor = inoise8(col * scale, scaledRow, timeComponent);
        CRGB color = simplexColor;
        color.nscale8(noiseFactor);
        rowBuffer[col] = color;
      }
    }

    currentRow += rowsPerUpdate;    
  }
};

#endif
