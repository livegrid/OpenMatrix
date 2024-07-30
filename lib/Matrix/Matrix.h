#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "GFX_fonts/Font4x5Fixed.h"
#include "GFX_fonts/Font4x7Fixed.h"
#include "GFX_fonts/Font5x5Fixed.h"
#include "GFX_fonts/Font5x7Fixed.h"
#include "MatrixSettings.h"
#include "Vector.h"
#include "Sprites.h"

class Matrix {
 private:
  HUB75_I2S_CFG::i2s_pins _pins = {RL1, GL1, BL1, RL2, GL2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
  MatrixPanel_I2S_DMA* matrix = nullptr;
  uint8_t fontSize = 2;
  uint8_t matrixRotation = 0;

 public:
#if USE_CRGB_ARRAY
  CRGB leds[PANEL_RES_X * PANEL_RES_Y];
#endif

  Matrix();
  void setBrightness(uint8_t newBrightness);
  uint8_t getBrightness() const;
  uint8_t getXResolution();
  uint8_t getYResolution();
  void drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
  void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t r, uint8_t g, uint8_t b);
  void drawCircleHelper(int16_t x, int16_t y, int16_t radius, uint8_t corners, int16_t delta, uint8_t r, uint8_t g, uint8_t b);
  void drawCircle(int16_t x, int16_t y, int16_t radius, bool fill, uint8_t r, uint8_t g, uint8_t b);
  void drawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t r, uint8_t g, uint8_t b);
  void drawEllipse(int16_t x, int16_t y, int16_t rad, int16_t length, float angle, uint8_t r, uint8_t g, uint8_t b);
  void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t r, uint8_t g, uint8_t b);
  void fillNoise();
  void setFont(int val);
  void setCursor(int x, int y);
  void setTextColor(uint8_t r, uint8_t g, uint8_t b);

  template <typename T>
  void drawText(const T& text);

  template <typename... Args>
  void drawTextf(const char* format, Args... args);

  template <typename T>
  void drawText(const T& text, uint8_t r, uint8_t g, uint8_t b);

  void resetCursor();
  void newLine();
  int getOffset();
  void setRotation(uint8_t newRotation);
  void rotate90();
  void fillScreen(uint8_t r, uint8_t g, uint8_t b);

#if USE_CRGB_ARRAY
  void fillDMAFromCRGBArray();
#endif

  void update();
  void clearScreen();
  void drawSprite(int8_t posX, int8_t posY, const uint8_t* sprite, uint8_t sizeX, uint8_t sizeY, float scale = 1);
  void drawSpriteSpecial(int8_t posX, int8_t posY, const ImageData* spriteSheet, uint8_t size, float angle, float r, float g, float b);
};

#include "Matrix.tpp"