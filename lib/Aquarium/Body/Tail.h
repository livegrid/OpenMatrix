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
    matrix->foreground->drawCircleArray(pos.x, pos.y, size/3, size*3, angle+PI/2, CRGB(r, g, b));
  }
};

// ... existing code ...

class WavyTail : public Tail {
private:
    std::vector<PVector> segmentPositions;

public:
    WavyTail(Matrix* m) : Tail(m) {
        type = "WavyTail";
        int numSegments = random(5, 15);  // Adjust range as needed

        for (int i = 0; i < numSegments; ++i) {
            segmentPositions.push_back(PVector(0, 0));
        }
    }

    void drawSegment(uint8_t i, PVector vin, uint8_t r, uint8_t g, uint8_t b) {
        PVector dv = vin - segmentPositions[i];
        float segmentAngle = dv.heading();

        segmentPositions[i].x = vin.x - cos(segmentAngle);
        segmentPositions[i].y = vin.y - sin(segmentAngle);
        matrix->foreground->drawPixel(segmentPositions[i].x, segmentPositions[i].y, CRGB(r, g, b));
    }

    void display(PVector pos, float angle, uint8_t size, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) override {
        for(int8_t i = static_cast<int8_t>((segmentPositions.size()-2) * size); i > -1; i--) {
            drawSegment(i+1, segmentPositions[i], r, g, b);
        }
        drawSegment(0, pos, r, g, b);
    }
};

#endif // TAIL_H
