#ifndef MATRIX_H
#define MATRIX_H

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

#include <iostream>

#include "Arduino.h"
#include "app_constants.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_dma_parallel16.hpp"
//#include "MatrixSettings.h"
#include "sdkconfig.h"
#include "spi_dma_seg_tx_loop.h"
#include <array>
#include <GFX_Lite.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

/*
  // MBI5152 Application Note V1.00- EN

  Section 2: The Setting of Gray Scale
  The setting of gray scale data describes as below.
  1. The sequence of input data starts from scan line 1 scan line 2….  scan line M-1 scan line M
  (M≦16)
  2. The data sequence of cascaded IC is ICnICn-1…. IC2IC1.
  3. The data sequence of each channel is ch15ch14…. ch0.
  4. The data length of each channel is 16-bits, and the default PWM mode is 16-bits. The sequence of gray
  sacle is bit15bit14bit13…bit0 as figure 4 shows. The 14-bits gray scale can be set through Bit[7]=1
  in configuration register 1, and the sequence of gray scale data is bit13bit12… bit0 0 0, the
  last 2-bits (LSB) are set to “0”.
  5. The frequency of GCLK must be higher than 20% of DCLK to get the correct gray scale data.
  6. LE executes the data latch to send gray scale data into SRAM. Each 16xN bits data needs a “data latch
  command”, where N means the number of cascaded driver.
  7. After the last data latch, it needs at least 50 GCLKs to read the gray scale data into internal display buffer
  before the Vsync command comes.
  8. Display is updated immediately when MBI5152 receives the Vsync signal.
  9. GCLK must keep at low level more than 7ns before MBI5152 receives the Vsync signal.
  10. The period of dead time (ie. The 1025th GCLK) must be larger than 100ns.

  The gray scale data needs the GCLK to save the data into SRAM. The frequency of GCLK must be higher
  than 20% of DCLK to get the correct data.

  // MBI5153
  After the last data latch command, it needs at least 50 GCLKs to read the gray scale data into internal display
  buffer before the Vsync command comes. And display is updated immediately until MBI5051/52/53 receives
  the Vsync signal (high pulse of LE pin is sampled by 3-DCLK rising edges), as figure 6 shows.

*/

static const char *TAG = "Matrix.h";


// Precalculated
static const uint16_t mbi_grey_data_latch_bits[] = {79, 159, 239, 319, 399, 479, 559, 639, 719, 799, 879, 959, 1039, 1119, 1199, 1279, 1359, 1439, 1519, 1599, 1679, 1759, 1839, 1919, 1999, 2079, 2159, 2239, 2319, 2399, 2479, 2559, 2639, 2719, 2799, 2879, 2959, 3039, 3119, 3199, 3279, 3359, 3439, 3519, 3599, 3679, 3759, 3839, 3919, 3999, 4079, 4159, 4239, 4319, 4399, 4479, 4559, 4639, 4719, 4799, 4879, 4959, 5039, 5119, 5199, 5279, 5359, 5439, 5519, 5599, 5679, 5759, 5839, 5919, 5999, 6079, 6159, 6239, 6319, 6399, 6479, 6559, 6639, 6719, 6799, 6879, 6959, 7039, 7119, 7199, 7279, 7359, 7439, 7519, 7599, 7679, 7759, 7839, 7919, 7999, 8079, 8159, 8239, 8319, 8399, 8479, 8559, 8639, 8719, 8799, 8879, 8959, 9039, 9119, 9199, 9279, 9359, 9439, 9519, 9599, 9679, 9759, 9839, 9919, 9999, 10079, 10159, 10239, 10319, 10399, 10479, 10559, 10639, 10719, 10799, 10879, 10959, 11039, 11119, 11199, 11279, 11359, 11439, 11519, 11599, 11679, 11759, 11839, 11919, 11999, 12079, 12159, 12239, 12319, 12399, 12479, 12559, 12639, 12719, 12799, 12879, 12959, 13039, 13119, 13199, 13279, 13359, 13439, 13519, 13599, 13679, 13759, 13839, 13919, 13999, 14079, 14159, 14239, 14319, 14399, 14479, 14559, 14639, 14719, 14799, 14879, 14959, 15039, 15119, 15199, 15279, 15359, 15439, 15519, 15599, 15679, 15759, 15839, 15919, 15999, 16079, 16159, 16239, 16319, 16399, 16479, 16559, 16639, 16719, 16799, 16879, 16959, 17039, 17119, 17199, 17279, 17359, 17439, 17519, 17599, 17679, 17759, 17839, 17919, 17999, 18079, 18159, 18239, 18319, 18399, 18479, 18559, 18639, 18719, 18799, 18879, 18959, 19039, 19119, 19199, 19279, 19359, 19439, 19519, 19599, 19679, 19759, 19839, 19919, 19999, 20079, 20159, 20239, 20319, 20399, 20479, 20559, 20639, 20719, 20799, 20879, 20959, 21039, 21119, 21199, 21279, 21359, 21439, 21519, 21599, 21679, 21759, 21839, 21919, 21999, 22079, 22159, 22239, 22319, 22399, 22479, 22559, 22639, 22719, 22799, 22879, 22959, 23039, 23119, 23199, 23279, 23359, 23439, 23519, 23599, 23679, 23759, 23839, 23919, 23999, 24079, 24159, 24239, 24319, 24399, 24479, 24559, 24639, 24719, 24799, 24879, 24959, 25039, 25119, 25199, 25279, 25359, 25439, 25519, 25599};

