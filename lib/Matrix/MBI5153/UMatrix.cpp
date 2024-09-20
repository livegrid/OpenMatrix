#include "UMatrix.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

// CIE - Lookup table for converting between perceived LED brightness and PWM
// https://gist.github.com/mathiasvr/19ce1d7b6caeab230934080ae1f1380e

// 8 bit lookup table as we're only passing through 8 bit colour data so pointless to scale it up
// to 14-bit or 16-bit values for mbi_set_pixel as it'll just take longer to load the data to the
// panel because of more bit iterations.
const uint16_t CIE[256] = {
    0,    0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    3,    3,    3,    3,    3,    3,    3,    3,    4,
    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,    6,    6,    6,    6,    6,    7,
    7,    7,    7,    8,    8,    8,    8,    9,    9,    9,   10,   10,   10,   10,   11,   11,
   11,   12,   12,   12,   13,   13,   13,   14,   14,   15,   15,   15,   16,   16,   17,   17,
   17,   18,   18,   19,   19,   20,   20,   21,   21,   22,   22,   23,   23,   24,   24,   25,
   25,   26,   26,   27,   28,   28,   29,   29,   30,   31,   31,   32,   32,   33,   34,   34,
   35,   36,   37,   37,   38,   39,   39,   40,   41,   42,   43,   43,   44,   45,   46,   47,
   47,   48,   49,   50,   51,   52,   53,   54,   54,   55,   56,   57,   58,   59,   60,   61,
   62,   63,   64,   65,   66,   67,   68,   70,   71,   72,   73,   74,   75,   76,   77,   79,
   80,   81,   82,   83,   85,   86,   87,   88,   90,   91,   92,   94,   95,   96,   98,   99,
  100,  102,  103,  105,  106,  108,  109,  110,  112,  113,  115,  116,  118,  120,  121,  123,
  124,  126,  128,  129,  131,  132,  134,  136,  138,  139,  141,  143,  145,  146,  148,  150,
  152,  154,  155,  157,  159,  161,  163,  165,  167,  169,  171,  173,  175,  177,  179,  181,
  183,  185,  187,  189,  191,  193,  196,  198,  200,  202,  204,  207,  209,  211,  214,  216,
  218,  220,  223,  225,  228,  230,  232,  235,  237,  240,  242,  245,  247,  250,  252,  255,
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
    refreshMatrixConfig();
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

  uint8_t r_data = CIE[_r_data];
  uint8_t g_data = CIE[_g_data];
  uint8_t b_data = CIE[_b_data];

  int16_t _x = x, _y = y;
  int16_t _w = PANEL_RES_X, _h = PANEL_RES_Y;
  transform(_x, _y, _w, _h);
  CRGB color = CRGB(r_data, g_data, b_data);
  color.nscale8(brightness);

  // Merged mbi_set_pixel functionality
  if (_x >= PANEL_MBI_RES_X || _y >= PANEL_MBI_RES_Y) {
    log_e("Pixel position out of bounds: x=%u, y=%u", _x, _y);
    return;
  }

  _x += 2;  // offset for missing pixels on the left

  uint16_t _colourbitsoffset = (_y / PANEL_SCAN_LINES) * 3;
  uint16_t _colourbitsclear = ~(0b111 << _colourbitsoffset);

  uint16_t g_gpio_bitmask = BIT_G1 << _colourbitsoffset;
  uint16_t b_gpio_bitmask = BIT_B1 << _colourbitsoffset;
  uint16_t r_gpio_bitmask = BIT_R1 << _colourbitsoffset;

  int y_normalised = _y % PANEL_SCAN_LINES;
  int bit_start_pos = (1280 * y_normalised) + ((_x % 16) * 80) + ((_x / 16) * 16);

  int subpixel_colour_bit = 8;
  uint8_t mask;
  while (subpixel_colour_bit > 0) {
    subpixel_colour_bit--;
    dma_grey_gpio_data[bit_start_pos] &= _colourbitsclear;

    mask = 1 << subpixel_colour_bit;

    if (color.g & mask) {
      dma_grey_gpio_data[bit_start_pos] |= g_gpio_bitmask;
    }
    if (color.b & mask) {
      dma_grey_gpio_data[bit_start_pos] |= b_gpio_bitmask;
    }
    if (color.r & mask) {
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
