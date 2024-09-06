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
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad * size, length*(1+vel.mag()/20) * size, angle + PI/2, colorPalette->colors[0]);
  }
};

#endif