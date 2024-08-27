#include "OMatrix.h"

OMatrix::OMatrix() {
  fontSize = 2;
  rotation = 0;

  HUB75_I2S_CFG mxconfig(
      PANEL_RES_X,  // module width
      PANEL_RES_Y,  // module height
      PANEL_CHAIN,  // chain length
      _pins         // pin mapping
  );
#ifdef DOUBLE_BUFFER
  mxconfig.double_buff = true;
#endif
  matrix = new MatrixPanel_I2S_DMA(mxconfig);
  matrix->begin();
  matrix->setBrightness8(brightness);  // 0-255
  matrix->clearScreen();

  background = new GFX_Layer(PANEL_RES_X, PANEL_RES_Y, 
    [this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        matrix->drawPixelRGB888(x, y, r, g, b);
    });

  foreground = new GFX_Layer(PANEL_RES_X, PANEL_RES_Y, 
    [this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        matrix->drawPixelRGB888(x, y, r, g, b);
    });

  gfx_compositor = new GFX_LayerCompositor([this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        matrix->drawPixelRGB888(x, y, r, g, b);
    });
}

void OMatrix::init() {
  // do nothing
}

void OMatrix::setBrightness(uint8_t newBrightness) {
  if (newBrightness > 255) {
    brightness = 255;
  } else if (newBrightness < 0) {
    brightness = 0;
  } else {
    brightness = newBrightness;
  }
  matrix->setBrightness8(brightness);
}

uint8_t OMatrix::getBrightness() const {
  return brightness;
}

uint8_t OMatrix::getXResolution() {
  return PANEL_RES_X;
}

uint8_t OMatrix::getYResolution() {
  return PANEL_RES_Y;
}

void OMatrix::setRotation(uint8_t newRotation) {
  if ((newRotation < 4) && (newRotation != rotation)) {
    rotation = newRotation;
    matrix->setRotation(rotation);
  }
}

void OMatrix::rotate90() {
  if (++rotation > 3) rotation = 0;
  matrix->setRotation(rotation);
}

void OMatrix::update() {
#ifdef DOUBLE_BUFFER
  matrix->flipDMABuffer();
  // matrix->clearScreen();
#endif
}

void OMatrix::clearScreen() {
  matrix->clearScreen();
}