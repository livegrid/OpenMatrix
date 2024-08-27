#ifndef TURTLEMOTION_H
#define TURTLEMOTION_H

#include "Motion.h"

class TurtleMotion : public Motion {
 public:
  TurtleMotion(PVector pos, uint16_t xResolution, uint16_t yResolution, std::vector<std::shared_ptr<Attractor>>* attractors)
      : Motion(pos, xResolution, yResolution, attractors) {
    maxSpeed = 80;
    minSpeed = 20;
    maxforce = 0.3;
    sinAmplitude = 5;
    sinFrequency = .002;
    noiseAmplitude = 2;
    noiseFrequency = 0.01;
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