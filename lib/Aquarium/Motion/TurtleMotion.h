#ifndef TURTLEMOTION_H
#define TURTLEMOTION_H

#include "Motion.h"

class TurtleMotion : public Motion {
 public:
  TurtleMotion(PVector pos, uint16_t xResolution, uint16_t yResolution)
      : Motion(pos, xResolution, yResolution) {
    maxSpeed = TURTLE_MAX_SPEED;
    minSpeed = TURTLE_MIN_SPEED;
    maxforce = TURTLE_MAX_FORCE;
    sinAmplitude = TURTLE_SIN_AMPLITUDE;
    sinFrequency = TURTLE_SIN_FREQUENCY;
    noiseAmplitude = TURTLE_NOISE_AMPLITUDE;
    noiseFrequency = TURTLE_NOISE_FREQUENCY;
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
  }
};

#endif