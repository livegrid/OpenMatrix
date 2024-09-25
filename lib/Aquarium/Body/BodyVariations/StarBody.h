#ifndef STARBODY_H
#define STARBODY_H

#include <Arduino.h>
#include <Body/Body.h>

class StarBody : public Body {
 protected:
  uint8_t rad, length;
  uint8_t arms;
  bool nodes;
  float starAngle;
  float rotationSpeed;

 public:
  StarBody(Matrix* m, Head* head, Tail* tail, Fin* fin)
      : Body(m, head, tail, fin) {
    length = random(STAR_LENGTH);  // random upper bound is exclusive
    rad = random(STAR_RAD);
    arms = random(STAR_NUM_ARMS);
    rotationSpeed = random(STAR_ROTATION_SPEED) / 1000.0;
    starAngle = random(TWO_PI);  // Random initial angle


    colorPalette = new ColorPalette(3);
    nodes = random(0, 2);
    type = "Star";
  }
  
  void display() override {
    float velocityMagnitude = max(vel.mag(), 0.1f);  // Ensure minimum velocity for rotation
    starAngle += velocityMagnitude * rotationSpeed;
    
    // Normalize starAngle to keep it between 0 and TWO_PI
    starAngle = fmod(starAngle, TWO_PI);
    if (starAngle < 0) starAngle += TWO_PI;

    if (nodes) {
      // Node-based star
      for (int i = 0; i < arms; i++) {
        float armAngle = starAngle + (TWO_PI / arms * i);
        PVector armPt = PVector::fromAngle(armAngle);
        armPt.setMag(length * size);
        PVector endPoint = pos + armPt;
        
        // Draw arm
        matrix->foreground->drawLine(
            pos.x, pos.y, endPoint.x, endPoint.y,
            CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g,
                colorPalette->colors[0].b));
        
        // Draw node at arm end
        matrix->foreground->fillCircle(
            endPoint.x, endPoint.y, max(1, int(rad * 0.5 * size)),
            CRGB(colorPalette->colors[1].r, colorPalette->colors[1].g,
                colorPalette->colors[1].b));
      }
      
      // Draw center
      matrix->foreground->fillCircle(
          pos.x, pos.y, max(1, int(rad * 0.5 * size)),
          CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g,
              colorPalette->colors[2].b));
    } else {
      // Triangle-based star
      for (int i = 0; i < arms; i++) {
        float armAngle = starAngle + (TWO_PI / arms * i);
        PVector pt1 = PVector::fromAngle(armAngle);
        PVector pt2 = PVector::fromAngle(armAngle - PI / arms);
        PVector pt3 = PVector::fromAngle(armAngle + PI / arms);
        
        pt1.setMag(length * size);
        pt2.setMag(rad * size);
        pt3.setMag(rad * size);
        
        matrix->foreground->fillTriangle(
            pos.x + pt1.x, pos.y + pt1.y,
            pos.x + pt2.x, pos.y + pt2.y,
            pos.x + pt3.x, pos.y + pt3.y,
            CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g,
                colorPalette->colors[0].b));
      }
      
      // Draw center
      matrix->foreground->fillCircle(
          pos.x, pos.y, max(1, int(rad * 0.5 * size)),
          CRGB(colorPalette->colors[2].r, colorPalette->colors[2].g,
              colorPalette->colors[2].b));
    }
  }
};

#endif