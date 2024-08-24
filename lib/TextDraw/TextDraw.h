#pragma once

#include <Arduino.h>
#include <Fonts/Font4x5Fixed.h>
#include <Fonts/Font5x5Fixed.h>
#include <Fonts/Font5x7Fixed.h>
#include <Matrix.h>

class TextDraw {
 public:
  TextDraw(Matrix* matrix);
  void reset();
  void drawText(String text);
  void setSize(uint8_t size);
  void setColor(CRGB color);

 private:
  Matrix* matrix;
  uint8_t size;
  CRGB color;
};

TextDraw::TextDraw(Matrix* matrix) {
  this->matrix = matrix;
  this->size = 1;
  this->color = CRGB(255, 255, 255);
}

void TextDraw::drawText(String text) {
  matrix->background->clear();
  switch (size) {
    case 0:
      matrix->background->setCursor(0,5);
      break;
    case 1:
      matrix->background->setCursor(0,5);
      break;
    case 2:
      matrix->background->setCursor(0,7);
      break;
  }
  matrix->background->print(text);
  matrix->background->display();
}

void TextDraw::setSize(uint8_t size) {
  if(size > 2) {
    log_e("Invalid size: %d", size);
    return;
  }
  else if(size == this->size) {
    return;
  }
  else {
    this->size = size;
    switch (size) {
      case 0:
        matrix->background->setFont(&Font4x5Fixed);
        break;
      case 1:
        matrix->background->setFont(&Font5x5Fixed);
        break;
      case 2:
        matrix->background->setFont(&Font5x7Fixed);
        break;
    }
  }
}

void TextDraw::setColor(CRGB color) {
  this->color = color;
}