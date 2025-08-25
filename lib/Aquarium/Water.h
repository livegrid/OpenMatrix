#ifndef WATER_H
#define WATER_H

#include <FastNoise.h> 
#include <Matrix.h>
#include <math.h>

class Water {
private:
  Matrix* matrix = nullptr;

  CRGB simplexColor = CRGB(0, 100, 100);
  CRGBPalette16 palette;

  CRGB** updateBuffer = nullptr;
  size_t currentRow = 0;
  static const size_t rowsPerUpdate = 8;
  size_t totalRows = matrix->getYResolution();

  uint8_t scale = 20;
  float simplexSpeed = .002;

public:
  Water(Matrix* matrix): matrix(matrix) {
    // Initialize palette with water temperature colors
    palette = CRGBPalette16(
      CRGB(0, 28, 72),   // 10°C deep blue (not black)
      CRGB(0, 32, 80),
      CRGB(0, 40, 88),
      CRGB(0, 52, 96),
      CRGB(0, 64, 96),
      CRGB(0, 84, 92),
      CRGB(0, 96, 86),
      CRGB(0,100, 80),   // 21°C anchor
      CRGB(0,100, 80),   // keep anchor broad
      CRGB(12, 96, 72),  // gentle warm-up from anchor
      CRGB(28, 88, 56),
      CRGB(56, 76, 36),
      CRGB(80, 60, 20),
      CRGB(96, 36, 12),
      CRGB(100,16,  8),  // deep red but not full
      CRGB(100, 0,  0)   // 34–35°C bright red
    );
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

    uint8_t limitTemperature = constrain(temperature, 10, 35);
    // Delay bright red until ~34°C by compressing 32–35°C into the final palette sector
    uint8_t colorIndex = map(limitTemperature, 10, 35, 0, 255);
    if (limitTemperature < 34) {
      // cap index before full red for temps <34°C
      colorIndex = min<uint8_t>(colorIndex, 235);
    }
    simplexColor = ColorFromPalette(palette, colorIndex, 255, LINEARBLEND);
    
    // Update a portion of the buffer
    for (size_t row = currentRow; row < currentRow + rowsPerUpdate && row < totalRows; ++row) {
      for (size_t col = 0; col < matrix->getXResolution(); ++col) {
        uint8_t noiseFactor = inoise8(col * scale, row * scale, (millis() * simplexSpeed));
        if (noiseFactor < 96) noiseFactor = 96; // keep cold from going near-black
        CRGB color = simplexColor;
        color.nscale8(noiseFactor);
        updateBuffer[row][col] = color;
      }
    }

    currentRow += rowsPerUpdate;    
  }
};

#endif