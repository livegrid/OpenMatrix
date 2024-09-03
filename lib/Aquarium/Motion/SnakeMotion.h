#ifndef SNAKEMOTION_H
#define SNAKEMOTION_H

#include "Motion.h"

class SnakeMotion : public Motion {
 public:
  SnakeMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    maxSpeed = SNAKE_MAX_SPEED;
    minSpeed = SNAKE_MIN_SPEED;
    maxforce = SNAKE_MAX_FORCE;
    sinAmplitude = SNAKE_SIN_AMPLITUDE;
    sinFrequency = SNAKE_SIN_FREQUENCY;
    noiseAmplitude = SNAKE_NOISE_AMPLITUDE;
    noiseFrequency = SNAKE_NOISE_FREQUENCY;
    do {
      vel = PVector(int8_t(random(-10, 10)), int8_t(random(-10, 10)));
    } while (vel.mag() < 5);
    acc = PVector(0, 0);

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFrequency(noiseFrequency);
  }
  
  void doMotion() {
    sideSineMotion();
    noiseMotion();
  }
};

#endif