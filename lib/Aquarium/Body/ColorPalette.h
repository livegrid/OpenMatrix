#ifndef COLORPALETTE_H
#define COLORPALETTE_H

#include <Arduino.h>
#include <vector>


class ColorPalette {
 public:
  std::vector<CHSV> colorsHSV;
  std::vector<CRGB> colors;

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
    updateRGB();
  }

  void updateRGB() {
    for (size_t i = 0; i < colorsHSV.size(); i++) {
      hsv2rgb_rainbow(colorsHSV[i], colors[i]);
    }
  }

  void adjustColorbyAge(float age) {
    for (auto& hsvColor : colorsHSV) {
      if (age >= AGE_ADULT) {
        float fadeFactor = (age - AGE_ADULT) / (AGE_DEAD - AGE_ADULT);
        hsvColor.val = static_cast<uint8_t>(255 * (1 - fadeFactor * 0.5));  // Reduce fading effect
      } else {
        hsvColor.val = 255;  // Full brightness for young fish
      }
    }
    updateRGB();
  }

  void adjustColorbyHealth(float health) {
    for (auto& hsvColor : colorsHSV) {
      hsvColor.sat = static_cast<uint8_t>(130 * health);  // Saturation ranges from 0 to 130
      hsvColor.val = static_cast<uint8_t>(255 * (0.2 + health * 0.8));  // Value ranges from 51 to 255
    }
    updateRGB();
  }

 private:
  void applyStripes() {
    int swapType = random(0, 3);
    for (size_t i = 0; i < colorsHSV.size(); i++) {
      if (i % 2 == 1) {
        switch (swapType) {
          case 0:
            colorsHSV[i].hue = (colorsHSV[i].hue + 85) % 256;  // Shift hue by 1/3
            break;
          case 1:
            colorsHSV[i].hue = (colorsHSV[i].hue + 170) % 256;  // Shift hue by 2/3
            break;
          case 2:
            colorsHSV[i].sat = 255 - colorsHSV[i].sat;  // Invert saturation
            break;
        }
      }
    }
    updateRGB();
  }
};

#endif  // COLORPALETTE_H
