#include "SimplexNoiseEffect.h"

SimplexNoiseEffect::SimplexNoiseEffect(Matrix* m) : Effect(m) {
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
  noise.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes);
  noise.SetFrequency(.0004);
}

void SimplexNoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void SimplexNoiseEffect::setSpeed(float s) {
    speed = s;
}

void SimplexNoiseEffect::update() {
    // uint16_t currentTimeSpeedInt = millis() * speed;
    // for(int i = 0; i < m_matrix->getXResolution(); i++) {
    //     int ioffset = scale * i;
    //     for(int j = 0; j < m_matrix->getYResolution(); j++) {
    //         int noiseNow = int((1 + noise.GetNoise(j * scale * 10, i * scale * 10, int(currentTimeSpeedInt*speed))) / 127);
    //         CRGB col = baseColor;
    //         col.nscale8(noiseNow);
    //         m_matrix->background->drawPixel(i, j, col);
    //     }
    // }
}

const char* SimplexNoiseEffect::getName() const {
    return "SimplexNoise";
}