#ifndef FISHMOTION_H
#define FISHMOTION_H

#include "Motion.h"

class FishMotion : public Motion {
 public:
  FishMotion(PVector pos, uint16_t xResolution, uint16_t yResolution, std::vector<std::shared_ptr<Attractor>>* attractors)
      : Motion(pos, xResolution, yResolution, attractors) {
    maxSpeed = FISH_MAX_SPEED;
    minSpeed = FISH_MIN_SPEED;
    maxforce = FISH_MAX_FORCE;
    sinAmplitude = FISH_SIN_AMPLITUDE;
    sinFrequency = FISH_SIN_FREQUENCY;
    noiseAmplitude = FISH_NOISE_AMPLITUDE;
    noiseFrequency = FISH_NOISE_FREQUENCY;
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