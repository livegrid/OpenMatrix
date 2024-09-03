#ifndef FISHBODY_H
#define FISHBODY_H

#include <Arduino.h>
#include <Body/Body.h>

class FishBody : public Body {
public:
    FishBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    int numSegments = random(FISH_NUM_SEGMENTS); // random upper bound is exclusive
    float baseSize = FISH_MIN_SEGMENT_SIZE; // Minimum size of segment
    float maxAddSize = random(FISH_MAX_SEGMENT_SIZE); // Maximum additional size (total max 8)
    gapBetweenSegments = random(FISH_GAP_BETWEEN_SEGMENTS) / 100.0; // random upper bound is exclusive

    for (int i = 0; i < numSegments; ++i) {
      float phase = (PI * i) / (numSegments - 1); // Phase shift to distribute sizes along the sine wave
      uint8_t segmentSize = static_cast<uint8_t>(baseSize + sin(phase) * maxAddSize);
      segments.push_back(segmentSize);
      segmentPositions.push_back(PVector(0, 0));
    }
    colorPalette = new ColorPalette(numSegments, true);
  }

  void drawSegment(uint8_t i, PVector vin, uint8_t r, uint8_t g, uint8_t b, bool drawExtras = false) {
    PVector dv = vin - segmentPositions[i];
    float segmentAngle = dv.heading();

    // Use updated size for each segment
    float currentSegmentSize = segments[i] * size;

    if(i==0) {
      segmentPositions[i].x = vin.x - cos(segmentAngle) * currentSegmentSize;
      segmentPositions[i].y = vin.y - sin(segmentAngle) * currentSegmentSize;
    } else {
      float maxSegmentSize = std::max(segments[i-1], segments[i]) * size * gapBetweenSegments;
      segmentPositions[i].x = vin.x - cos(segmentAngle) * maxSegmentSize;
      segmentPositions[i].y = vin.y - sin(segmentAngle) * maxSegmentSize;
    }

    if(drawExtras && (i == 1 || i == 3)) {
      fin->display(segmentPositions[i], segmentAngle, currentSegmentSize, r, g, b);
    }

    if(drawExtras && i == segments.size()-1) {
      tail->display(segmentPositions[i], segmentAngle, currentSegmentSize * 2, r, g, b);
    }
    else {
      matrix->foreground->fillCircle(segmentPositions[i].x, segmentPositions[i].y, currentSegmentSize, CRGB(r, g, b));
    }
  }

  void display() override {
    colorPalette->adjustColorbyAge(size);
    for(int8_t i=segments.size()-2; i > -1; i--) {
      drawSegment(i+1, segmentPositions[i], colorPalette->colors[i].r, colorPalette->colors[i].g, colorPalette->colors[i].b, true);
    }
    drawSegment(0, pos, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b, false);
    head->display(pos, angle, segments[0], size, 255, 255, 255);
  }

};

#endif