class Matrix : public GFX {

 public:
  Matrix() : GFX (PANEL_PHY_RES_X, PANEL_PHY_RES_Y) {  }

  ~Matrix() {
  }

  void initMatrix() 
  {
    // Step 1) Allocate raw buffer space for MBI5153 greyscale / MBI chip command /  pixel memory
    dma_grey_buffer_parallel_bit_length = ((PANEL_SCAN_LINES * PANEL_MBI_RES_X * 16));
    dma_grey_buffer_size = sizeof(ESP32_GREY_DMA_STORAGE_TYPE) * dma_grey_buffer_parallel_bit_length;  // add some extract blank data at the end for time to latch if we're updating the greyscale buffers
    ESP_LOGD(TAG, "Allocating greyscale DMA memory buffer. Size of memory required: %lu bytes.", dma_grey_buffer_size);

    // Malloc Greyscale / Command DMA Memory
    dma_grey_gpio_data = (ESP32_GREY_DMA_STORAGE_TYPE *)heap_caps_malloc(dma_grey_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    assert(dma_grey_gpio_data != nullptr);

    // Fill with zeros to start with
    memset(dma_grey_gpio_data, 0, dma_grey_buffer_size);

    // Setup LCD DMA and Output to GPIO
    auto bus_cfg = dma_bus.config();
    bus_cfg.pin_wr = MBI_DCLK;  // DCLK Pin
    bus_cfg.invert_pclk = false;
    bus_cfg.pin_d0 = MBI_G1;
    bus_cfg.pin_d1 = MBI_B1;
    bus_cfg.pin_d2 = MBI_R1;
    bus_cfg.pin_d3 = MBI_G2;
    bus_cfg.pin_d4 = MBI_B2;
    bus_cfg.pin_d5 = MBI_R2;
    bus_cfg.pin_d6 = MBI_G3;
    bus_cfg.pin_d7 = MBI_B3;
    bus_cfg.pin_d8 = MBI_R3;
    bus_cfg.pin_d9 = MBI_G4;
    bus_cfg.pin_d10 = MBI_B4;
    bus_cfg.pin_d11 = MBI_R4;
    bus_cfg.pin_d12 = MBI_LAT;  // Latch
    bus_cfg.pin_d13 = -1;       // DCLK potentially if we need to manually generate
    bus_cfg.pin_d14 = -1;
    bus_cfg.pin_d15 = -1;

    dma_bus.config(bus_cfg);
    dma_bus.setup_lcd_dma_periph();

    // Setup SPI DMA Output for GCLK and Address Lines
    spi_setup();

    updateRegisters();

    update();

    initialized = true;    
 
/*
    mbi_update_frame(true); // send blank frame    
    //ESP_LOGD(TAG, "Sending greyscale data buffer out via LCD DMA.");
    //dma_bus.send_stuff_once(dma_grey_gpio_data, counter2*sizeof(uint16_t), true); // sending payload hence TRUE 

    spi_transfer_loop_stop();
    mbi_v_sync_dma();
    spi_transfer_loop_restart();
*/    
  }

  void refreshMatrixConfig() {

    // spi_transfer_loop_stop();
    mbi_pre_active_dma();  // 14 clocks
    mbi_send_config_reg1_dma();

    // MBI Step 2) Some other register 2 hack to reduce ghosting.
    // mbi_pre_active_dma();
    // mbi_send_config_reg2_dma();
  }


/*
  // MBI5152 Application Note V1.00- EN
  
  <snipped>
  7. After the last data latch, it needs at least 50 GCLKs to read the gray scale data into internal display buffer
  before the Vsync command comes.
  8. Display is updated immediately when MBI5152 receives the Vsync signal.
  9. GCLK must keep at low level more than 7ns before MBI5152 receives the Vsync signal.
  10. The period of dead time (ie. The 1025th GCLK) must be larger than 100ns.

  The gray scale data needs the GCLK to save the data into SRAM. The frequency of GCLK must be higher
  than 20% of DCLK to get the correct data.

  // MBI5153
  After the last data latch command, it needs at least 50 GCLKs to read the gray scale data into internal display
  buffer before the Vsync command comes. And display is updated immediately until MBI5051/52/53 receives
  the Vsync signal (high pulse of LE pin is sampled by 3-DCLK rising edges), as figure 6 shows.

*/
  void update() {

    assert(initialized);

    mbi_update_frame(true);
    spi_transfer_loop_stop();
    mbi_v_sync_dma();
    spi_transfer_loop_start();        

    //log_e("tsfr count: %d", dma_bus.get_transfer_count());


    memset(dma_grey_gpio_data, 0, dma_grey_buffer_size);

  }

  void updateRegisters() 
  {
    spi_transfer_loop_start();  // start GCLK + Adress toggling

    // MBI Step 1) Set key registers, such as number of rows
    mbi_soft_reset_dma();  // 10 clocks
    mbi_pre_active_dma();  // 14 clocks
    mbi_send_config_reg1_dma();

    // MBI Step 2) Some other register 2 hack to reduce ghosting.
    mbi_pre_active_dma();
    mbi_send_config_reg2_dma();

    // MBI Step 3) Clean out any crap in the greyscale buffer
    mbi_soft_reset_dma();  // 10 clocks

    memset(dma_grey_gpio_data, 0, dma_grey_buffer_size);  // accidenty display configuration registers id we don't clear th
  }

  void setBrightness(uint8_t newBrightness) {
    // Map newBrightness from 0-255 to 0-63
    brightness = map(newBrightness, 0, 255, 0, 63);
    refreshMatrixConfig();
  }


  uint8_t getBrightness() const {
    return map(brightness, 0, 63, 0, 255);
  }

  uint8_t getXResolution() {
    return PANEL_PHY_RES_X;
  }

  uint8_t getYResolution() {
    return PANEL_PHY_RES_Y;
  }

  void drawPixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data) {
    writePixel(x, y, r_data, g_data, b_data);
  }

