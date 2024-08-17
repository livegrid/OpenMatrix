#include "CellularNoiseEffect.h"

CellularNoiseEffect::CellularNoiseEffect(Matrix* m) : Effect(m) {
  noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
  noise.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
  noise.SetFrequency(.0004);
}

void CellularNoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void CellularNoiseEffect::setSpeed(float s) {
    speed = s;
}

void CellularNoiseEffect::update() {
    // uint16_t currentTimeSpeedInt = millis() * speed;
    // for(int i = 0; i < m_matrix->getXResolution(); i++) {
    //     for(int j = 0; j < m_matrix->getYResolution(); j++) {
    //         int noiseNow = int((1 + noise.GetNoise(j * scale * 10, i * scale * 10, int(currentTimeSpeedInt*speed))) / 127);
    //         CRGB col = baseColor;
    //         col.nscale8(noiseNow); // Convert float to uint8_t
    //         m_matrix->background->drawPixel(i, j, col);
    //     }
    // }
}

const char* CellularNoiseEffect::getName() const {
    return "CellularNoise";
}