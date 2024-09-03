#ifndef STARMOTION_H
#define STARMOTION_H

#include "Motion.h"

class StarMotion : public Motion {
 public:
  StarMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    maxSpeed = STAR_MAX_SPEED;
    minSpeed = STAR_MIN_SPEED;
    maxforce = STAR_MAX_FORCE;
    sinAmplitude = STAR_SIN_AMPLITUDE;
    sinFrequency = STAR_SIN_FREQUENCY;
    noiseAmplitude = STAR_NOISE_AMPLITUDE;
    noiseFrequency = STAR_NOISE_FREQUENCY;
    do {
      vel = PVector(int8_t(random(-10, 10)), int8_t(random(-10, 10)));
    } while (vel.mag() < 5);
    acc = PVector(0, 0);

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFrequency(noiseFrequency);
  }
  void doMotion() {
    frontSineMotion();
    // sideSineMotion();
    noiseMotion();
    // angle += vel.mag() * 0.01;
  }
};

#endif