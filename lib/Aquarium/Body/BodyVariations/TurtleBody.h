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
  }

  void displayChild() override {
    // head->display(pos, angle, length, size, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
    
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad * size, length*(1-sinAngle/2) * size, angle + PI/2, colorPalette->colors[0]);
  }

  void displayTeen(float transitionFactor) override {
    // head->display(pos, angle, length, size, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
    
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad * size, length*(1-sinAngle/2) * size, angle + PI/2, colorPalette->colors[0]);
  }

  void displayAdult() override {
    // head->display(pos, angle, length, size, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
    
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad * size, length*(1-sinAngle/2) * size, angle + PI/2, colorPalette->colors[0]);
  }

  void displaySenior(float transitionFactor) override {
    // head->display(pos, angle, length, size, colorPalette->colors[0].r, colorPalette->colors[0].g, colorPalette->colors[0].b);
    
    matrix->foreground->drawCircleArray(pos.x, pos.y, rad * size, length*(1-sinAngle/2) * size, angle + PI/2, colorPalette->colors[0]);
  }
};

#endif