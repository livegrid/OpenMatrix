#ifndef SNAKEBODY_H
#define SNAKEBODY_H

#include <Arduino.h>
#include <Body/Body.h>

class SnakeBody : public Body {
public:
    SnakeBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    int numSegments = random(SNAKE_NUM_SEGMENTS); // random upper bound is exclusive

    for (int i = 0; i < numSegments; ++i) {
      segmentPositions.push_back(PVector(0, 0));
    }
    colorPalette = new ColorPalette(numSegments);
    type = "Snake";
  }

  void drawSegment(uint8_t i, PVector vin, uint8_t r, uint8_t g, uint8_t b) {
    PVector dv = vin - segmentPositions[i];
    float dvMagSq = dv.x * dv.x + dv.y * dv.y;
    
    // Use normalized vector directly instead of atan2 + cos/sin
    if (dvMagSq > 0.0001f) {
      float invMag = 1.0f / sqrt(dvMagSq);
      segmentPositions[i].x = vin.x - dv.x * invMag;
      segmentPositions[i].y = vin.y - dv.y * invMag;
    } else {
      segmentPositions[i].x = vin.x - 1.0f;
      segmentPositions[i].y = vin.y;
    }
    matrix->foreground->drawPixel(segmentPositions[i].x, segmentPositions[i].y, CRGB(r, g, b));
  }

  void display() override {
    for(int8_t i=static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
      drawSegment(i+1, segmentPositions[i], colorPalette->colors[i].r, colorPalette->colors[i].g, colorPalette->colors[i].b);
    }
    drawSegment(0, pos, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
  }

};

#endif