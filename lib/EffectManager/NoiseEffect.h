#pragma once

#include "Effect.h"
#include "FastNoise.h"

class NoiseEffect : public Effect {
private:
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t scale = 15;
    float speed = 0.05f;

    FastNoiseLite noise;
    float* m_noiseBuffer = nullptr;
    int m_bufferSize = 0;

public:
    NoiseEffect(Matrix* m);
    ~NoiseEffect();

    void setScale(uint8_t s);
    void setSpeed(float s);

    void update() override;

    void reset() override;

    const char* getName() const override;
};