#ifndef COLORPALETTE_H
#define COLORPALETTE_H

#include <Arduino.h>
#include <vector>


class ColorPalette {
 public:
  std::vector<CHSV> colorsHSV;
  std::vector<CRGB> colors;
  
 private:
  // Cached values to avoid redundant HSV→RGB conversions
  uint8_t lastSat = 255;
  uint8_t lastVal = 255;
  
 public:
  ColorPalette(uint8_t size, bool stripesEnabled = false) {
    
    colorsHSV.reserve(size);
    colors.reserve(size);
    uint8_t baseHue = random(0, 256);  // Random starting hue
    uint8_t hueStep = random(5, 31);   // Random hue increment between 5 and 30

    for (int i = 0; i < size; ++i) {
      uint8_t hue = (baseHue + i * hueStep) % 256;  // Wrap around at 256
      colorsHSV.push_back(CHSV(hue, 130, 255));
      CRGB rgbColor;
      hsv2rgb_rainbow(colorsHSV[i], rgbColor);
      colors.push_back(rgbColor);
    }

    if(stripesEnabled) {
      applyStripes();
    }
  }

  std::vector<CRGB> getColors() const {
    return colors;
  }

  const std::vector<CHSV>& getColorsHSV() const {
    return colorsHSV;
  }

  void setColors(const std::vector<CHSV>& newColors) {
    colorsHSV.clear();
    colors.clear();
    colorsHSV.reserve(newColors.size());
    colors.reserve(newColors.size());
    colorsHSV = newColors;
    // Ensure RGB buffer is sized before writing into it in updateRGB()
    colors.resize(colorsHSV.size());
    updateRGB();
  }

  void updateRGB() {
    for (size_t i = 0; i < colorsHSV.size(); i++) {
      hsv2rgb_rainbow(colorsHSV[i], colors[i]);
    }
  }

  void adjustColorByAgeAndHealth(float age, float health) {
    // Age adjustment
    float ageFactor = 1.0f;
    if (age >= AGE_ADULT) {
      ageFactor = 1.0f - ((age - AGE_ADULT) / (AGE_DEAD - AGE_ADULT)) * 0.5f;
    }

    // Calculate new sat/val values
    uint8_t newSat = static_cast<uint8_t>(115 * health);
    uint8_t newVal = static_cast<uint8_t>(255 * ageFactor);
    
    // Only update if values actually changed (avoids expensive HSV→RGB conversion)
    if (newSat == lastSat && newVal == lastVal) {
      return;
    }
    
    lastSat = newSat;
    lastVal = newVal;

    for (auto& hsvColor : colorsHSV) {
      hsvColor.sat = newSat;
      hsvColor.val = newVal;
    }
    updateRGB();
  }

 private:
  void applyStripes() {
    int swapType = random(0, 4);
    for (size_t i = 0; i < colorsHSV.size(); i++) {
      if (i % 2 == 1) {
        switch (swapType) {
          case 0:
            colorsHSV[i].hue = (colorsHSV[i].hue + 85) % 256;  // Shift hue by 1/3
            break;
          case 1:
            colorsHSV[i].hue = (colorsHSV[i].hue + 170) % 256;  // Shift hue by 2/3
            break;
        }
      }
    }
  }
};

#endif  // COLORPALETTE_H
