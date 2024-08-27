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
  }

  void drawSegment(uint8_t i, PVector vin, uint8_t r, uint8_t g, uint8_t b) {
    PVector dv = vin - segmentPositions[i];
    float segmentAngle = dv.heading();

    segmentPositions[i].x = vin.x - cos(segmentAngle);
    segmentPositions[i].y = vin.y - sin(segmentAngle);
    matrix->foreground->drawPixel(segmentPositions[i].x, segmentPositions[i].y, CRGB(r, g, b));
  }

  void displayChild() override {
    for(int8_t i=static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
      drawSegment(i+1, segmentPositions[i], CHILD_COLOR);
    }
    drawSegment(0, pos, CHILD_COLOR);
  }

  void displayTeen(float transitionFactor) override {
    for(int8_t i=static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
      drawSegment(i+1, segmentPositions[i],colorPalette->colors[i].r, colorPalette->colors[i].g, colorPalette->colors[i].b);
    }
    drawSegment(0, pos, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
  }

  void displayAdult() override {
    for(int8_t i=static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
      drawSegment(i+1, segmentPositions[i], colorPalette->colors[i].r, colorPalette->colors[i].g, colorPalette->colors[i].b);
    }
    drawSegment(0, pos, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
  }

  void displaySenior(float transitionFactor) override {
    for(int8_t i=static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
      drawSegment(i+1, segmentPositions[i], colorPalette->colors[i].r, colorPalette->colors[i].g, colorPalette->colors[i].b);
    }
    drawSegment(0, pos, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
  }
};

#endif