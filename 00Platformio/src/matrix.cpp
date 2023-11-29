#include <matrix.h>
#include <Adafruit_GFX.h>
#include "GFX_fonts\Font4x5Fixed.h"
#include "GFX_fonts\Font4x7Fixed.h"
#include "GFX_fonts\Font5x5Fixed.h"
#include "GFX_fonts\Font5x7Fixed.h"

Matrix::Matrix() {
    // Default values for the 
    panelResX = 64;
    panelResY = 32;

    numRows = 2;
    numCols = 1;
    panelChain = numRows*numCols;

    // Check PANEL_CHAIN_TYPE in ESP32-VirtualMatrixPanel-I2S-DMA.h for more information
    panelChainType = CHAIN_BOTTOM_RIGHT_UP;
    numRows = 2;

    displayOffset = 2;

    virtualPanelWidth = panelResX * numCols;
    virtualPanelHeight = panelResY * numRows;

    globalBrightness = 150;
    globalRotation = 0;
}


/**
 * @brief Start a new virtual matrix object
 * 
 * @param doubleBuffer Do we need a double-buffer, should be avoided if possible (takes double memory)
 * @return true Memory allocation successful
 * @return false Memory Allocation failed
 */
bool Matrix::startMatrix(bool doubleBuffer) {
  HUB75_I2S_CFG mxconfig(
      panelResX,  // module width
      panelResY,  // module height
      panelChain   // chain length
  );
  mxconfig.double_buff = doubleBuffer;
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);

  if (not dma_display->begin()) {
    if (DEBUG_LEVEL > 0)
      Serial.println("DMA memory allocation failed");
    return false;
  }
  
  dma_display->setBrightness8(globalBrightness);
  virtualDisp = new VirtualMatrixPanel((*dma_display), numRows, numCols, panelResX, panelResY, panelChainType);
  
  virtualDisp->setRotation(globalRotation);

  virtualDisp->fillScreen(virtualDisp->color565(0, 0, 0));
  virtualDisp->setTextWrap(false);
  virtualDisp->setTextColor(virtualDisp->color565(255, 255, 255));
  virtualDisp->setFont(&Font4x7Fixed);

  if(DEBUG_LEVEL > 1) {
    Serial.println("DMA Display created");
  }
  return true;
}

int Matrix::getPanelSizeX() {
  return (virtualDisp->width());
}

int Matrix::getPanelSizeY() {
  return (virtualDisp->height());
}

void Matrix::clearScreen() {
  virtualDisp->fillScreen(virtualDisp->color444(0, 0, 0));
}

void Matrix::setBrightness(int brightness) {
  if (brightness > 0 && brightness < 255) {
    globalBrightness = brightness;
    dma_display->setBrightness8(globalBrightness);  // 0-255
  } else if(DEBUG_LEVEL > 0){
    Serial.println("Invalid brightness");
  }
}

void Matrix::setCursor(int x, int y) {
  virtualDisp->setCursor(x, y);
}

void Matrix::newLine() {
  if (fontSize == 1 || fontSize == 3)
    virtualDisp->setCursor(displayOffset, (virtualDisp->getCursorY() + 6));
  else
    virtualDisp->setCursor(displayOffset, (virtualDisp->getCursorY() + 8));
}

int Matrix::getOffset() {
  return displayOffset;
}

void Matrix::drawText(int text) {
  virtualDisp->print(text);
}

void Matrix::drawText(float text) {
  virtualDisp->print(text);
}

void Matrix::drawText(unsigned long text) {
  virtualDisp->print(text);
}

void Matrix::drawText(const char *text) {
  virtualDisp->print(text);
}


/**
 * Draws a sprite on the virtual display.
 *
 * @brief Draws a sprite on the virtual display at the specified position and size.
 *
 * @note The sprite is represented by an array of uint8_t values, where each group of three values represents the RGB color of a pixel.
 *
 * @param posX The x-coordinate of the top-left corner of the sprite on the display.
 * @param posY The y-coordinate of the top-left corner of the sprite on the display.
 * @param sizeX The width of the sprite.
 * @param sizeY The height of the sprite.
 * @param sprite A pointer to the array of uint8_t values representing the sprite.
 * @param transparent A boolean value indicating whether transparent pixels should be skipped.
 */
void Matrix::drawSprite(int posX, int posY, int sizeX, int sizeY, const uint8_t *sprite, bool transparent) {
  int k = 0;
  for (int i = posY; i < sizeY + posY; i++) {
    for (int j = posX; j < sizeX + posX; j++) {
      if (sprite[k] != 0 || sprite[k + 1] != 0 || sprite[k + 2] != 0 || !transparent) {
        virtualDisp->drawPixelRGB888(j, i, sprite[k], sprite[k + 1], sprite[k + 2]);
      }
      k += 3;
    }
  }
}

/**
 * Changes the font size of the virtual display.
 *
 * @brief Changes the font size of the virtual display to the specified value.
 *
 * @param font The font size to set. Valid values are 1, 2, 3, and 4.
 */

void Matrix::changeFont(int font) {
  fontSize = font;
  switch (fontSize) {
    case 1:
      virtualDisp->setFont(&Font4x5Fixed);
      break;
    case 2:
      virtualDisp->setFont(&Font4x7Fixed);
      break;
    case 3:
      virtualDisp->setFont(&Font5x5Fixed);
      break;
    case 4:
      virtualDisp->setFont(&Font5x7Fixed);
      break;
  }
}

int Matrix::getBrightness() {
  return globalBrightness;
}

void Matrix::rotate90() {
  if (++globalRotation > 3) globalRotation = 0;
  virtualDisp->setRotation(globalRotation);
}

void Matrix::updateDisplay() {
  virtualDisp->flipDMABuffer();
}
