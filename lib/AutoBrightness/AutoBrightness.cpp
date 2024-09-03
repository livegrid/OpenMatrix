#include "AutoBrightness.h"

AutoBrightness::AutoBrightness(Matrix* matrix, uint16_t minLux, uint16_t maxLux, uint8_t minBrightness, uint8_t maxBrightness)
  : matrix(matrix), sensorWorking(false), lux(0), minLux(minLux), maxLux(maxLux), minBrightness(minBrightness), maxBrightness(maxBrightness) {
}

void AutoBrightness::init() {
  sensorWorking = BH1750.begin(BH1750_TO_GROUND);
  if (!sensorWorking) {
    log_e("No BH1750 sensor found!");           
  }
  else {
    log_i("conversion time: %dms\n", BH1750.getMtregTime());
    BH1750.start(); 
  }
}

void AutoBrightness::updateSensorValues() {
  if (sensorWorking && (BH1750.hasValue() == true)) {
    lux = BH1750.getLux();
    log_v("lux: %.2f", lux);
  }
  BH1750.start(); 
}

float AutoBrightness::getLux() {
  return lux;
}

uint8_t AutoBrightness::matrixBrightness() {
  long matrixBrightness = map(lux, minLux, maxLux, minBrightness, maxBrightness);
  matrixBrightness = constrain(matrixBrightness, minBrightness, maxBrightness);
  return uint8_t(matrixBrightness);
}