/*
* Aurora: https://github.com/pixelmatix/aurora
* Copyright (c) 2014 Jason Coon
*
* Portions of this code are adapted from SmartMatrixSwirl by Mark Kriegsman: https://gist.github.com/kriegsman/5adca44e14ad025e6d3b
* https://www.youtube.com/watch?v=bsGBT-50cts
* Copyright (c) 2014 Mark Kriegsman
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

#ifndef PatternSwirl_H

class PatternSwirl : public Drawable {
  private:
    const uint8_t borderWidth = 2;

  public:
    PatternSwirl() {
      name = (char *)"Swirl";
    }

    void start() {
        effects.ClearFrame();
    }

    unsigned int drawFrame() {
      // Apply some blurring to whatever's already on the matrix
      // Note that we never actually clear the matrix, we just constantly
      // blur it repeatedly.  Since the blurring is 'lossy', there's
      // an automatic trend toward black -- by design.
      uint8_t blurAmount = beatsin8(2, 10, 255);

#if FASTLED_VERSION >= 3001000
      blur2d(effects.leds, VIRTUAL_RES_X > 255 ? 255 : VIRTUAL_RES_X, VIRTUAL_RES_Y > 255 ? 255 : VIRTUAL_RES_Y, blurAmount);
#else
      effects.DimAll(blurAmount); 
#endif

      // Use two out-of-sync sine waves
      uint8_t  i = beatsin8(256/VIRTUAL_RES_Y, borderWidth, VIRTUAL_RES_X - borderWidth);
      uint8_t  j = beatsin8(2048/VIRTUAL_RES_X, borderWidth, VIRTUAL_RES_Y - borderWidth);

      // Also calculate some reflections
      uint8_t ni = (VIRTUAL_RES_X - 1) - i;
      uint8_t nj = (VIRTUAL_RES_Y - 1) - j;

      // The color of each point shifts over time, each at a different speed.
      uint16_t ms = millis();
      effects.leds[XY16(i, j)] += effects.ColorFromCurrentPalette(ms / 11);
      //effects.leds[XY16(j, i)] += effects.ColorFromCurrentPalette(ms / 13);   // this doesn't work for non-square matrices
      effects.leds[XY16(ni, nj)] += effects.ColorFromCurrentPalette(ms / 17);
      //effects.leds[XY16(nj, ni)] += effects.ColorFromCurrentPalette(ms / 29); // this doesn't work for non-square matrices
      effects.leds[XY16(i, nj)] += effects.ColorFromCurrentPalette(ms / 37);
      effects.leds[XY16(ni, j)] += effects.ColorFromCurrentPalette(ms / 41);

      
      effects.ShowFrame();
      return 0;
      
    }
};

#endif
