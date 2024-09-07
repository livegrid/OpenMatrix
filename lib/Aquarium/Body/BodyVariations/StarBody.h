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
      for (int i = 0; i < arms; i++) {
        PVector armPt = PVector(pt1.x, pt1.y);
        armPt.rotate(2 * PI / arms * i);
        PVector endPoint = pos + armPt;
        matrix->foreground->drawLine(pos.x, pos.y, endPoint.x, endPoint.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
        matrix->foreground->fillCircle(endPoint.x, endPoint.y, rad / 2, CRGB(colorPalette->colors[1].r, colorPalette->colors[1].g, colorPalette->colors[1].b));
      }
      matrix->foreground->fillCircle(pos.x, pos.y, rad * size, CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g, colorPalette->colors[2].b));
    } else {
      PVector pt2 = PVector::fromAngle(starAngle - PI / 2);
      PVector pt3 = PVector::fromAngle(starAngle + PI / 2);
      pt2.setMag(rad * size);
      pt3.setMag(rad * size);
      
      for (int i = 0; i < arms; i++) {
        PVector arm1 = PVector(pt1.x, pt1.y);
        PVector arm2 = PVector(pt2.x, pt2.y);
        PVector arm3 = PVector(pt3.x, pt3.y);
        arm1.rotate(2 * PI / arms * i);
        arm2.rotate(2 * PI / arms * i);
        arm3.rotate(2 * PI / arms * i);
        matrix->foreground->fillTriangle(pos.x + arm1.x, pos.y + arm1.y, 
                                         pos.x + arm2.x, pos.y + arm2.y, 
                                         pos.x + arm3.x, pos.y + arm3.y, 
                                         CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
      }
      matrix->foreground->fillCircle(pos.x, pos.y, rad / 2 * size, CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g, colorPalette->colors[2].b));
    }
  }

};

#endif