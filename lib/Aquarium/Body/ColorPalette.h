#ifndef COLORPALETTE_H
#define COLORPALETTE_H

#include <Arduino.h>
#include <vector>


class ColorPalette {
 public:
  std::vector<CRGB> colors;
  std::vector<CHSV> colorsHSV;
  bool stripes = false;
  ColorPalette(uint8_t size, bool stripesEnabled = false) {
    
    colorsHSV.reserve(size);
    uint8_t baseHue = random(0, 256);  // Random starting hue
    uint8_t hueStep = random(5, 31);   // Random hue increment between 5 and 30

    for (int i = 0; i < size; ++i) {
      uint8_t hue = (baseHue + i * hueStep) % 256;  // Wrap around at 256
      colorsHSV.push_back(CHSV(hue, 130, 255));
    }

    colors.reserve(size);
    for (const auto& hsvColor : colorsHSV) {
      CRGB rgbColor;
      hsv2rgb_rainbow(hsvColor, rgbColor);
      colors.push_back(rgbColor);
    }

    if(stripesEnabled) {
      stripes = random(0, 2) == 1;
    }

    if (stripes) {
      int swapType = random(0, 3);
      for (size_t i = 0; i < colors.size(); i++) {
        if (i % 2 == 1) {
          switch (swapType) {
            case 0:
              std::swap(colors[i].r, colors[i].g);
              break;
            case 1:
              std::swap(colors[i].r, colors[i].b);
              break;
            case 2:
              std::swap(colors[i].g, colors[i].b);
              break;
            }
          }
        }
    }
  }

  void adjustColorbyAge(float age) {
    for (size_t i = 0; i < colorsHSV.size(); ++i) {
      CHSV& hsvColor = colorsHSV[i];
      CRGB& rgbColor = colors[i];

      if (age < AGE_ADULT) {
        // Adjust saturation from 0 to 130 between AGE_CHILD and AGE_TEEN
        float saturationFactor = (age - AGE_EGG) / (AGE_ADULT - AGE_EGG);
        hsvColor.sat = static_cast<uint8_t>(130 * saturationFactor);
      }

      // Convert HSV to RGB
      hsv2rgb_rainbow(hsvColor, rgbColor);

      // Apply fadeToBlackBy for the senior stage
      if (age >= AGE_ADULT) {
        float fadeFactor = (age - AGE_ADULT) / (AGE_DEAD - AGE_ADULT);
        uint8_t fadeAmount = static_cast<uint8_t>(255 * fadeFactor);
        fadeToBlackBy(&rgbColor, 1, fadeAmount);
      }
    }
  }
};

#endif  // COLORPALETTE_H
