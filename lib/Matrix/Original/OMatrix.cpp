#include "OMatrix.h"
#ifdef PANEL_AURA
#include "AuraLayer.h"
#include "PanelOrientation.h"
#endif

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

  auto drawToPanel = [this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    matrix->drawPixelRGB888(x, y, r, g, b);
  };
#ifdef PANEL_AURA
  background = new AuraLayer(PANEL_RES_X, PANEL_RES_Y, drawToPanel);
  foreground = new AuraLayer(PANEL_RES_X, PANEL_RES_Y, drawToPanel);
#else
  background = new GFX_Layer(PANEL_RES_X, PANEL_RES_Y, drawToPanel);
  foreground = new GFX_Layer(PANEL_RES_X, PANEL_RES_Y, drawToPanel);
#endif

  gfx_compositor = new GFX_LayerCompositor([this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        matrix->drawPixelRGB888(x, y, r, g, b);
    });
}

void OMatrix::init() {
  // do nothing
}

void OMatrix::setBrightness(uint8_t newBrightness) {
  brightness = newBrightness;
  matrix->setBrightness8(brightness);
}

uint8_t OMatrix::getBrightness() const {
  return brightness;
}

uint8_t OMatrix::getXResolution() {
#ifdef PANEL_AURA
  uint8_t visualW = PANEL_RES_X;
  uint8_t visualH = PANEL_RES_Y;
  panelVisualSize(PANEL_RES_X, PANEL_RES_Y, rotation, visualW, visualH);
  return visualW;
#else
  return PANEL_RES_X;
#endif
}

void OMatrix::drawPixelRGB888(uint16_t x, uint16_t y, uint8_t r_data,
                              uint8_t g_data, uint8_t b_data) {
  matrix->drawPixelRGB888(x, y, r_data, g_data, b_data);
}

uint8_t OMatrix::getYResolution() {
#ifdef PANEL_AURA
  uint8_t visualW = PANEL_RES_X;
  uint8_t visualH = PANEL_RES_Y;
  panelVisualSize(PANEL_RES_X, PANEL_RES_Y, rotation, visualW, visualH);
  return visualH;
#else
  return PANEL_RES_Y;
#endif
}

uint8_t OMatrix::getBufferWidth() {
  return PANEL_RES_X;
}

uint8_t OMatrix::getBufferHeight() {
  return PANEL_RES_Y;
}

void OMatrix::setRotation(uint8_t newRotation) {
  if (newRotation > 3 || newRotation == rotation) return;
  rotation = newRotation;
#ifdef PANEL_AURA
  // Keep the HUB75 scan in native 192x64. AuraLayer remaps upright draws.
  static_cast<AuraLayer*>(background)->setPanelRotation(rotation);
  static_cast<AuraLayer*>(foreground)->setPanelRotation(rotation);
#else
  matrix->setRotation(rotation);
#endif
}

void OMatrix::rotate90() {
  setRotation((rotation + 1) & 3);
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