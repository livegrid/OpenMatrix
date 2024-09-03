#ifndef BODY_H
#define BODY_H

#include <Arduino.h>
#include <Matrix.h>
#include <Body/Fin.h>
#include <Body/Head.h>
#include <Body/Tail.h>
#include <Body/ColorPalette.h>
#include <AquariumSettings.h>

class Body {
protected:
  Matrix* matrix;
  PVector pos;
  float size;
  float angle;
  float gapBetweenSegments;

  PVector vel;

  Fin* fin;
  Head* head;
  Tail* tail;
  ColorPalette* colorPalette;

  float transitionFactor;

  std::vector<uint8_t> segments;
  std::vector<PVector> segmentPositions;

public:
  Body(Matrix* m, Head* head, Tail* tail, Fin* fin) : matrix(m), head(head), tail(tail), fin(fin) {
  }

  virtual void update(PVector pos, PVector vel, float angle, float age = 0.8, long co2 = 600) {
    this->pos = pos;
    this->angle = angle;
    this->vel = vel;

    this->size = age;
  }

  void displayEgg() {
    matrix->foreground->fillCircle(pos.x, pos.y, EGG_SIZE, CRGB(EGG_COLOR));
  }

  virtual void display() = 0;

};

#endif // BODY_H


