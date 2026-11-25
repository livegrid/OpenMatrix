#pragma once

#include <Arduino.h>
#include "PVector.h"
#include "GFX_Layer.hpp"

class Matrix {
 protected:
  uint8_t rotation;
  uint8_t brightness = 100;
  uint8_t fontSize;

 public:
  virtual ~Matrix() = default;

  virtual void init() = 0;

  virtual void drawPixelRGB888(uint16_t x, uint16_t y, uint8_t r_data, uint8_t g_data,
                     uint8_t b_data) = 0;

  virtual void setBrightness(uint8_t newBrightness) = 0;
  virtual uint8_t getBrightness() const = 0;
  virtual uint8_t getXResolution() = 0;
  virtual uint8_t getYResolution() = 0;
  // Physical framebuffer size. Upright resolution can differ when the panel is rotated.
  virtual uint8_t getBufferWidth() { return getXResolution(); }
  virtual uint8_t getBufferHeight() { return getYResolution(); }
  
  virtual void setRotation(uint8_t newRotation) = 0;
  virtual void rotate90() = 0;
  uint8_t getRotation() const { return rotation; }
  virtual void clearScreen() = 0;

  virtual void update() = 0;

  GFX_Layer* background = nullptr;
  GFX_Layer* foreground = nullptr;
  GFX_LayerCompositor* gfx_compositor = nullptr;
};

// GFX_Lite::fillTriangle swaps width and height when all three vertices share
// a row (y0 == y1 == y2), drawing horizontal tails/fins as a vertical spike.
// Draw that degenerate single-row span horizontally before delegating to GFX.
inline void fillTriangleSafe(GFX_Layer* layer, int16_t x0, int16_t y0,
                             int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                             CRGB color) {
  if (!layer) return;
  if (y0 == y1 && y1 == y2) {
    int16_t a = x0;
    int16_t b = x0;
    if (x1 < a) a = x1;
    else if (x1 > b) b = x1;
    if (x2 < a) a = x2;
    else if (x2 > b) b = x2;
    layer->fillRect(a, y0, b - a + 1, 1, color);
    return;
  }
  layer->fillTriangle(x0, y0, x1, y1, x2, y2, color);
}