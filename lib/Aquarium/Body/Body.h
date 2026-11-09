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
    const int x = (int)pos.x;
    const int y = (int)pos.y;
    if (x < 0 || y < 0 || x >= matrix->getXResolution() ||
        y >= matrix->getYResolution()) {
      return;
    }

    // Twinkle via alpha over the background. Never fade the egg toward black —
    // Stack treats CRGB(0,0,0) as transparent, so dimming RGB would look like a
    // dead pixel instead of disappearing into the water.
    const float phase =
        millis() * EGG_TWINKLE_SPEED + pos.x * 17.0f + pos.y * 31.0f;
    const float wave = (sinf(phase) + 1.0f) * 0.5f;
    const uint8_t alpha = (uint8_t)(EGG_TWINKLE_MIN_ALPHA +
                                    wave * (EGG_TWINKLE_MAX_ALPHA -
                                            EGG_TWINKLE_MIN_ALPHA));

    const CRGB eggColor = colorPalette->colors[0];
    const CRGB bgColor = matrix->background->pixels->data[y][x];
    matrix->foreground->drawPixel(x, y, blend(bgColor, eggColor, alpha));
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


