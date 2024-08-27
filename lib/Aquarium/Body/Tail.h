#ifndef TAIL_H
#define TAIL_H

#include <Arduino.h>
#include <Matrix.h>

class Tail {
protected:
  Matrix* matrix;

public:
  Tail(Matrix* m) : matrix(m) {}

  virtual void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) = 0;
};

class noTail : public Tail {
public:
  noTail(Matrix* m) : Tail(m) {}

  void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
    // Do nothing
  }
};

class TriangleTail : public Tail {
public:
  TriangleTail(Matrix* m) : Tail(m) {}

  void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
    PVector heading = PVector::fromAngle(angle);
    PVector pt1 = heading;
    pt1.setMag(size);
    PVector pt2 = heading;
    pt2.setMag(-size);
    heading *= 3;
    PVector pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(size * 3);
    pt3 -= heading;
    pt1 += pos;
    pt2 += pos;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
    pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(-size * 3);
    pt3 -= heading;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
  }
};

class CurvyTail : public Tail {
public:
  CurvyTail(Matrix* m) : Tail(m) {}

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    matrix->foreground->drawCircleArray(pos.x, pos.y, size*3, size/3, angle, CRGB(r, g, b));
  }
};

#endif // TAIL_H
