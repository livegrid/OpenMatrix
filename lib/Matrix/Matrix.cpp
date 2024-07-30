#include "Matrix.h"

Matrix::Matrix() {
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
  matrix->setBrightness8(matrixBrightness);  // 0-255
  matrix->setRotation(2);
  matrix->clearScreen();
}

void Matrix::setBrightness(uint8_t newBrightness) {
  if (newBrightness > 255) {
    matrixBrightness = 255;
  } else if (newBrightness < 0) {
    matrixBrightness = 0;
  } else {
    matrixBrightness = newBrightness;
  }
  matrix->setBrightness8(matrixBrightness);
}

uint8_t Matrix::getBrightness() const {
  return matrixBrightness;
}

uint8_t Matrix::getXResolution() {
  return PANEL_RES_X;
}

uint8_t Matrix::getYResolution() {
  return PANEL_RES_Y;
}

void Matrix::drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
  matrix->drawPixel(x, y, matrix->color565(r, g, b));
}

void Matrix::drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t r, uint8_t g, uint8_t b) {
  matrix->drawLine(x1, y1, x2, y2, matrix->color565(r, g, b));
}

void Matrix::drawCircleHelper(int16_t x, int16_t y, int16_t radius, uint8_t corners, int16_t delta, uint8_t r, uint8_t g, uint8_t b) {
  matrix->fillCircleHelper(x, y, radius, corners, delta, matrix->color565(r, g, b));
}

void Matrix::drawCircle(int16_t x, int16_t y, int16_t radius, bool fill, uint8_t r, uint8_t g, uint8_t b) {
  if (fill) {
    matrix->fillCircle(x, y, radius, matrix->color565(r, g, b));
  } else {
    matrix->drawCircle(x, y, radius, matrix->color565(r, g, b));
  }
}

void Matrix::drawRectangle(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t r, uint8_t g, uint8_t b) {
  matrix->fillRect(x, y, width, height, matrix->color565(r, g, b));
}

void Matrix::drawEllipse(int16_t x, int16_t y, int16_t rad, int16_t length, float angle, uint8_t r, uint8_t g, uint8_t b) {
  // matrix->fillEllipse(x, y, rad, length, angle, r, g, b);
}

void Matrix::drawTriangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3, uint8_t r, uint8_t g, uint8_t b) {
  // matrix->fillTriangle(x1, y1, x2, y2, x3, y3, r, g, b);
  // matrix->fillTriangle(x1, y1, x2, y2, x3, y3, matrix->color565(r, g, b));
}

void Matrix::fillNoise() {
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

void Matrix::setFont(int val) {
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

void Matrix::setCursor(int x, int y) {
  matrix->setCursor(x, y);
}

void Matrix::setTextColor(uint8_t r, uint8_t g, uint8_t b) {
  matrix->setTextColor(matrix->color565(r, g, b));
}

void Matrix::resetCursor() {
  if (fontSize == 1 || fontSize == 3)
    matrix->setCursor(OFFSET, 6);
  else
    matrix->setCursor(OFFSET, 8);
}

void Matrix::newLine() {
  if (fontSize == 1 || fontSize == 3)
    matrix->setCursor(OFFSET, (matrix->getCursorY() + 6));
  else
    matrix->setCursor(OFFSET, (matrix->getCursorY() + 8));
}

int Matrix::getOffset() {
  return OFFSET;
}

void Matrix::setRotation(uint8_t newRotation) {
  if ((newRotation < 4) && (newRotation != matrixRotation)) {
    matrixRotation = newRotation;
    matrix->setRotation(matrixRotation);
  }
}

void Matrix::rotate90() {
  if (++matrixRotation > 3) matrixRotation = 0;
  matrix->setRotation(matrixRotation);
}

void Matrix::fillScreen(uint8_t r, uint8_t g, uint8_t b) {
  matrix->fillScreen(matrix->color565(r, g, b));
}

#if USE_CRGB_ARRAY
void Matrix::fillDMAFromCRGBArray() {
  for (int y = 0; y < PANEL_RES_Y; y++) {
    for (int x = 0; x < PANEL_RES_X; x++) {
      int index = y * PANEL_RES_X + x;
      const CRGB &color = leds[index];
      matrix->drawPixelRGB888(x, y, color.r, color.g, color.b);
    }
  }
}
#endif

void Matrix::update() {
#ifdef DOUBLE_BUFFER
  matrix->flipDMABuffer();
  // matrix->clearScreen();
#endif
}

void Matrix::clearScreen() {
  matrix->clearScreen();
}

void Matrix::drawSprite(int8_t posX, int8_t posY, const uint8_t* sprite, uint8_t sizeX, uint8_t sizeY, float scale) {
  for (int i = 0; i < sizeY; i++) {
    for (int j = 0; j < sizeX; j++) {
      int k = (i * sizeX + j) * 3;
      if (sprite[k] != 0 || sprite[k + 1] != 0 || sprite[k + 2] != 0) {
        matrix->drawPixelRGB888(j + posX, i + posY, sprite[k], sprite[k + 1], sprite[k + 2]);
      }
    }
  }
}

void Matrix::drawSpriteSpecial(int8_t posX, int8_t posY, const ImageData* spriteSheet, uint8_t size, float angle, float r, float g, float b) {
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
    angle += PI / 32;
    angle = fmod(angle, 2 * PI);
    if (angle < 0) {
      angle += 2 * PI;
    }
    int spriteIndex = static_cast<int>(angle / (PI / 10));
    bool mirrorHorizontal = false;
    bool mirrorVertical = false;
    bool rotate90 = false;

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
    int startY = spriteIndex * spriteSheet[size].height * spriteSheet[size].width;
    int centerX = spriteSheet[size].width / 2;
    int centerY = spriteSheet[size].height / 2;

    for (int i = 0; i < spriteSheet[size].height; i++) {
      for (int j = 0; j < spriteSheet[size].width; j++) {
        int drawX = posX - centerX + j;
        int drawY = posY - centerY + i;
        if (mirrorHorizontal) {
          drawX = posX + (spriteSheet[size].width - 1 - j) - centerX;
        }
        if (mirrorVertical) {
          drawY = posY + (spriteSheet[size].height - 1 - i) - centerY;
        }
        if (rotate90) {
          int temp = drawX;
          drawX = drawY;
          drawY = temp;
          drawX = posX + (spriteSheet[size].height - 1 - i) - centerY;
          drawY = posY + j - centerX;
        }
        int k = startY + i * spriteSheet[size].width + j;
        if (spriteSheet[size].data[k] > 50) {
          matrix->drawPixelRGB888(drawX, drawY, spriteSheet[size].data[k], spriteSheet[size].data[k], spriteSheet[size].data[k]);
        }
      }
    }
  }
}
