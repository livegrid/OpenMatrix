#pragma once

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "OMatrixSettings.h"
#include "GFX_Layer.hpp"
#include "Matrix.h"

class OMatrix : public Matrix {
 private:
  HUB75_I2S_CFG::i2s_pins _pins = {RL1, GL1, BL1, RL2, GL2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};
  MatrixPanel_I2S_DMA* matrix = nullptr;

 public:
  OMatrix();

  void init() override;

  void setBrightness(uint8_t newBrightness) override;
  uint8_t getBrightness() const override;
  uint8_t getXResolution() override;
  uint8_t getYResolution() override;
  
  void setRotation(uint8_t newRotation) override;
  void rotate90() override;
  void clearScreen() override;

  void update() override;
};