#pragma once

#include "Effect.h"

class NoiseEffect : public Effect {
private:
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t hue = 140;
    uint8_t scale = 30;
    float speed = 0.02;

public:
    NoiseEffect(Matrix* m);

    void setScale(uint8_t s);
    void setSpeed(float s);
    void setHue(uint8_t h);

    void update() override;

    const char* getName() const override;
};
