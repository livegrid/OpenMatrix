#include "NoiseEffect.h"

NoiseEffect::NoiseEffect(Matrix* m) : Effect(m) {}

void NoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void NoiseEffect::setSpeed(float s) {
    speed = s;
}

void NoiseEffect::reset() {}

// void NoiseEffect::setHue(uint8_t h) {
//     hue = h;
// }

void NoiseEffect::update() {
    uint16_t currentTimeSpeedInt = millis() * speed;
    for(int i = 0; i < m_matrix->getXResolution(); i++) {
        int ioffset = scale * i;
        for(int j = 0; j < m_matrix->getYResolution(); j++) {
            int joffset = scale * j;
            uint8_t noiseNow = inoise8(x + ioffset, y + joffset, currentTimeSpeedInt);
            CRGB col = baseColor;
            col.nscale8(noiseNow/2);
            m_matrix->background->drawPixel(i, j, col);
        }
    }
}

const char* NoiseEffect::getName() const {
    return "Noise";
}