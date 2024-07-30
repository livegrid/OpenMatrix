#include "NoiseEffect.h"

NoiseEffect::NoiseEffect(Matrix* m) : Effect(m) {}

void NoiseEffect::setScale(uint8_t s) {
    scale = s;
}

void NoiseEffect::setSpeed(float s) {
    speed = s;
}

void NoiseEffect::setHue(uint8_t h) {
    hue = h;
}

void NoiseEffect::update() {
    uint16_t currentTimeSpeedInt = millis() * speed;
    for(int i = 0; i < m_matrix->getXResolution(); i++) {
        int ioffset = scale * i;
        for(int j = 0; j < m_matrix->getYResolution(); j++) {
            int joffset = scale * j;
            uint8_t noiseNow = inoise8(x + ioffset, y + joffset, currentTimeSpeedInt);
            m_matrix->leds[j * m_matrix->getXResolution() + i] = CHSV(hue, 255, noiseNow);
        }
    }
}

const char* NoiseEffect::getName() const {
    return "Noise";
}
