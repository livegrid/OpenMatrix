#pragma once

#include "GFX_Layer.hpp"
#include "PanelOrientation.h"

// Stores pixels in the physical HUB75 buffer. Callers draw in upright
// coordinates; drawPixel turns those into buffer coordinates.
class AuraLayer : public GFX_Layer {
 public:
  AuraLayer(uint16_t bufferW, uint16_t bufferH,
            std::function<void(int16_t, int16_t, uint8_t, uint8_t, uint8_t)> cb)
      : GFX_Layer(bufferW, bufferH, cb),
        bufferW(bufferW),
        bufferH(bufferH),
        panelRotation(0) {}

  void setPanelRotation(uint8_t rotation) { panelRotation = rotation & 3; }

  void drawPixel(int16_t x, int16_t y, CRGB color) override {
    uint8_t visualW = 0;
    uint8_t visualH = 0;
    panelVisualSize(bufferW, bufferH, panelRotation, visualW, visualH);
    if (x < 0 || y < 0 || x >= visualW || y >= visualH) return;

    int16_t bx = 0;
    int16_t by = 0;
    panelVisualToBuffer(x, y, panelRotation, bufferW, bufferH, bx, by);
    GFX_Layer::drawPixel(bx, by, color);
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    uint8_t r = ((((color >> 11) & 0x1F) * 527) + 23) >> 6;
    uint8_t g = ((((color >> 5) & 0x3F) * 259) + 33) >> 6;
    uint8_t b = (((color & 0x1F) * 527) + 23) >> 6;
    drawPixel(x, y, CRGB(r, g, b));
  }

 private:
  uint8_t bufferW;
  uint8_t bufferH;
  uint8_t panelRotation;
};
