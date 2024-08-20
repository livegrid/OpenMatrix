#pragma once

#include "Matrix.h"

class Effect {
public:
    Effect(Matrix* matrix) : m_matrix(matrix) {}
    virtual ~Effect() {}

    virtual void reset() = 0;

    virtual void update() = 0;
    virtual const char* getName() const = 0;

    void setColor(CRGB baseColor) {
      this->baseColor = baseColor;
    }
    void setColor(CHSV baseColor) {
      hsv2rgb_rainbow(baseColor, this->baseColor);
    }
    void setSpeed(float speed) {
      this->speed = speed;
    }

protected:
    Matrix* m_matrix;
    CRGB baseColor = CRGB::White;
    float speed;
    uint16_t baseUpdateInterval = 30; // Base update interval in milliseconds
};