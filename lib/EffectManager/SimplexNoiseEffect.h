#pragma once

#include "Effect.h"
#include "FastNoise.h"

class SimplexNoiseEffect : public Effect {
private:
  uint16_t x = 0;
  uint16_t y = 0;
  uint8_t scale = 1;
  float speed = 0.1;
  FastNoiseLite noise;
  float* m_noiseBuffer = nullptr;
  int m_bufferSize = 0;

public:
  SimplexNoiseEffect(Matrix* m);
  ~SimplexNoiseEffect();

  void setScale(uint8_t s);
  void setSpeed(float s);
  void update() override;

  const char* getName() const override;
};