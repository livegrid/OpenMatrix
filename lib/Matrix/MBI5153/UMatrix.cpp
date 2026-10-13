#include "UMatrix.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

// Gamma 2.2, 8-bit channel to 14-bit PWM (0..16383).
// The MBI5153 is configured for 14-bit grayscale and the frame sender already
// clocks 16 bits per channel. Writing only the top 8 left the lower 6 PWM bits
// at 0 (steps of 64). Brightness scaling then rounded a code of 1 down to 0,
// so dark water became black speckles.
const uint16_t GAMMA14[256] = {
        0,     0,     0,     1,     2,     3,     4,     6,     8,    10,    13,    16,    20,    23,    28,    32,
       37,    42,    48,    54,    61,    67,    75,    82,    90,    99,   108,   117,   127,   137,   148,   159,
      170,   182,   195,   207,   221,   234,   249,   263,   278,   294,   310,   326,   343,   361,   379,   397,
      416,   435,   455,   475,   496,   517,   539,   561,   583,   607,   630,   654,   679,   704,   730,   756,
      783,   810,   838,   866,   894,   924,   953,   983,  1014,  1045,  1077,  1110,  1142,  1176,  1210,  1244,
     1279,  1314,  1350,  1387,  1424,  1461,  1499,  1538,  1577,  1617,  1657,  1698,  1739,  1781,  1823,  1866,
     1910,  1954,  1998,  2044,  2089,  2136,  2182,  2230,  2278,  2326,  2375,  2425,  2475,  2525,  2577,  2629,
     2681,  2734,  2787,  2841,  2896,  2951,  3007,  3063,  3120,  3178,  3236,  3295,  3354,  3414,  3474,  3535,
     3596,  3658,  3721,  3784,  3848,  3913,  3978,  4043,  4110,  4176,  4244,  4312,  4380,  4449,  4519,  4589,
     4660,  4732,  4804,  4876,  4950,  5024,  5098,  5173,  5249,  5325,  5402,  5479,  5557,  5636,  5715,  5795,
     5876,  5957,  6039,  6121,  6204,  6287,  6372,  6456,  6542,  6628,  6714,  6801,  6889,  6978,  7067,  7156,
     7247,  7337,  7429,  7521,  7614,  7707,  7801,  7896,  7991,  8087,  8183,  8281,  8378,  8477,  8576,  8675,
     8775,  8876,  8978,  9080,  9183,  9286,  9390,  9495,  9600,  9706,  9812,  9920, 10027, 10136, 10245, 10355,
    10465, 10576, 10688, 10800, 10913, 11027, 11141, 11256, 11371, 11487, 11604, 11721, 11840, 11958, 12078, 12198,
    12318, 12440, 12562, 12684, 12807, 12931, 13056, 13181, 13307, 13433, 13561, 13688, 13817, 13946, 14076, 14206,
    14337, 14469, 14602, 14735, 14868, 15003, 15138, 15273, 15410, 15547, 15685, 15823, 15962, 16102, 16242, 16383,
};

UMatrix::UMatrix() {
  fontSize = 2;
  rotation = 0;

  background =
      new GFX_Layer(PANEL_RES_X, PANEL_RES_Y,
                    [this](int16_t x, int16_t y, uint8_t r, uint8_t g,
                           uint8_t b) { mbi_set_pixel(x, y, r, g, b); });

  foreground =
      new GFX_Layer(PANEL_RES_X, PANEL_RES_Y,
                    [this](int16_t x, int16_t y, uint8_t r, uint8_t g,
                           uint8_t b) { mbi_set_pixel(x, y, r, g, b); });

  gfx_compositor = new GFX_LayerCompositor(
      [this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        mbi_set_pixel(x, y, r, g, b);
      });
}

