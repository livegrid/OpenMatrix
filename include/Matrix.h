#ifndef MATRIX_H
#define MATRIX_H

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

  // MatrixPanel_I2S_DMA matrix;
  MatrixPanel_I2S_DMA *matrix = nullptr;

  uint8_t fontSize = 2;
  uint8_t matrixRotation = 0;

 public:
 #if USE_CRGB_ARRAY
  CRGB leds[PANEL_RES_X * PANEL_RES_Y];
 #endif
 
  Matrix() {
    HUB75_I2S_CFG mxconfig(
        PANEL_RES_X,  // module width
        PANEL_RES_Y,  // module height
        PANEL_CHAIN,  // chain length
        _pins         // pin mapping
    );
    // double_buffer = double_buff;
    #ifdef DOUBLE_BUFFER
      mxconfig.double_buff = true;
    #endif
    // mxconfig.driver = HUB75_I2S_CFG::FM6124;
    matrix = new MatrixPanel_I2S_DMA(mxconfig);
    matrix->begin();
    matrix->setBrightness8(matrixBrightness);  // 0-255
    matrix->setRotation(2);
    matrix->clearScreen();
  }

  void setBrightness(uint8_t newBrightness) {
    if (newBrightness > 255) {
      matrixBrightness = 255;
    } else if (newBrightness < 0) {
      matrixBrightness = 0;
    } else {
      matrixBrightness = newBrightness;
    }
    matrix->setBrightness8(matrixBrightness);
  }

  uint8_t getBrightness() const {
    return matrixBrightness;
  }

  uint8_t getXResolution() {
    return PANEL_RES_X;
  }

  uint8_t getYResolution() {
    return PANEL_RES_Y;
  }

  void drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    matrix->drawPixel(x, y, matrix->color565(r, g, b));
  }

  void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t r, uint8_t g, uint8_t b) {
    matrix->drawLine(x1, y1, x2, y2, matrix->color565(r, g, b));
  }

  void drawCircleHelper(int16_t x, int16_t y, int16_t radius, uint8_t corners, int16_t delta, uint8_t r, uint8_t g, uint8_t b) {
    matrix->fillCircleHelper(x, y, radius, corners, delta, matrix->color565(r, g, b));
  }

  void drawCircle(int16_t x, int16_t y, int16_t radius, bool fill, uint8_t r, uint8_t g, uint8_t b) {
    if (fill) {
      matrix->fillCircle(x, y, radius, matrix->color565(r, g, b));
    } else {
      matrix->drawCircle(x, y, radius, matrix->color565(r, g, b));
    }
  }

  void drawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t r, uint8_t g, uint8_t b) {
    matrix->fillRect(x, y, width, height, matrix->color565(r, g, b));
  }

  void drawEllipse(int16_t x, int16_t y, int16_t rad, int16_t length, float angle, uint8_t r, uint8_t g, uint8_t b) {
    // matrix->fillEllipse(x, y, rad, length, angle, r, g, b);
  }

  void drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t r, uint8_t g, uint8_t b) {
    // matrix->fillTriangle(x1, y1, x2, y2, x3, y3, r, g, b);
    // matrix->fillTriangle(x1, y1, x2, y2, x3, y3, matrix->color565(r, g, b));
  }

  void fillNoise() {
    unsigned long currentTime = millis();
    fill_raw_noise8(
      (uint8_t*)leds,  // Cast CRGB array to uint8_t*
      PANEL_RES_X * PANEL_RES_Y,  // Total number of color components (R, G, B for each LED)
      1,    // octaves
      0,    // x
      32,   // scalex
      currentTime / 20  // time
    );
  }

  void setFont(int val) {
    fontSize = val;
    switch (fontSize) {
      case 1:
        matrix->setFont(&Font4x5Fixed);
        break;
      case 2:
        matrix->setFont(&Font4x7Fixed);
        break;
      case 3:
        matrix->setFont(&Font5x5Fixed);
        break;
      case 4:
        matrix->setFont(&Font5x7Fixed);
        break;
    }
  }

  void setCursor(int x, int y) {
    matrix->setCursor(x, y);
  }

  void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
    matrix->setTextColor(matrix->color565(r, g, b));
  }

  template <typename T>
  void drawText(const T& text) {
    if constexpr (std::is_same_v<T, std::string>) {
      matrix->print(text.c_str());
    } else {
      matrix->print(text);
    }
  }

  template <typename... Args>
  void drawTextf(const char* format, Args... args) {
    matrix->printf(format, args...);
  }

  template <typename T>
  void drawText(const T& text, uint8_t r, uint8_t g, uint8_t b) {
    matrix->setTextColor(matrix->color565(r, g, b));
    if constexpr (std::is_same_v<T, std::string>) {
      matrix->print(text.c_str());
    } else {
      matrix->print(text);
    }
  }

  void resetCursor() {
    if (fontSize == 1 || fontSize == 3)
      matrix->setCursor(OFFSET, 6);
    else
      matrix->setCursor(OFFSET, 8);
  }

  void newLine() {
    if (fontSize == 1 || fontSize == 3)
      matrix->setCursor(OFFSET, (matrix->getCursorY() + 6));
    else
      matrix->setCursor(OFFSET, (matrix->getCursorY() + 8));
  }

  int getOffset() {
    return (OFFSET);
  }

  void setRotation(uint8_t newRotation) {
    if ((newRotation < 4) && (newRotation != matrixRotation)) {
      matrixRotation = newRotation;
      matrix->setRotation(matrixRotation);
    }
  }

  void rotate90() {
    if (++matrixRotation > 3) matrixRotation = 0;
    matrix->setRotation(matrixRotation);
  }

  void fillScreen(uint8_t r, uint8_t g, uint8_t b) {
    matrix->fillScreen(matrix->color565(r, g, b));
  }

