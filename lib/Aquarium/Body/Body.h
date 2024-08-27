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

  float sinAngle;

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

  virtual void update(PVector pos, float angle, float sinAngle = 0, float age = 0.8, long co2 = 600) {
    this->pos = pos;
    this->angle = angle;
    this->sinAngle = sinAngle;

    this->size = age;
  }

  void display() {
    colorPalette->adjustColorbyAge(size);
    if(size < AGE_EGG) displayEgg();
    else if(size < AGE_CHILD) displayChild();
    else if(size < AGE_TEEN)  {
      transitionFactor = map(size, AGE_CHILD*10, AGE_CHILD*10+AGE_TEEN*10, 0, 1000)/1000.0;
      displayTeen(transitionFactor);
    }
    else if(size < AGE_ADULT) displayAdult();
    else {
      transitionFactor = map(size, AGE_ADULT*10, AGE_ADULT*10+AGE_SENIOR*10, 0, 1000)/1000.0;
      displaySenior(transitionFactor);
    }
  };

  void displayEgg() {
    matrix->foreground->fillCircle(pos.x, pos.y, EGG_SIZE, CRGB(EGG_COLOR));
  }

  virtual void displayChild() = 0;
  virtual void displayTeen(float transitionFactor) = 0;
  virtual void displayAdult() = 0;
  virtual void displaySenior(float transitionFactor) = 0;

};

#endif // BODY_H