void UMatrix::init() {
  // Step 1) Allocate raw buffer space for MBI5153 greyscale / MBI chip
  // command /  pixel memory
  dma_grey_buffer_parallel_bit_length =
      ((PANEL_SCAN_LINES * PANEL_MBI_RES_X * 16));
  dma_grey_buffer_size =
      sizeof(ESP32_GREY_DMA_STORAGE_TYPE) *
      dma_grey_buffer_parallel_bit_length;  // add some extract blank data at
                                            // the end for time to latch if
                                            // we're updating the greyscale
                                            // buffers
  log_d(
      "Allocating greyscale DMA memory buffer. Size of memory required: "
      "%lu bytes.",
      dma_grey_buffer_size);

  // Malloc Greyscale / Command DMA Memory
  dma_grey_gpio_data = (ESP32_GREY_DMA_STORAGE_TYPE*)heap_caps_malloc(
      dma_grey_buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
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
  bus_cfg.pin_d13 = -1;  // DCLK potentially if we need to manually generate
  bus_cfg.pin_d14 = -1;
  bus_cfg.pin_d15 = -1;

  dma_bus.config(bus_cfg);
  dma_bus.setup_lcd_dma_periph();

  // Setup SPI DMA Output for GCLK and Address Lines
  spi_setup();

  updateRegisters();

  update();

  initialized = true;
  // If rotation was updated earlier (e.g. ADXL in SensorTask before init), sync hardware now.
  refreshMatrixConfig();
}

uint8_t UMatrix::getXResolution() {
  return PANEL_RES_X;
}

uint8_t UMatrix::getYResolution() {
  return PANEL_RES_Y;
}

void UMatrix::drawPixelRGB888(uint16_t x, uint16_t y, uint8_t r_data,
                              uint8_t g_data, uint8_t b_data) {
  mbi_set_pixel(x, y, r_data, g_data, b_data);
}

void UMatrix::transform(int16_t& x, int16_t& y, int16_t& w, int16_t& h) {
  int16_t temp;
  switch (rotation) {
    case 1:  // 90 degrees clockwise
      temp = x;
      x = y;
      y = PANEL_RES_X - 1 - temp;
      break;
    case 2:  // 180 degrees
      x = PANEL_RES_X - 1 - x;
      y = PANEL_RES_Y - 1 - y;
      break;
    case 3:  // 270 degrees clockwise
      temp = x;
      x = PANEL_RES_Y - 1 - y;
      y = temp;
      break;
    default:  // No rotation
      break;
  }
}

void UMatrix::setBrightness(uint8_t newBrightness) {
  brightness = newBrightness;
}

uint8_t UMatrix::getBrightness() const {
  return brightness;
}

void UMatrix::setRotation(uint8_t newRotation) {
  if (newRotation < 4 && newRotation != rotation) {
    rotation = newRotation;
    // SensorTask can call setRotation before DisplayTask finishes init(); DMA
    // buffers are not allocated until then — skip hardware refresh until ready.
    if (initialized) {
      refreshMatrixConfig();
    }
  }
}

void UMatrix::rotate90() {
  rotation = (rotation + 1) % 4;
}

void UMatrix::clearScreen() {
  memset(dma_grey_gpio_data, 0, dma_grey_buffer_size);
}

void UMatrix::update() {
  assert(initialized);

  mbi_update_frame(true);
  spi_transfer_loop_stop();
  mbi_v_sync_dma();
  spi_transfer_loop_start();

  // log_e("tsfr count: %d", dma_bus.get_transfer_count());

  clearScreen();
}

void UMatrix::mbi_update_frame(bool configure_latches) {
  int counter = 0;
  for (int row = 0; row < PANEL_SCAN_LINES; row++) {
    for (int chan = 0; chan < PANEL_MBI_LED_CHANS; chan++) {
      for (int ic = 0; ic < PANEL_MBI_CHAIN_LEN;
           ic++) {  // number of chained ICs

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

  // log_d(TAG, "Sending greyscale data buffer out via LCD DMA.");
  dma_bus.send_stuff_once(dma_grey_gpio_data, dma_grey_buffer_size,
                          true);  // sending payload hence TRUE
}

void UMatrix::updateRegisters() {
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

  memset(dma_grey_gpio_data, 0,
         dma_grey_buffer_size);  // accidenty display configuration registers
                                 // id we don't clear th
}

void UMatrix::refreshMatrixConfig() {
  mbi_pre_active_dma();
  mbi_send_config_reg1_dma();
}

void UMatrix::mbi_set_pixel(uint8_t x, uint8_t y, uint8_t _r_data,
                            uint8_t _g_data, uint8_t _b_data) {
  if (x >= PANEL_RES_X || x < 0 || y >= PANEL_RES_Y || y < 0)
    return;

  int16_t _x = x, _y = y;
  int16_t _w = PANEL_RES_X, _h = PANEL_RES_Y;
  transform(_x, _y, _w, _h);

  if (_x >= PANEL_MBI_RES_X || _y >= PANEL_MBI_RES_Y) {
    log_e("Pixel position out of bounds: x=%u, y=%u", _x, _y);
    return;
  }

  // Scale in 14-bit so a dim code stays non-zero, then left-align into the
  // 16-bit shift word. 14-bit mode ignores the bottom two bits.
  uint16_t r_pwm = (uint16_t)(((uint32_t)GAMMA14[_r_data] * brightness) >> 8) << 2;
  uint16_t g_pwm = (uint16_t)(((uint32_t)GAMMA14[_g_data] * brightness) >> 8) << 2;
  uint16_t b_pwm = (uint16_t)(((uint32_t)GAMMA14[_b_data] * brightness) >> 8) << 2;

  _x += 2;  // offset for missing pixels on the left

  uint16_t _colourbitsoffset = (_y / PANEL_SCAN_LINES) * 3;
  uint16_t _colourbitsclear = ~(0b111 << _colourbitsoffset);

  uint16_t g_gpio_bitmask = BIT_G1 << _colourbitsoffset;
  uint16_t b_gpio_bitmask = BIT_B1 << _colourbitsoffset;
  uint16_t r_gpio_bitmask = BIT_R1 << _colourbitsoffset;

  int y_normalised = _y % PANEL_SCAN_LINES;
  int bit_start_pos = (1280 * y_normalised) + ((_x % 16) * 80) + ((_x / 16) * 16);

  int subpixel_colour_bit = 16;
  while (subpixel_colour_bit > 0) {
    subpixel_colour_bit--;
    dma_grey_gpio_data[bit_start_pos] &= _colourbitsclear;

    uint16_t mask = (uint16_t)1 << subpixel_colour_bit;

    if (g_pwm & mask) {
      dma_grey_gpio_data[bit_start_pos] |= g_gpio_bitmask;
    }
    if (b_pwm & mask) {
      dma_grey_gpio_data[bit_start_pos] |= b_gpio_bitmask;
    }
    if (r_pwm & mask) {
      dma_grey_gpio_data[bit_start_pos] |= r_gpio_bitmask;
    }

    bit_start_pos++;
  }
}

void UMatrix::mbi_pre_active_dma() {
  // log_d("Sending MBI Pre-Active.");
  int payload_length = 0;
  for (int i = 0; i < 14; i++) {
    dma_grey_gpio_data[payload_length++] = BIT_LAT;
  }

  for (int i = 0; i < 2; i++) {
    dma_grey_gpio_data[payload_length++] = 0x00;
  }

  dma_bus.send_stuff_once(dma_grey_gpio_data,
                          payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_v_sync_dma() {
  // log_d("Sending MBI Vert Sync.");
  int payload_length = 600;
  memset(dma_grey_gpio_data, 0,
         payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE));

  int start_pos = payload_length - (payload_length / 2);
  for (int i = 0; i < 3; i++) {
    dma_grey_gpio_data[start_pos++] = BIT_LAT;
  }
  dma_grey_gpio_data[start_pos] = 0x00;

  dma_bus.send_stuff_once(dma_grey_gpio_data,
                          payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_soft_reset_dma() {
  // log_d("Sending MBI Soft Reset.");
  int payload_length = 0;
  for (int i = 0; i < 10; i++) {
    dma_grey_gpio_data[payload_length++] = BIT_LAT;
  }
  dma_grey_gpio_data[payload_length++] = 0x00;

  dma_bus.send_stuff_once(dma_grey_gpio_data,
                          payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_set_config_dma(unsigned int& dma_output_pos,
                                 uint16_t config_reg, bool latch, bool reg2) {
  // log_d("Sending MBI Config DMA.");
  int latch_trigger_point = reg2 ? 8 : 4;

  for (int bit = 15; bit >= 0; bit--) {
    int bitval = (config_reg >> bit) & 1;
    uint16_t mbi_rgb_sdi_val = bitval ? BIT_ALL_RGB : 0;

    if (bit < latch_trigger_point && latch) {
      mbi_rgb_sdi_val |= BIT_LAT;
    }

    dma_grey_gpio_data[dma_output_pos++] = mbi_rgb_sdi_val;
  }
}

void UMatrix::mbi_send_config_reg1_dma() {
  uint16_t config_reg1_val = 0;
  int ghost_elimination = ghost_elimination_ON;
  int line_num = PANEL_SCAN_LINES - 1;
  int gray_scale = gray_scale_14;
  int gclk_multiplier = gclk_multiplier_OFF;
  int current = brightness_base;

  config_reg1_val = (ghost_elimination << 14) | (line_num << 8) |
                    (gray_scale << 7) | (gclk_multiplier << 6) | current;

  unsigned int dma_output_pos = 0;
  for (int i = 0; i < PANEL_MBI_CHAIN_LEN; i++) {
    mbi_set_config_dma(dma_output_pos, config_reg1_val,
                       i == (PANEL_MBI_CHAIN_LEN - 1), false);
  }

  dma_bus.send_stuff_once(dma_grey_gpio_data,
                          dma_output_pos * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_send_config_reg2_dma() {
  uint16_t config_reg2_val = 0b1001000000011110;
  unsigned int dma_output_pos = 0;

  for (int i = 0; i < PANEL_MBI_CHAIN_LEN; i++) {
    mbi_set_config_dma(dma_output_pos, config_reg2_val,
                       i == (PANEL_MBI_CHAIN_LEN - 1), true);
  }

  dma_bus.send_stuff_once(dma_grey_gpio_data,
                          dma_output_pos * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}
