#ifndef TURTLEBODY_H
#define TURTLEBODY_H

#include <Arduino.h>
#include <Body/Body.h>

class TurtleBody : public Body {
  protected:
  uint8_t rad, length;
public:
    TurtleBody(Matrix* m, Head* head, Tail* tail, Fin* fin) : Body(m, head, tail, fin) {
    length = random(TURTLE_LENGTH); // random upper bound is exclusive
    rad = random(TURTLE_WIDTH);

    colorPalette = new ColorPalette(rad);
    type = "Turtle";
  }

  void display() override {
    uint8_t rad2draw = rad * size;
    if(rad2draw < 1) rad2draw = 1;
    uint8_t length2draw = length*(1+vel.mag()/20) * size;
    if(length2draw < 1) length2draw = 1;
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad2draw, length2draw, angle + PI/2, colorPalette->colors[0]);
  }
};

#endif