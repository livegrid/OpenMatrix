#ifndef FIN_H
#define FIN_H

#include <Arduino.h>
#include <Matrix.h>

class Fin {
protected:
  Matrix* matrix;

public:
  Fin(Matrix* m) : matrix(m) {}
  String type;
  virtual void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) = 0;
};

class TriangleFin : public Fin {
  uint8_t varY = random(0,5);
  public:
  TriangleFin(Matrix* m) : Fin(m) {
    type = "TriangleFin";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    PVector heading = PVector::fromAngle(angle);
    PVector pt1 = heading;
    pt1.setMag(size);
    pt1 += pos;
    PVector pt2 = heading;
    pt2.setMag(-size/2);
    pt2 += pos;
    heading *= 3;
    PVector pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(size * 2);
    pt3 -= heading;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
    pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(-size * 2);
    pt3 -= heading;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
  }
};

class EllipseFin : public Fin {
public:
  EllipseFin(Matrix* m) : Fin(m) {
    type = "EllipseFin";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    matrix->foreground->drawCircleArray(pos.x, pos.y, size*3, size/3, angle, CRGB(r, g, b));
  }
};

class LegFin : public Fin {
public:
  LegFin(Matrix* m) : Fin(m) {}

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    // PVector heading = PVector::fromAngle(angle);
    // heading *= 0.5;
    PVector pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(size*2);
    // pt += heading;
    pt += pos;
    matrix->foreground->drawLine(pt.x, pt.y, pos.x, pos.y, CRGB(r, g, b));
    pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(-size*2);
    // pt += heading;
    pt += pos;
    matrix->foreground->drawLine(pt.x, pt.y, pos.x, pos.y, CRGB(r, g, b));
  }
};

class RoundFin : public Fin {
public:
  RoundFin(Matrix* m) : Fin(m) {
    type = "RoundFin";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    // PVector heading = PVector::fromAngle(angle);
    // heading *= 0.5;
    PVector pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(size*2);
    // pt += heading;
    pt += pos;
    matrix->foreground->drawLine(pt.x, pt.y, pos.x, pos.y, CRGB(r, g, b));
    matrix->foreground->fillCircle(pt.x, pt.y, size/3, CRGB(b, g, r));
    pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(-size*2);
    // pt += heading;
    pt += pos;
    matrix->foreground->drawLine(pt.x, pt.y, pos.x, pos.y, CRGB(r, g, b));
    matrix->foreground->fillCircle(pt.x, pt.y, size/3, CRGB(b, g, r));
  }
};

#endif // FIN_H
