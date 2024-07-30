/*
*
* Aurora: https://github.com/pixelmatix/aurora
* Copyright (c) 2014 Jason Coon
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PatternIncrementalDrift2_H
#define PatternIncrementalDrift2_H

class PatternIncrementalDrift2 : public Drawable {
  public:
    PatternIncrementalDrift2() {
      name = (char *)"Incremental Drift Rose";
    }

    unsigned int drawFrame() {
      uint8_t dim = beatsin8(2, 170, 250);
      effects.DimAll(dim); effects.ShowFrame();

      for (int i = 2; i < VIRTUAL_RES_Y / 2; ++i)
      //for (uint8_t i = 0; i < 32; i++)
      {
        CRGB color;

        uint8_t x = 0;
        uint8_t y = 0;

        if (i < 16) {
          x = effects.beatcos8((i + 1) * 2, i, VIRTUAL_RES_X - i);
          y = beatsin8((i + 1) * 2, i, VIRTUAL_RES_Y - i);
          color = effects.ColorFromCurrentPalette(i * 14);
        }
        else
        {
          x = beatsin8((32 - i) * 2, VIRTUAL_RES_X - i, i + 1);
          y = effects.beatcos8((32 - i) * 2, VIRTUAL_RES_Y - i, i + 1);
          color = effects.ColorFromCurrentPalette((31 - i) * 14);
        }

        effects.setPixel(x, y, color);
      }

      return 0;
    }
};

#endif
