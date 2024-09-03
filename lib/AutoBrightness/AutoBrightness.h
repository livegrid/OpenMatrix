#ifndef AUTOBRIGHTNESS_H
#define AUTOBRIGHTNESS_H

#include <hp_BH1750.h> 
#include "Matrix.h"

class AutoBrightness {
private:
  hp_BH1750 BH1750;
  bool sensorWorking;
  float lux;
  uint16_t minLux;
  uint16_t maxLux;
  uint8_t minBrightness;
  uint8_t maxBrightness;
  Matrix* matrix;

public:
    AutoBrightness(Matrix* matrix, uint16_t minLux = 30, uint16_t maxLux = 600, uint8_t minBrightness = 30, uint8_t maxBrightness = 255);

    void init();
    void updateSensorValues();
    float getLux();
    uint8_t matrixBrightness();
};

#endif