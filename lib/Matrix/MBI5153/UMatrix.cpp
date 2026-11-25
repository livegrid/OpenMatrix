#pragma GCC optimize("O2")
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

  // PSRAM keeps internal RAM free for WiFi; only update() reads it, once per frame.
  framebuffer_size = sizeof(Rgb8) * PANEL_MBI_RES_Y * PANEL_MBI_RES_X;
  framebuffer = (Rgb8(*)[PANEL_MBI_RES_X])heap_caps_calloc(
      1, framebuffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (framebuffer == nullptr) {
    framebuffer = (Rgb8(*)[PANEL_MBI_RES_X])heap_caps_calloc(
        1, framebuffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  assert(framebuffer != nullptr);

  dma_cmd_data = (ESP32_GREY_DMA_STORAGE_TYPE*)heap_caps_calloc(
      MBI_CMD_BUFFER_ENTRIES, sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
      MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  assert(dma_cmd_data != nullptr);

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

  initialized = true;
  update();
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
    // Called from SensorTask. Only the display task may drive the LCD DMA bus,
    // so the config refresh is applied on the next update().
    config_refresh_pending = true;
  }
}

void UMatrix::rotate90() {
  rotation = (rotation + 1) % 4;
}

void UMatrix::clearScreen() {
  memset(framebuffer, 0, framebuffer_size);
}

void UMatrix::update() {
  assert(initialized);

  if (config_refresh_pending) {
    config_refresh_pending = false;
    refreshMatrixConfig();
  }

  flushFramebuffer();
  mbi_update_frame();
  if (spi_transfer_loop_stop(MBI_SCANS_PER_FRAME) != ESP_OK) {
    log_e("GCLK loop stop failed");
  }
  mbi_v_sync_dma();
  if (spi_transfer_loop_start() != ESP_OK) {
    log_e("GCLK loop start failed, retrying");
    spi_transfer_loop_stop(0);
    if (spi_transfer_loop_start() != ESP_OK) {
      log_e("GCLK loop restart failed");
    }
  }

  // log_e("tsfr count: %d", dma_bus.get_transfer_count());

  clearScreen();
}

void UMatrix::mbi_update_frame() {
  dma_bus.send_stuff_once(dma_grey_gpio_data, dma_grey_buffer_size,
                          true);  // sending payload hence TRUE
}

// Transpose a 16x16 bit matrix in place. On entry each value is one GPIO
// lane's 16-bit PWM word (lane n at index 15 - n); on exit each value is one
// parallel DMA sample, MSB first.
static inline void transpose16x16(uint16_t A[16]) {
  uint16_t t;
  for (int k = 0; k < 8; k++) {
    t = (A[k] ^ (A[k + 8] >> 8)) & 0x00FF;
    A[k] ^= t;
    A[k + 8] ^= (t << 8);
  }
  for (int k = 0; k < 16; k += 8) {
    for (int j = 0; j < 4; j++) {
      t = (A[k + j] ^ (A[k + j + 4] >> 4)) & 0x0F0F;
      A[k + j] ^= t;
      A[k + j + 4] ^= (t << 4);
    }
  }
  for (int k = 0; k < 16; k += 4) {
    for (int j = 0; j < 2; j++) {
      t = (A[k + j] ^ (A[k + j + 2] >> 2)) & 0x3333;
      A[k + j] ^= t;
      A[k + j + 2] ^= (t << 2);
    }
  }
  for (int k = 0; k < 16; k += 2) {
    t = (A[k] ^ (A[k + 1] >> 1)) & 0x5555;
    A[k] ^= t;
    A[k + 1] ^= (t << 1);
  }
}

// Rewrites every word of the greyscale buffer, including the data latch on
// the last bit of the last chained IC for every row/channel.
void UMatrix::flushFramebuffer() {
  // Scale in 14-bit so a dim code stays non-zero, then left-align into the
  // 16-bit shift word. 14-bit mode ignores the bottom two bits.
  uint16_t pwm[256];
  const uint32_t b = brightness;
  for (int i = 0; i < 256; i++) {
    pwm[i] = (uint16_t)(((uint32_t)GAMMA14[i] * b) >> 8) << 2;
  }

  constexpr int kGroups = PANEL_MBI_RES_Y / PANEL_SCAN_LINES;
  constexpr int kWordsPerIc = PANEL_MBI_CHAIN_LEN * 16;
  constexpr int kWordsPerRow = PANEL_MBI_LED_CHANS * kWordsPerIc;

  for (int row = 0; row < PANEL_SCAN_LINES; row++) {
    ESP32_GREY_DMA_STORAGE_TYPE* row_out = dma_grey_gpio_data + row * kWordsPerRow;
    for (int x = 0; x < PANEL_MBI_RES_X; x++) {
      uint16_t lanes[16] = {};
      uint16_t any = 0;
      for (int g = 0; g < kGroups; g++) {
        const Rgb8& p = framebuffer[g * PANEL_SCAN_LINES + row][x];
        lanes[15 - g * 3] = pwm[p.g];
        lanes[14 - g * 3] = pwm[p.b];
        lanes[13 - g * 3] = pwm[p.r];
        any |= lanes[15 - g * 3] | lanes[14 - g * 3] | lanes[13 - g * 3];
      }
      if (any) {
        transpose16x16(lanes);
      }

      ESP32_GREY_DMA_STORAGE_TYPE* out =
          row_out + (x % PANEL_MBI_LED_CHANS) * kWordsPerIc + (x / PANEL_MBI_LED_CHANS) * 16;
      for (int s = 0; s < 16; s++) {
        out[s] = lanes[s];
      }
      if (x / PANEL_MBI_LED_CHANS == PANEL_MBI_CHAIN_LEN - 1) {
        out[15] |= BIT_LAT;
      }
    }
  }
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
}

void UMatrix::refreshMatrixConfig() {
  mbi_pre_active_dma();
  mbi_send_config_reg1_dma();
}

void UMatrix::mbi_set_pixel(uint8_t x, uint8_t y, uint8_t _r_data,
                            uint8_t _g_data, uint8_t _b_data) {
  if (x >= PANEL_RES_X || y >= PANEL_RES_Y)
    return;

  int16_t _x = x, _y = y;
  int16_t _w = PANEL_RES_X, _h = PANEL_RES_Y;
  transform(_x, _y, _w, _h);

  _x += 2;  // offset for missing pixels on the left
  if (_x >= PANEL_MBI_RES_X || _y >= PANEL_MBI_RES_Y) {
    return;
  }

  framebuffer[_y][_x] = {_r_data, _g_data, _b_data};
}

void UMatrix::mbi_pre_active_dma() {
  // log_d("Sending MBI Pre-Active.");
  int payload_length = 0;
  for (int i = 0; i < 14; i++) {
    dma_cmd_data[payload_length++] = BIT_LAT;
  }

  for (int i = 0; i < 2; i++) {
    dma_cmd_data[payload_length++] = 0x00;
  }

  dma_bus.send_stuff_once(dma_cmd_data,
                          payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_v_sync_dma() {
  // log_d("Sending MBI Vert Sync.");
  int payload_length = MBI_CMD_BUFFER_ENTRIES;
  memset(dma_cmd_data, 0,
         payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE));

  int start_pos = payload_length - (payload_length / 2);
  for (int i = 0; i < 3; i++) {
    dma_cmd_data[start_pos++] = BIT_LAT;
  }
  dma_cmd_data[start_pos] = 0x00;

  dma_bus.send_stuff_once(dma_cmd_data,
                          payload_length * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}

void UMatrix::mbi_soft_reset_dma() {
  // log_d("Sending MBI Soft Reset.");
  int payload_length = 0;
  for (int i = 0; i < 10; i++) {
    dma_cmd_data[payload_length++] = BIT_LAT;
  }
  dma_cmd_data[payload_length++] = 0x00;

  dma_bus.send_stuff_once(dma_cmd_data,
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

    dma_cmd_data[dma_output_pos++] = mbi_rgb_sdi_val;
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

  dma_bus.send_stuff_once(dma_cmd_data,
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

  dma_bus.send_stuff_once(dma_cmd_data,
                          dma_output_pos * sizeof(ESP32_GREY_DMA_STORAGE_TYPE),
                          false);
}
