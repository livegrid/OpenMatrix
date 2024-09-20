#ifndef HEAD_H
#define HEAD_H

#include <Arduino.h>
#include <Matrix.h>

class Head {
protected:
  Matrix* matrix;

public:
  Head(Matrix* m) : matrix(m) {}
  virtual ~Head() = default;
  String type;
  virtual void display(PVector position, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) = 0;
};

class TriangleHead : public Head {
public:
  TriangleHead(Matrix* m) : Head(m) {
    type = "TriangleHead";
  }

  void display(PVector position, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    // Calculate the shift amount (e.g., 1/4 of the size)
    PVector shift = PVector::fromAngle(angle + PI);  // Opposite direction of heading
    shift *= size * 0.25;  // Adjust this factor to control the shift amount
    
    // Apply the shift to the position
    PVector shiftedPosition = position + shift;

    PVector pt1 = PVector::fromAngle(angle);
    pt1 *= size;
    pt1 += shiftedPosition;
    PVector pt2 = PVector::fromAngle(angle - PI / 2);
    pt2 *= size/2;
    pt2 += shiftedPosition;
    PVector pt3 = PVector::fromAngle(angle + PI / 2);
    pt3 *= size/2;
    pt3 += shiftedPosition;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r,g,b));
  }
};

class FrogHead : public Head {
public:
  FrogHead(Matrix* m) : Head(m) {
    type = "FrogHead";
  }

  void display(PVector position, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    PVector heading = PVector::fromAngle(angle);
    heading *= 0.5;
    PVector pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(size);
    pt += heading;
    pt += position;
    matrix->foreground->fillCircle(pt.x, pt.y, size/2, CRGB(b, g, r));
    pt = PVector::fromAngle(angle + PI/2);
    pt.setMag(-size);
    pt += heading;
    pt += position;
    matrix->foreground->fillCircle(pt.x, pt.y, size/2, CRGB(b, g, r));
  }
};

class NeedleHead : public Head {
  uint8_t noseLengthMultiplier = random(FISH_NEEDLE_NOSE_LENGTH_MULTIPLIER);
public:
  NeedleHead(Matrix* m) : Head(m) {
    type = "NeedleHead";
  }

  void display(PVector position, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, int8_t xOffset = 0, int8_t yOffset = 0) override {
    PVector pt1 = PVector::fromAngle(angle);
    pt1 *= size * noseLengthMultiplier;
    pt1 += position;
    matrix->foreground->drawLine(pt1.x, pt1.y, position.x, position.y, CRGB(r,g,b));
  }
};

#endif // HEAD_H
