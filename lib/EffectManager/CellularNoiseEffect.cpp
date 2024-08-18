#include "CellularNoiseEffect.h"

CellularNoiseEffect::CellularNoiseEffect(Matrix* m) : Effect(m) {
  noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  noise.SetFrequency(.04);
}

void CellularNoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void CellularNoiseEffect::setSpeed(float s) {
    speed = s;
}

void CellularNoiseEffect::update() {
  unsigned long currentTime = millis();
    for(float i = 0; i < m_matrix->getXResolution(); i++) {
        for(float j = 0; j < m_matrix->getYResolution(); j++) {
            int noiseNow = int((1 + noise.GetNoise(j, i, float(currentTime) * speed)) * 127.5);
            CRGB col = baseColor;
            int reducedNoise = noiseNow-50;
            if(reducedNoise > 0)
              col.nscale8(noiseNow);
            else
              col.nscale8(0);
            m_matrix->background->drawPixel(i, j, col);
        }
    }
}


const char* CellularNoiseEffect::getName() const {
    return "CellularNoise";
}