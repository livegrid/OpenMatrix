#ifndef BODY_H
#define BODY_H

#include <Arduino.h>
#include <Matrix.h>
#include <Body/Fin.h>
#include <Body/Head.h>
#include <Body/Tail.h>
#include <Body/ColorPalette.h>
#include <AquariumSettings.h>

class Body {
protected:
  Matrix* matrix;
  PVector pos;
  float size;
  float health;
  float angle;
  float gapBetweenSegments;

  PVector vel;

  Head* head;
  Tail* tail;
  Fin* fin;
  ColorPalette* colorPalette;

  float transitionFactor;

  std::vector<uint8_t> segments;
  std::vector<PVector> segmentPositions;

public:
  Body(Matrix* m, Head* head, Tail* tail, Fin* fin) : matrix(m), head(head), tail(tail), fin(fin) {
  }

  String getHeadType() {
    return head->type;
  }

  String getFinType() {
    return fin->type;
  }

  String getTailType() {
    return tail->type;
  }

  String type;

  virtual void update(PVector pos, PVector vel, float angle, float age = 0.8, float health = 1.0) {
    this->pos = pos;
    this->angle = angle;
    this->vel = vel;
    this->size = age;
    this->health = health;
    colorPalette->adjustColorByAgeAndHealth(this->size, this->health);
  }

  void displayEgg() {
    matrix->foreground->drawPixel(pos.x, pos.y, CRGB(colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b));
  }
  
  void setColorPaletteHSV(const std::vector<CHSV>& palette) {
    colorPalette->setColors(palette);
  }

  const std::vector<CHSV>& getColorPaletteHSV() const {
    return colorPalette->getColorsHSV();
  }

  std::vector<CRGB> getColorPaletteRGB() const {
    return colorPalette->getColors();
  }

  virtual void display() = 0;
};

#endif // BODY_H


