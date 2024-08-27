#ifndef HEAD_H
#define HEAD_H

#include <Arduino.h>
#include <Matrix.h>

class Head {
protected:
  Matrix* matrix;

public:
  Head(Matrix* m) : matrix(m) {}

  virtual void display(PVector position, float angle, uint8_t segmentSize, uint8_t bodySize, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) = 0;
};

class TriangleHead : public Head {
public:
  TriangleHead(Matrix* m) : Head(m) {}

  void display(PVector position, float angle, uint8_t segmentSize, uint8_t bodySize, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    PVector pt1 = PVector::fromAngle(angle);
    pt1 *= segmentSize * bodySize;
    pt1 += position;
    PVector pt2 = PVector::fromAngle(angle - PI / 2);
    pt2 *= segmentSize * bodySize/2;
    pt2 += position;
    PVector pt3 = PVector::fromAngle(angle + PI / 2);
    pt3 *= segmentSize * bodySize/2;
    pt3 += position;
    // matrix->drawRectangle(int8_t(pos.x/physicsScale), int8_t(pos.y/PHYSICS_SCALE), 5, 5, col.r, col.g, col.b);
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
    // matrix->drawCircle(int8_t(pos.x/physicsScale), int8_t(pos.y/PHYSICS_SCALE), 3, true, col.r, col.g, col.b);
  }
};

class FrogHead : public Head {
public:
  FrogHead(Matrix* m) : Head(m) {}
  void display(PVector position, float angle, uint8_t segmentSize, uint8_t bodySize, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    PVector heading = PVector::fromAngle(angle);
    heading *= 0.5;
    PVector pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(segmentSize);
    pt += heading;
    pt += position;
    matrix->foreground->fillCircle(pt.x, pt.y, segmentSize/2 * bodySize, CRGB(b, g, r));
    pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(-segmentSize);
    pt += heading;
    pt += position;
    matrix->foreground->fillCircle(pt.x, pt.y, segmentSize/2 * bodySize, CRGB(b, g, r));
  }
};

class NeedleHead : public Head {
public:
  NeedleHead(Matrix* m) : Head(m) {}

  void display(PVector position, float angle, uint8_t segmentSize, uint8_t bodySize, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    PVector pt1 = PVector::fromAngle(angle);
    pt1 *= segmentSize * bodySize * 5;
    pt1 += position;
    matrix->foreground->drawLine(pt1.x, pt1.y, position.x, position.y, CRGB(r, g, b));
  }
};

#endif // HEAD_H
