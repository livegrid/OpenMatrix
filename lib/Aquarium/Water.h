#ifndef WATER_H
#define WATER_H

#include <FastNoise.h> 
#include <Matrix.h>

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
  static const size_t rowsPerUpdate = 8;
  size_t totalRows = matrix->getYResolution();

  uint8_t scale = 20;
  float simplexSpeed = .002;

 public:
  Water(Matrix* matrix): matrix(matrix) {
    updateBuffer = new CRGB*[matrix->getYResolution()];
    for (size_t i = 0; i < matrix->getYResolution(); i++) {
      updateBuffer[i] = new CRGB[matrix->getXResolution()];
    }
  }

  ~Water() {
    // Clean up updateBuffer
    for (size_t i = 0; i < matrix->getYResolution(); i++) {
      delete[] updateBuffer[i];
    }
    delete[] updateBuffer;
  }


  void update(long temperature = 25) {

    // If we've filled the entire buffer, update the matrix background
    if (currentRow >= totalRows) {
      for (size_t i = 0; i < totalRows; ++i) {
        for (size_t j = 0; j < matrix->getXResolution(); ++j) {
          matrix->background->drawPixel(j, i, updateBuffer[i][j]);
        }
      }
      currentRow = 0;  // Reset for the next cycle
      return;
    }

    uint8_t limitTemperature = constrain(temperature, 0, 50);
    uint8_t colorIndex = map(limitTemperature, 0, 50, 0, 245);
    simplexColor = ColorFromPalette(palette, colorIndex);
    
    // Update a portion of the buffer
    for (size_t row = currentRow; row < currentRow + rowsPerUpdate && row < totalRows; ++row) {
      for (size_t col = 0; col < matrix->getXResolution(); ++col) {
        uint8_t noiseFactor = inoise8(col * scale, row * scale, (millis() * simplexSpeed));
        CRGB color = simplexColor;
        color.nscale8(noiseFactor);
        updateBuffer[row][col] = color;
      }
    }

    currentRow += rowsPerUpdate;    
  }
};

#endif
