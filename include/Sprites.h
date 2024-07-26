#ifndef SPRITES_H
#define SPRITES_H

#include <Arduino.h>

struct ImageData {
  const uint8_t *data;
  uint8_t height;
  uint8_t width;
};

#endif