  void fillScreen(uint8_t r, uint8_t g, uint8_t b) {
    fillRectDMA(0, 0, PANEL_PHY_RES_X, PANEL_PHY_RES_Y, r, g, b);
  }

  void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t red, uint8_t green, uint8_t blue) {
    writeLine(x0, y0, x1, y1, red, green, blue);
  }

  void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b) {
    fillRectDMA(x, y, w, h, r, g, b);
  }

  void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t r, uint8_t g, uint8_t b) {
    fillTriangleDMA(x0, y0, x1, y1, x2, y2, r, g, b);
  }

  void drawCircle(int16_t x0, int16_t y0, int16_t rad, uint8_t r, uint8_t g, uint8_t b) {
    fillCircleDMA(x0, y0, rad, r, g, b);
  }

  void drawEllipse(int16_t x, int16_t y, int16_t rad, int16_t length, float angle, uint8_t r, uint8_t g, uint8_t b) {
    fillEllipseDMA(x, y, rad, length, angle, r, g, b);
  }

  void writePixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data) {
    mbi_set_pixel(x, y, r_data, g_data, b_data);
  }

  void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t red, uint8_t green, uint8_t blue) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
      _swap_int16_t(x0, y0);
      _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
      _swap_int16_t(x0, x1);
      _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
      ystep = 1;
    } else {
      ystep = -1;
    }

    mbi_set_pixel(x0, y0, red, green, blue);

    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }  // writeLine

  void hlineDMA(int16_t x_coord, int16_t y_coord, int16_t length, uint8_t red, uint8_t green, uint8_t blue) {
    if (x_coord + length < 1 || y_coord < 0 || length < 1 || x_coord >= PANEL_PHY_RES_X || y_coord >= PANEL_PHY_RES_Y)
      return;

    // Adjust length if starting x coordinate is negative
    if (x_coord < 0) {
      length += x_coord;
      x_coord = 0;
    }

    // Adjust length to not exceed panel width
    if (x_coord + length > PANEL_PHY_RES_X)
      length = PANEL_PHY_RES_X - x_coord;

    for (int16_t x = x_coord; x < x_coord + length; x++) {
      mbi_set_pixel(x, y_coord, red, green, blue);
    }
  }  // hlineDMA

  void vlineDMA(int16_t x_coord, int16_t y_coord, int16_t length, uint8_t red, uint8_t green, uint8_t blue) {
    if (x_coord < 0 || y_coord + length < 1 || length < 1 || x_coord >= PANEL_PHY_RES_X || y_coord >= PANEL_PHY_RES_Y)
      return;

    // Adjust length if starting y coordinate is negative
    if (y_coord < 0) {
      length += y_coord;
      y_coord = 0;
    }

    // Adjust length to not exceed panel height
    if (y_coord + length > PANEL_PHY_RES_Y)
      length = PANEL_PHY_RES_Y - y_coord;

    for (int16_t y = y_coord; y < y_coord + length; y++) {
      mbi_set_pixel(x_coord, y, red, green, blue);
    }
  }  // vlineDMA

  void fillRectDMA(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t g, uint8_t b) {
    // h-lines are >2 times faster than v-lines
    // so will use it only for tall rects with h >2w
    if (h > 2 * w) {
      // draw using v-lines
      do {
        --w;
        vlineDMA(x + w, y, h, r, g, b);
      } while (w);
    } else {
      // draw using h-lines
      do {
        --h;
        hlineDMA(x, y + h, w, r, g, b);
      } while (h);
    }
  }  // fillRectDMA

  void fillTriangleDMA(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                       int16_t x2, int16_t y2, uint8_t r, uint8_t g, uint8_t b) {
    int16_t a0, b0, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
      _swap_int16_t(y0, y1);
      _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
      _swap_int16_t(y2, y1);
      _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
      _swap_int16_t(y0, y1);
      _swap_int16_t(x0, x1);
    }

    if (y0 == y2) {  // Handle awkward all-on-same-line case as its own thing
      a0 = b0 = x0;
      if (x1 < a0)
        a0 = x1;
      else if (x1 > b0)
        b0 = x1;
      if (x2 < a0)
        a0 = x2;
      else if (x2 > b0)
        b0 = x2;
      hlineDMA(a0, y0, b0 - a0 + 1, r, g, b);
      return;
    }
    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
            dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
      last = y1;  // Include y1 scanline
    else
      last = y1 - 1;  // Skip it

    for (y = y0; y <= last; y++) {
      a0 = x0 + sa / dy01;
      b0 = x0 + sb / dy02;
      sa += dx01;
      sb += dx02;
      /* longhand:
      a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
      b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
      */
      if (a0 > b0)
        _swap_int16_t(a0, b0);
      hlineDMA(a0, y, b0 - a0 + 1, r, g, b);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
      a0 = x1 + sa / dy12;
      b0 = x0 + sb / dy02;
      sa += dx12;
      sb += dx02;
      /* longhand:
      a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
      b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
      */
      if (a0 > b0)
        _swap_int16_t(a0, b0);
      hlineDMA(a0, y, b0 - a0 + 1, r, g, b);
    }
  }

  void fillCircleHelperDMA(int16_t x0, int16_t y0, int16_t rad, uint8_t corners, int16_t delta, uint8_t r, uint8_t g, uint8_t b) {
    int16_t f = 1 - rad;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * rad;
    int16_t x = 0;
    int16_t y = rad;
    int16_t px = x;
    int16_t py = y;

    delta++;  // Avoid some +1's in the loop

    while (x < y) {
      if (f >= 0) {
        y--;
        ddF_y += 2;
        f += ddF_y;
      }
      x++;
      ddF_x += 2;
      f += ddF_x;
      // These checks avoid double-drawing certain lines, important
      // for the SSD1306 library which has an INVERT drawing mode.
      if (x < (y + 1)) {
        if (corners & 1)
          vlineDMA(x0 + x, y0 - y, 2 * y + delta, r, g, b);
        if (corners & 2)
          vlineDMA(x0 - x, y0 - y, 2 * y + delta, r, g, b);
      }
      if (y != py) {
        if (corners & 1)
          vlineDMA(x0 + py, y0 - px, 2 * px + delta, r, g, b);
        if (corners & 2)
          vlineDMA(x0 - py, y0 - px, 2 * px + delta, r, g, b);
        py = y;
      }
      px = x;
    }
  }

  void fillCircleDMA(int16_t x0, int16_t y0, int16_t rad, uint8_t r, uint8_t g, uint8_t b) {
    vlineDMA(x0, y0 - rad, 2 * rad + 1, r, g, b);
    fillCircleHelperDMA(x0, y0, rad, 3, 0, r, g, b);
  }

  void fillEllipseDMA(int16_t x, int16_t y, int16_t rad, int16_t length, float angle, uint8_t r, uint8_t g, uint8_t b) {
    // Calculate the rotation angle in radians
    float cosAngle = cos(angle);
    float sinAngle = sin(angle);

    int16_t _rad, _length;
    _rad = min(rad, length);
    _length = max(rad, length);

    // Calculate the step size and number of circles based on the rectangle's dimensions
    int numCircles = max(_rad, static_cast<int16_t>(_length / 2));  // Adjust the divisor to control circle density
    for (int i = -numCircles; i <= numCircles; i++) {
      float dx = i * cosAngle;
      float dy = i * sinAngle;

      // Calculate circle center
      int16_t circleX = x + dx;
      int16_t circleY = y + dy;

      // Draw circle
      fillCircleDMA(circleX, circleY, _rad, r, g, b);
    }
  }


  void drawPixel(int16_t x, int16_t y, CRGB color)
  {
    mbi_set_pixel(x, y, color.red, color.green, color.blue);
  }

  // includes 565 to 888 conversion
  void drawPixel(int16_t x, int16_t y, uint16_t color)
  {
    uint8_t r   = (color >> 8) & 0xf8;
    uint8_t  g  = (color >> 3) & 0xfc;
    uint8_t b   = (color << 3);
    r |= r >> 5;
    g |= g >> 6;
    b |= b >> 5;    
    mbi_set_pixel(x, y, r, g, b);
  }


 protected:

  bool initialized = false;

  // D<A Data to send
  Bus_Parallel16 dma_bus;

  ESP32_GREY_DMA_STORAGE_TYPE *dma_grey_gpio_data;

  size_t dma_grey_buffer_parallel_bit_length; // Length in bits of the buffer -> sequance of 13 x 16 bits (2 bytes) sent in parallel = length value of 13
  size_t dma_grey_buffer_size; // length of buffer in memory used -> sequance of 13 x 16 bits (2 bytes) sent in parallel = value of 26 bytes

  uint8_t brightness = 32;

  void mbi_update_frame(bool configure_latches) {

      int counter = 0;
      for (int row = 0; row < PANEL_SCAN_LINES; row++) {
        for (int chan = 0; chan < PANEL_MBI_LED_CHANS; chan++) {
          for (int ic = 0; ic < PANEL_MBI_CHAIN_LEN; ic++) {  // number of chained ICs

            // data latch on the last bit, when sending the last byte set latch=1
            int latch = 0;
            if (ic == 4) {
              latch = 1;
            }  // latch on last channel / ic

            int bit_offset = 16;
            while (bit_offset > 0)  // shift out MSB first per the documentation.
            {
              bit_offset--;  // start from 15

              if (latch == 1 && bit_offset == 0) {
                dma_grey_gpio_data[counter] |= BIT_LAT;
              }
              counter++;
            }
          }
        }
      }
   
    //log_d(TAG, "Sending greyscale data buffer out via LCD DMA.");
    dma_bus.send_stuff_once(dma_grey_gpio_data, dma_grey_buffer_size, true);  // sending payload hence TRUE

  }  // mbi_update_frame


  /*
    Gray Scale Mode and Scan-type S-PWM
    MBI5153 provides a selectable 14-bit or 13-bit gray scale by setting the configuration register1 bit [7]. The default
    value is set to ’0’ for 14-bit color depth. In 14-bit gray scale mode, users should still send 16-bit data with 2-bit ‘0’ in
    LSB bits. For example, {14’h1234, 2’h0}.
    MBI5153 has a smart S-PWM technology for scan type. With S-PWM, the total PWM cycles can be broken into MSB
    (Most Significant Bits) and LSB (Least Significant Bits) of gray scale cycles. The MSB information can be broken
    down into many refresh cycles to achieve overall same high bit resolution.

    The 16384 GCLKs (14-bit) PWM cycle of MBI5052/53 is divided into 32 sections, each section has 512 GCLKs.
  */

  void mbi_set_pixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data) {
    
    if (x >= PANEL_PHY_RES_X || y >= PANEL_PHY_RES_Y) {
      return;
    }

    x += 2;  // offset for missing pixels on the left

    // Calculate bitmasks
    uint16_t _colourbitsoffset = (y / PANEL_SCAN_LINES) * 3; // three is an important bit
    uint16_t _colourbitsclear  = ~(0b111 << _colourbitsoffset);  // invert

    uint16_t g_gpio_bitmask = BIT_G1 << _colourbitsoffset;  // bit 0
    uint16_t b_gpio_bitmask = BIT_B1 << _colourbitsoffset;  // bit 1
    uint16_t r_gpio_bitmask = BIT_R1 << _colourbitsoffset;  // bit 2    

    // Row offset + channel offset + individual IC LED offset
    // Calculate data array start position
    int y_normalised = y % PANEL_SCAN_LINES;  // Only have 20 rows of data...
    int bit_start_pos = (1280 * y_normalised) + ((x % 16) * 80) + ((x / 16) * 16);  

  /*
      MBI5153 provides a selectable 14-bit or 13-bit gray scale by setting the configuration register1 bit [7]. The default 
      value is set to ’0’ for 14-bit color depth. In 14-bit gray scale mode, users should still send 16-bit data with 2-bit ‘0’ in 
      LSB bits. For example, {14’h1234, 2’h0}. 
  */    

    // RGB colour data provided is only 8bits, so we'll fill it from bit 16 down to bit 8
    // 14-bit resolution = 16,384
    //  8-bit resolutoin = 255     
    int subpixel_colour_bit = 8;
    uint8_t mask;
    while (subpixel_colour_bit > 0)  // shift out MSB first per the documentation.
    {
      subpixel_colour_bit--;  // start from 7
      dma_grey_gpio_data[bit_start_pos] &= _colourbitsclear; // celear what was there before   

      mask = 1 << subpixel_colour_bit;      

      if (g_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= g_gpio_bitmask;
      }

      if (b_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= b_gpio_bitmask;
      }

      if (r_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= r_gpio_bitmask;
      }

      //ESP_LOGV(TAG, "Setting dma_grey_gpio_data from bit_start_pos %d. Value %d", bit_start_pos, dma_grey_gpio_data[bit_start_pos]);
      bit_start_pos++;
    }

    // We assumpt bit_start_postions that should be cleared, are.
        
 
  }  // mbi_set_pixel


  // About 10% slower than the new implementation.
  void mbi_set_pixel_old(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data) {
    
    if (x >= PANEL_PHY_RES_X || y >= PANEL_PHY_RES_Y) {
      return;
    }

    x += 2;  // offset for missing pixels on the left

    // 14-bit resolution = 16,384
    //  8-bit resolutoin = 255
    //  ... that's 64 times larger
    uint16_t g_14bit_data = (g_data * 64) << 2;
    uint16_t b_14bit_data = (b_data * 64) << 2;
    uint16_t r_14bit_data = (r_data * 64) << 2;  // could just bit shift by << 6  in total instead

    ESP_LOGV(TAG, "Converted 14bit colour value r: %d,  g: %d,  b: %d", g_14bit_data, g_14bit_data, b_14bit_data);

    // x and y positions start from 0, so 0-78 are valid values only
    uint16_t g_gpio_bitmask = BIT_G1;  // bit 0
    uint16_t b_gpio_bitmask = BIT_B1;  // bit 1
    uint16_t r_gpio_bitmask = BIT_R1;  // bit 2

    uint16_t _colourbitclear = BIT_RGB1_CLR, _colourbitoffset = 0;

    _colourbitoffset = (y / PANEL_SCAN_LINES) * 3;   // three is an important bit
    _colourbitclear = ~(0b111 << _colourbitoffset);  // invert

    g_gpio_bitmask = g_gpio_bitmask << _colourbitoffset;
    b_gpio_bitmask = b_gpio_bitmask << _colourbitoffset;
    r_gpio_bitmask = r_gpio_bitmask << _colourbitoffset;

    
      // if (y < 20) {
      // } else if (y < 40) {
      //   b_gpio_bitmask = BIT_B2;
      //   g_gpio_bitmask = BIT_G2;
      //   r_gpio_bitmask = BIT_R2;
      // } else if (y < 60)  {
      //   b_gpio_bitmask = BIT_B3;
      //   g_gpio_bitmask = BIT_G3;
      //   r_gpio_bitmask = BIT_R3;
      // } else {
      //   b_gpio_bitmask = BIT_B4;
      //   g_gpio_bitmask = BIT_G4;
      //   r_gpio_bitmask = BIT_R4;
      // }
     

    // Row offset + channel offset + individual IC LED offset
    int y_normalised = y % PANEL_SCAN_LINES;  // Only have 20 rows of data...
    int bit_start_pos = (1280 * y_normalised) + ((x % 16) * 80) + ((x / 16) * 16);

    int bit_offset = 16;
    while (bit_offset > 0)  // shift out MSB first per the documentation.
    {
      bit_offset--;  // start from 15

      uint16_t mask = 1 << bit_offset;

      dma_grey_gpio_data[bit_start_pos] &= _colourbitclear;  // clear relevant rgb bits

      if (g_14bit_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= g_gpio_bitmask;
      }

      if (b_14bit_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= b_gpio_bitmask;
      }

      if (r_14bit_data & mask) {
        dma_grey_gpio_data[bit_start_pos] |= r_gpio_bitmask;
      }

      ESP_LOGV(TAG, "Setting dma_grey_gpio_data from bit_start_pos %d. Value %d", bit_start_pos, dma_grey_gpio_data[bit_start_pos]);

      bit_start_pos++;
    }

  }  // mbi_set_pixel

  /*pre-activation command - sent before sending configuration register data*/
  void mbi_pre_active_dma() {
    ESP_LOGD(TAG, "Send MBI Pre-Active.");

    int payload_length = 0;  // length in parallel CLOCK cycles
    for (int i = 0; i < 14; i++) {
      dma_grey_gpio_data[payload_length] = BIT_LAT;
      payload_length++;
    }

    /* LE/LAT should be low for any rising edge of DCLK */
    for (int i = 0; i < 2; i++) {
      dma_grey_gpio_data[payload_length] = 0x00;
      payload_length++;
    }

    dma_bus.send_stuff_once(dma_grey_gpio_data, payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE), false);

    /*
      gpio_set_level((gpio_num_t)MBI_LAT, 1);
      mbi_clock(14);
      gpio_set_level((gpio_num_t)MBI_LAT, 0);
      mbi_clock(2);
    */
  }  // mbi_pre_active_dma

  /*vertical sync - updates frame data on outputs, used in conjunction with vertical scan*/
  // 9. GCLK must keep at low level more than 7ns before MBI5152 receives the Vsync signa
  // 7ns = 142 Mhz clock speed...
  /* After the last data latch command, it needs at least 50 GCLKs to read the gray scale data into internal display
  buffer before the Vsync command comes. And display is updated immediately until MBI5051/52/53 receives
  the Vsync signal (high pulse of LE pin is sampled by 3-DCLK rising edges), as figure 6 shows.
  */
  void mbi_v_sync_dma() {
    ESP_LOGV(TAG, "Send MBI Vert Sync.");
  
    /*
    int payload_length = 0;
    for (int i = 0; i < 3; i++) {
      dma_grey_gpio_data[payload_length] = BIT_LAT;
      payload_length++;
    }

    dma_grey_gpio_data[payload_length] = 0x00;
    payload_length++;
    */

    // Send the Vsync somewhere in the middle of the gclk data.
    int payload_length = 600; 
    memset(dma_grey_gpio_data, 0, payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE));

    int start_pos = payload_length - (payload_length/2); 
    for (int i = 0; i < 3; i++) {
      dma_grey_gpio_data[start_pos++] = BIT_LAT;
    }

    dma_grey_gpio_data[start_pos++] = 0x00;

    dma_bus.send_stuff_once(dma_grey_gpio_data, payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE), false);

    /*
      gpio_set_level((gpio_num_t)MBI_LAT, 1);
      mbi_clock(2);
      gpio_set_level((gpio_num_t)MBI_LAT, 0);
    */
  }  // mbi_v_sync_dma

  void mbi_soft_reset_dma() {
    /* “Software reset” command makes MBI5153 go back to the initial state except configuration register value. After this
    command is received, the output channels will be turned off and will display again with last gray-scale value after
    new “Vsync” command is received.
    */
    ESP_LOGD(TAG, "Send MBI Soft Reset.");

    int payload_length = 0;
    for (int i = 0; i < 10; i++) {
      dma_grey_gpio_data[payload_length] = BIT_LAT;
      payload_length++;
    }

    dma_grey_gpio_data[payload_length] = 0x00;
    payload_length++;

    dma_bus.send_stuff_once(dma_grey_gpio_data, payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE), false);

    /*
      gpio_set_level((gpio_num_t)MBI_LAT, 1);
      mbi_clock(10);
      gpio_set_level((gpio_num_t)MBI_LAT, 0);
    */
  }  // mbi_soft_reset_dma

  /*отправка регистра конфигурации*/
  /* Need to set config for all MBI input on the panel at once as they all share the same latch pin */
  void mbi_set_config_dma(unsigned int &dma_output_pos, uint16_t config_reg, bool latch, bool reg2) {
    ESP_LOGD(TAG, "Send MBI config");

    // Number of DCLK Rising Edge when LE is asserted
    // Write Configuration 1 = 4
    // Write Configuration 2 = 8
    int latch_trigger_point = (reg2) ? 8 : 4;

    for (int bit = 15; bit >= 0; bit--) {
      int bitval = ((config_reg >> bit) & 1);

      uint16_t mbi_rgb_sdi_val = 0;
      if (bitval) {
        mbi_rgb_sdi_val = BIT_ALL_RGB;  // all <BI colour channels get the same config for now.
      }

      if ((bit < latch_trigger_point) && (latch == true)) {  // for reg1, data latch on the last 4 bits when latch is true (on last send)

        // std::cout << "Latch." << std::endl;
        mbi_rgb_sdi_val |= BIT_LAT;
      }

      dma_grey_gpio_data[dma_output_pos++] = mbi_rgb_sdi_val;

    }  // iterate through bits

  }  // mbi_send_config_dma

  void mbi_send_config_reg1_dma() {
    static int config_payload_bit_pos = 0;
    // Step 1) Send configuration for
    uint16_t config_reg1_val = 0;

    int ghost_elimination = ghost_elimination_ON;
    int line_num = PANEL_SCAN_LINES - 1;
    int gray_scale = gray_scale_14;
    int gclk_multiplier = gclk_multiplier_OFF;
    int current = current_1;  // change as required by channel

    // Documentation says set bits E and F of Config1 Reg to 1
    config_reg1_val = (config_reg1_val | (ghost_elimination << 14) | (line_num << 8) | (gray_scale << 7) | (gclk_multiplier << 6) | (current));

    unsigned int dma_output_pos = 0;
    for (int i = 0; i < PANEL_MBI_CHAIN_LEN; i++) {
      mbi_set_config_dma(dma_output_pos, config_reg1_val, (i == (PANEL_MBI_CHAIN_LEN - 1)), false);  // on last mbi chip, latch
    }

    dma_bus.send_stuff_once(dma_grey_gpio_data, dma_output_pos * sizeof(ESP32_GREY_DMA_STORAGE_TYPE), false);
  }  // mbi_send_config_reg1_dma

  /*настройка регистра конфигурации*/
  void mbi_send_config_reg2_dma() {
    // F E D C B A 9 8 7 6 5 4 3 2 1 0
    // uint16_t config_reg2_val = 0b0001000000010000; // default value apparently

    uint16_t config_reg2_val = 0b1001000000011110;  // removes ghosting except for red
    unsigned int dma_output_pos = 0;

    for (int i = 0; i < PANEL_MBI_CHAIN_LEN; i++) {
      mbi_set_config_dma(dma_output_pos, config_reg2_val, (i == (PANEL_MBI_CHAIN_LEN - 1)), true);  // on last mbi chip, latch
    }

    dma_bus.send_stuff_once(dma_grey_gpio_data, dma_output_pos * sizeof(ESP32_GREY_DMA_STORAGE_TYPE), false);
  }  // mbi_send_config_reg2_dma
};

#endif  // MATRIX_H
