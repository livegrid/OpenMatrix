#ifndef STARBODY_H
#define STARBODY_H

#include <Body/Body.h>
#include <Arduino.h>

class StarBody : public Body {
 protected:
  uint8_t rad, length;
  uint8_t arms;
  bool nodes;
  float starAngle;
  float rotationSpeed;

 public:
  StarBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    length = random(STAR_LENGTH);  // random upper bound is exclusive
    rad = random(STAR_RAD);
    arms = random(STAR_NUM_ARMS);
    rotationSpeed = random(STAR_ROTATION_SPEED)/1000.0; 

    colorPalette = new ColorPalette(3);
    nodes = random(0, 2);
    type = "Star";
  }

  void display() override {
    starAngle += vel.mag() * rotationSpeed;
    PVector pt1 = PVector::fromAngle(starAngle);
    pt1.setMag(length * 2 * size);

    if (nodes) {
      matrix->foreground->drawLine(pos.x, pos.y, pos.x + pt1.x, pos.y + pt1.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
      matrix->foreground->fillCircle(pos.x + pt1.x, pos.y + pt1.y, rad / 2, CRGB(colorPalette->colors[1].r, colorPalette->colors[1].g, colorPalette->colors[1].b));
      for (int i = 1; i < arms; i++) {
        pt1.rotate(2 * PI / arms);
        matrix->foreground->drawLine(pos.x, pos.y, pos.x + pt1.x, pos.y + pt1.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
        matrix->foreground->fillCircle(pos.x + pt1.x, pos.y + pt1.y, rad/2, CRGB(colorPalette->colors[1].r, colorPalette->colors[1].g, colorPalette->colors[1].b));
      }
      matrix->foreground->fillCircle(pos.x, pos.y, rad * size, CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g, colorPalette->colors[2].b));
    } else {
      PVector pt2 = PVector::fromAngle(starAngle - PI / 2);
      PVector pt3 = PVector::fromAngle(starAngle + PI / 2);
      pt2.setMag(rad * size);
      pt3.setMag(rad * size);
      matrix->foreground->fillTriangle(pos.x + pt1.x, pos.y + pt1.y, pos.x + pt2.x, pos.y + pt2.y, pos.x + pt3.x, pos.y + pt3.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
      for (int i = 1; i < arms; i++) {
        pt1.rotate(2 * PI / arms);
        pt2.rotate(2 * PI / arms);
        pt3.rotate(2 * PI / arms);
        matrix->foreground->fillTriangle(pos.x + pt1.x, pos.y + pt1.y, pos.x + pt2.x, pos.y + pt2.y, pos.x + pt3.x, pos.y + pt3.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
      }
      matrix->foreground->fillCircle(pos.x, pos.y, rad / 2 * size, CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g, colorPalette->colors[2].b));
    }
  }

};

#endif