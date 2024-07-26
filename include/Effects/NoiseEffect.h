#ifndef NOISE_H
#define NOISE_H

#include "Effect.h"

class NoiseEffect : public Effect {
private:
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t hue = 140;
    uint8_t scale = 30;
    float speed = 0.02;

public:
    NoiseEffect(Matrix* m) : Effect(m) {}

    void setScale(uint8_t s) { scale = s; }
    void setSpeed(float s) { speed = s; }
    void setHue(uint8_t h) { hue = h; }

    void update() override {
        uint16_t currentTimeSpeedInt = millis() * speed;
        for(int i = 0; i < m_matrix->getXResolution(); i++) {
            int ioffset = scale * i;
            for(int j = 0; j < m_matrix->getYResolution(); j++) {
                int joffset = scale * j;
                uint8_t noiseNow = inoise8(x + ioffset, y + joffset, currentTimeSpeedInt);
                m_matrix->leds[j * m_matrix->getXResolution() + i] = CHSV(hue,255,noiseNow);
            }
        }
    }

    const char* getName() const override { return "Noise"; }
};

#endif // NOISE_H