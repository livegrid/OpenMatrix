#pragma once

#include <Arduino.h>
#include "Vector.h"
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
  
  virtual void setRotation(uint8_t newRotation) = 0;
  virtual void rotate90() = 0;
  virtual void clearScreen() = 0;

  virtual void update() = 0;

  GFX_Layer* background = nullptr;
  GFX_Layer* foreground = nullptr;
  GFX_LayerCompositor* gfx_compositor = nullptr;
};