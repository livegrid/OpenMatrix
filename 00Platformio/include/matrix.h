#ifndef MATRIX_H
#define MATRIX_H

#include <common.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
#include <Adafruit_GFX.h>

class Matrix {
  private:
  MatrixPanel_I2S_DMA *dma_display;
  VirtualMatrixPanel *virtualDisp;

  uint8_t panelResX;
  uint8_t panelResY;

  uint8_t numRows;
  uint8_t numCols;
  uint8_t panelChain;

  // Check PANEL_CHAIN_TYPE in ESP32-VirtualMatrixPanel-I2S-DMA.h for more information
  PANEL_CHAIN_TYPE panelChainType;

  uint8_t displayOffset;

  uint16_t virtualPanelWidth;
  uint16_t virtualPanelHeight;

  int globalBrightness;
  int globalRotation;
  uint8_t fontSize;

  public:
  Matrix();

  bool startMatrix(bool doubleBuffer = false);

  int getPanelSizeX();
  int getPanelSizeY();

  void clearScreen();
  void setBrightness(int);
  void setCursor(int, int);
  void newLine();
  int getOffset();

  void drawText(int);
  void drawText(float);
  void drawText(unsigned long);
  void drawText(const char *);
  void changeFont(int);

  void drawSprite(int , int , int , int , const uint8_t *, bool transparent);
  
  int getBrightness();
  void rotate90();
  void updateDisplay();
};

#endif