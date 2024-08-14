#pragma once

#include "Effect.h"

class NoiseEffect : public Effect {
private:
    uint16_t x = 0;
    uint16_t y = 0;
    uint8_t scale = 3;
    float speed = 0.05;

public:
    NoiseEffect(Matrix* m);

    void setScale(uint8_t s);
    void setSpeed(float s);

    void update() override;

    const char* getName() const override;
};