#if USE_CRGB_ARRAY
  void fillDMAFromCRGBArray() {
    for (int y = 0; y < PANEL_RES_Y; y++) {
      for (int x = 0; x < PANEL_RES_X; x++) {
        int index = y * PANEL_RES_X + x;
        const CRGB &color = leds[index];
        matrix->drawPixelRGB888(x, y, color.r, color.g, color.b);
      }
    }
  }
#endif

  void update() {
    #if USE_CRGB_ARRAY
      fillDMAFromCRGBArray();
    #endif
    #ifdef DOUBLE_BUFFER
      matrix->flipDMABuffer();
      // matrix->clearScreen();
    #endif
  }

  void clearScreen() {
    matrix->clearScreen();
  }

  void drawSprite(int8_t posX, int8_t posY, const uint8_t *sprite, uint8_t sizeX, uint8_t sizeY, float scale = 1) {
    // int centerX = sizeX / 2;
    // int centerY = sizeY / 2;
    // int angleIndex = (int)(angle * 180 / PI) % TABLE_SIZE;

    for (int i = 0; i < sizeY; i++) {
      for (int j = 0; j < sizeX; j++) {
        int k = (i * sizeX + j) * 3;
        if (sprite[k] != 0 || sprite[k + 1] != 0 || sprite[k + 2] != 0) {
          // int translatedX = j - centerX;
          // int translatedY = i - centerY;
          // int scaledX = scale * rotatedX;
          // int scaledY = scale * rotatedY;
          // int newX = translatedX + centerX + posX;
          // int newY = translatedY + centerY + posY;
          matrix->drawPixelRGB888(j + posX, i + posY, sprite[k], sprite[k + 1], sprite[k + 2]);
          // matrix->drawPixel(j+posX, i+posY, sprite[(i * sizeX + j) * 3]);
          // matrix->drawPixel(j+posX, i+posY, pgm_read_word(&sprite[(k) * 3]));
        }
      }
    }
  }

  void drawSpriteSpecial(int8_t posX, int8_t posY, const ImageData *spriteSheet, uint8_t size, float angle, float r, float g, float b) {
    if (spriteSheet[size].width == spriteSheet[size].height) {
      int centerX = spriteSheet[size].width / 2;
      int centerY = spriteSheet[size].height / 2;
      for (int i = 0; i < spriteSheet[size].height; i++) {
        for (int j = 0; j < spriteSheet[size].width; j++) {
          int k = (i * spriteSheet[size].width + j);
          if (spriteSheet[size].data[k] > 50) {
            matrix->drawPixelRGB888(posX + j - centerX, posY + i - centerY, spriteSheet[size].data[k]*r, spriteSheet[size].data[k]*g, spriteSheet[size].data[k]*b);
          }
        }
      }
    } else {
      // Adjust angle to map correctly to sprite indices
      angle += PI / 32;

      // Normalize angle to [0, 2*PI)
      angle = fmod(angle, 2 * PI);  // Normalize using modulo
      if (angle < 0) {
        angle += 2 * PI;  // Adjust if the result is negative
      }

      // Each sprite covers PI/4 radians. Determine which sprite to use.
      int spriteIndex = static_cast<int>(angle / (PI / 10));

      // Determine the transformation needed
      bool mirrorHorizontal = false;
      bool mirrorVertical = false;
      bool rotate90 = false;

      // Adjust for mirroring and additional rotation beyond the first 90 degrees
      if (spriteIndex >= 15) {
        rotate90 = true;
        mirrorHorizontal = true;
        spriteIndex -= 15;
      } else if (spriteIndex >= 10) {
        mirrorVertical = true;
        mirrorHorizontal = true;
        spriteIndex -= 10;
      } else if (spriteIndex >= 5) {
        rotate90 = true;
        spriteIndex -= 5;
      }
      // Calculate the starting Y position of the sprite in the sprite sheet
      int startY = spriteIndex * spriteSheet[size].height * spriteSheet[size].width;
      int centerX = spriteSheet[size].width / 2;
      int centerY = spriteSheet[size].height / 2;

      for (int i = 0; i < spriteSheet[size].height; i++) {
        for (int j = 0; j < spriteSheet[size].width; j++) {
          int drawX = posX - centerX + j;
          int drawY = posY - centerY + i;
          if (mirrorHorizontal) {
            drawX = posX + (spriteSheet[size].width - 1 - j) - centerX;  // Adjust for horizontal mirroring
          }
          if (mirrorVertical) {
            drawY = posY + (spriteSheet[size].height - 1 - i) - centerY;  // Adjust for vertical mirroring
          }
          if (rotate90) {
            int temp = drawX;
            drawX = drawY;
            drawY = temp;
            drawX = posX + (spriteSheet[size].height - 1 - i) - centerY;  // Adjusting for new center after rotation
            drawY = posY + j - centerX;
          }
          int k = startY + i * spriteSheet[size].width + j;
          if (spriteSheet[size].data[k] > 50) {
            matrix->drawPixelRGB888(drawX, drawY, spriteSheet[size].data[k], spriteSheet[size].data[k], spriteSheet[size].data[k]);
            // matrix->drawPixelRGB888(drawX, drawY, spriteSheet[size].data[k] * c2.r, spriteSheet[size].data[k] * c2.g, spriteSheet[size].data[k] * c2.b);
          }
        }
      }
    }
  }
};
// int drawX = posX + (mirrorHorizontal ? spriteWidth - 1 - j : j) - centerX;  // Apply horizontal mirroring
// int drawY = posY + (mirrorVertical ? spriteHeight - 1 - i : i) - centerY;   // Apply vertical mirroring
// if (rotate90) {
//   // Swap x and y for 90-degree rotation
//   int temp = drawX;
//   drawX = drawY;
//   drawY = temp;
// }
#endif