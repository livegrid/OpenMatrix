#ifndef TAIL_H
#define TAIL_H

#include <Arduino.h>
#include <Matrix.h>

class Tail {
protected:
  Matrix* matrix;

public:
  Tail(Matrix* m) : matrix(m) {
    type = "noTail";
  }
  virtual ~Tail() = default;
  String type;

  virtual void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) = 0;
};

class noTail : public Tail {
public:
  noTail(Matrix* m) : Tail(m) {
    type = "noTail";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
    // Do nothing
  }
};

class TriangleTail : public Tail {
public:
  TriangleTail(Matrix* m) : Tail(m) {
    type = "TriangleTail";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
    PVector heading = PVector::fromAngle(angle);
    PVector pt1 = heading;
    pt1.setMag(size);
    PVector pt2 = heading;
    pt2.setMag(-size);
    heading *= 3;
    PVector pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(size * 3);
    pt3 -= heading;
    pt1 += pos;
    pt2 += pos;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
    pt3 = PVector::fromAngle(angle + PI/2);
    pt3.setMag(-size * 3);
    pt3 -= heading;
    pt3 += pos;
    matrix->foreground->fillTriangle(pt1.x, pt1.y, pt2.x, pt2.y, pt3.x, pt3.y, CRGB(r, g, b));
  }
};

class CurvyTail : public Tail {
public:
  CurvyTail(Matrix* m) : Tail(m) {
    type = "CurvyTail";
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r, uint8_t g, uint8_t b) override {
    matrix->foreground->drawCircleArray(pos.x, pos.y, size/3, size*3, angle, CRGB(r, g, b));
  }
};

class WavyTail : public Tail {
private:
  std::vector<std::vector<PVector>> segments;
  int numStrands;
  int segmentsPerStrand;
public:
   WavyTail(Matrix* m) : Tail(m) {
    type = "WavyTail";
    numStrands = 5; // Adjust this for more or fewer strands
    segmentsPerStrand = 8; // Adjust this for longer or shorter strands
    segments.resize(numStrands, std::vector<PVector>(segmentsPerStrand, PVector(0, 0)));
  }

  void drawSegment(PVector& pos, PVector& prevPos, uint8_t r, uint8_t g, uint8_t b) {
    PVector dv = pos - prevPos;
    float segmentAngle = dv.heading();

    pos.x = prevPos.x - cos(segmentAngle);
    pos.y = prevPos.y - sin(segmentAngle);
    matrix->foreground->drawPixel(pos.x, pos.y, CRGB(r, g, b));
  }

  void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
    PVector perpendicular = PVector::fromAngle(angle + HALF_PI);
    perpendicular.setMag(size);

    for (int i = 0; i < numStrands; i++) {
      float t = (float)i / (numStrands - 1) - 0.5;
      PVector strandStart = pos + perpendicular * t;

      PVector prevPos = strandStart;
      for (int j = 0; j < segmentsPerStrand; j++) {
        float waveAngle = angle + sin(j * 0.5 + i * 0.2) * 0.5; // Create a wavy pattern
        PVector waveVector = PVector::fromAngle(waveAngle);
        waveVector.setMag(size * 0.5);

        drawSegment(segments[i][j], prevPos, r, g, b);
        prevPos = segments[i][j];
      }
    }
  }
};

#endif // TAIL_H
