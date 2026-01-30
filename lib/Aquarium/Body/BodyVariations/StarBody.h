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
    // Use squared magnitude to avoid sqrt - only need sqrt if actually near zero
    float velMagSq = vel.x * vel.x + vel.y * vel.y;
    float velocityMagnitude = velMagSq > 0.01f ? sqrt(velMagSq) : 0.1f;
    starAngle += velocityMagnitude * rotationSpeed;
    
    // Normalize starAngle to keep it between 0 and TWO_PI
    if (starAngle >= TWO_PI) starAngle -= TWO_PI;
    else if (starAngle < 0) starAngle += TWO_PI;

    // Pre-calculate common values
    float angleStep = TWO_PI / arms;
    float lengthScaled = length * size;
    float radScaled = rad * size;
    int centerRadius = max(1, int(radScaled * 0.5f));
    int nodeRadius = max(1, int(radScaled * 0.5f));
    
    // Cache colors to avoid repeated array lookups
    CRGB color0 = colorPalette->colors[0];
    CRGB color1 = colorPalette->colors[1];
    CRGB color2 = colorPalette->colors[2];

    if (nodes) {
      // Node-based star
      for (int i = 0; i < arms; i++) {
        float armAngle = starAngle + angleStep * i;
        float cosArm = cos(armAngle);
        float sinArm = sin(armAngle);
        float endX = pos.x + cosArm * lengthScaled;
        float endY = pos.y + sinArm * lengthScaled;
        
        // Draw arm
        matrix->foreground->drawLine(pos.x, pos.y, endX, endY, color0);
        
        // Draw node at arm end
        matrix->foreground->fillCircle(endX, endY, nodeRadius, color1);
      }
      
      // Draw center
      matrix->foreground->fillCircle(pos.x, pos.y, centerRadius, color2);
    } else {
      // Triangle-based star
      float halfAngleStep = PI / arms;
      for (int i = 0; i < arms; i++) {
        float armAngle = starAngle + angleStep * i;
        
        // Calculate all three points with direct trig
        float pt1x = pos.x + cos(armAngle) * lengthScaled;
        float pt1y = pos.y + sin(armAngle) * lengthScaled;
        float pt2x = pos.x + cos(armAngle - halfAngleStep) * radScaled;
        float pt2y = pos.y + sin(armAngle - halfAngleStep) * radScaled;
        float pt3x = pos.x + cos(armAngle + halfAngleStep) * radScaled;
        float pt3y = pos.y + sin(armAngle + halfAngleStep) * radScaled;
        
        matrix->foreground->fillTriangle(pt1x, pt1y, pt2x, pt2y, pt3x, pt3y, color0);
      }
      
      // Draw center
      matrix->foreground->fillCircle(pos.x, pos.y, centerRadius, color2);
    }
  }
};

#endif