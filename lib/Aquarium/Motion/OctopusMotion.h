#ifndef OCTOPUSMOTION_H
#define OCTOPUSMOTION_H

#include "Motion.h"

class OctopusMotion : public Motion {
 public:
  OctopusMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    maxSpeed = OCTOPUS_MAX_SPEED;
    minSpeed = OCTOPUS_MIN_SPEED;
    maxforce = OCTOPUS_MAX_FORCE;
    sinAmplitude = OCTOPUS_SIN_AMPLITUDE;
    sinFrequency = OCTOPUS_SIN_FREQUENCY;
    noiseAmplitude = OCTOPUS_NOISE_AMPLITUDE;
    noiseFrequency = OCTOPUS_NOISE_FREQUENCY;
    
    do {
      vel = PVector(int8_t(random(-10, 10)), int8_t(random(-10, 10)));
    } while (vel.mag() < 5);
    acc = PVector(0, 0);

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
    noise.SetFrequency(noiseFrequency);
  }

  void doMotion() {
    frontSineMotion();
    // Add a slight rotation to make the movement more interesting
    // angle += sin(millis() / 1000.0) * 0.05;
  }
};

#endif