#include "UMatrix.h"

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif

static const uint16_t mbi_grey_data_latch_bits[] = {
    79,    159,   239,   319,   399,   479,   559,   639,   719,   799,   879,
    959,   1039,  1119,  1199,  1279,  1359,  1439,  1519,  1599,  1679,  1759,
    1839,  1919,  1999,  2079,  2159,  2239,  2319,  2399,  2479,  2559,  2639,
    2719,  2799,  2879,  2959,  3039,  3119,  3199,  3279,  3359,  3439,  3519,
    3599,  3679,  3759,  3839,  3919,  3999,  4079,  4159,  4239,  4319,  4399,
    4479,  4559,  4639,  4719,  4799,  4879,  4959,  5039,  5119,  5199,  5279,
    5359,  5439,  5519,  5599,  5679,  5759,  5839,  5919,  5999,  6079,  6159,
    6239,  6319,  6399,  6479,  6559,  6639,  6719,  6799,  6879,  6959,  7039,
    7119,  7199,  7279,  7359,  7439,  7519,  7599,  7679,  7759,  7839,  7919,
    7999,  8079,  8159,  8239,  8319,  8399,  8479,  8559,  8639,  8719,  8799,
    8879,  8959,  9039,  9119,  9199,  9279,  9359,  9439,  9519,  9599,  9679,
    9759,  9839,  9919,  9999,  10079, 10159, 10239, 10319, 10399, 10479, 10559,
    10639, 10719, 10799, 10879, 10959, 11039, 11119, 11199, 11279, 11359, 11439,
    11519, 11599, 11679, 11759, 11839, 11919, 11999, 12079, 12159, 12239, 12319,
    12399, 12479, 12559, 12639, 12719, 12799, 12879, 12959, 13039, 13119, 13199,
    13279, 13359, 13439, 13519, 13599, 13679, 13759, 13839, 13919, 13999, 14079,
    14159, 14239, 14319, 14399, 14479, 14559, 14639, 14719, 14799, 14879, 14959,
    15039, 15119, 15199, 15279, 15359, 15439, 15519, 15599, 15679, 15759, 15839,
    15919, 15999, 16079, 16159, 16239, 16319, 16399, 16479, 16559, 16639, 16719,
    16799, 16879, 16959, 17039, 17119, 17199, 17279, 17359, 17439, 17519, 17599,
    17679, 17759, 17839, 17919, 17999, 18079, 18159, 18239, 18319, 18399, 18479,
    18559, 18639, 18719, 18799, 18879, 18959, 19039, 19119, 19199, 19279, 19359,
    19439, 19519, 19599, 19679, 19759, 19839, 19919, 19999, 20079, 20159, 20239,
    20319, 20399, 20479, 20559, 20639, 20719, 20799, 20879, 20959, 21039, 21119,
    21199, 21279, 21359, 21439, 21519, 21599, 21679, 21759, 21839, 21919, 21999,
    22079, 22159, 22239, 22319, 22399, 22479, 22559, 22639, 22719, 22799, 22879,
    22959, 23039, 23119, 23199, 23279, 23359, 23439, 23519, 23599, 23679, 23759,
    23839, 23919, 23999, 24079, 24159, 24239, 24319, 24399, 24479, 24559, 24639,
    24719, 24799, 24879, 24959, 25039, 25119, 25199, 25279, 25359, 25439, 25519,
    25599};

UMatrix::UMatrix() {
  fontSize = 2;
  rotation = 0;
  brightness = 32;

  background =
      new GFX_Layer(PANEL_RES_X, PANEL_RES_Y,
                    [this](int16_t x, int16_t y, uint8_t r, uint8_t g,
                           uint8_t b) { drawPixelRGB888(x, y, r, g, b); });

  foreground =
      new GFX_Layer(PANEL_RES_X, PANEL_RES_Y,
                    [this](int16_t x, int16_t y, uint8_t r, uint8_t g,
                           uint8_t b) { drawPixelRGB888(x, y, r, g, b); });

  gfx_compositor = new GFX_LayerCompositor(
      [this](int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
        drawPixelRGB888(x, y, r, g, b);
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
  if (x >= PANEL_RES_X || y >= PANEL_RES_Y)
    return;

  int16_t _x = x, _y = y;
  int16_t _w = PANEL_RES_X, _h = PANEL_RES_Y;
  transform(_x, _y, _w, _h);

  // mbi_set_pixel_old(_x, _y, r_data, g_data, b_data);
  mbi_set_pixel(_x, _y, r_data, g_data, b_data);
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
  brightness = map(newBrightness, 0, 255, 0, 63);
  refreshMatrixConfig();
}

uint8_t UMatrix::getBrightness() const {
  return map(brightness, 0, 63, 0, 255);
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
  // do nothing, not needed
}

void UMatrix::update() {
  assert(initialized);

  mbi_update_frame(true);
  spi_transfer_loop_stop();
  mbi_v_sync_dma();
  spi_transfer_loop_start();

  // log_e("tsfr count: %d", dma_bus.get_transfer_count());

  memset(dma_grey_gpio_data, 0, dma_grey_buffer_size);
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

void UMatrix::mbi_set_pixel(uint8_t x, uint8_t y, uint8_t r_data,
                            uint8_t g_data, uint8_t b_data) {
  // Use PANEL_MBI_RES_X and PANEL_MBI_RES_Y for bounds checking
  if (x >= PANEL_MBI_RES_X || y >= PANEL_MBI_RES_Y) {
    log_e("Pixel position out of bounds: x=%u, y=%u", x, y);
    return;
  }

  x += 2;  // offset for missing pixels on the left

  // Calculate bitmasks
  uint16_t _colourbitsoffset =
      (y / PANEL_SCAN_LINES) * 3;  // three is an important bit
  uint16_t _colourbitsclear = ~(0b111 << _colourbitsoffset);  // invert

  uint16_t g_gpio_bitmask = BIT_G1 << _colourbitsoffset;  // bit 0
  uint16_t b_gpio_bitmask = BIT_B1 << _colourbitsoffset;  // bit 1
  uint16_t r_gpio_bitmask = BIT_R1 << _colourbitsoffset;  // bit 2

  // Row offset + channel offset + individual IC LED offset
  // Calculate data array start position
  int y_normalised = y % PANEL_SCAN_LINES;  // Only have 20 rows of data...
  int bit_start_pos = (1280 * y_normalised) + ((x % 16) * 80) + ((x / 16) * 16);

  /*
      MBI5153 provides a selectable 14-bit or 13-bit gray scale by setting the
     configuration register1 bit [7]. The default value is set to ’0’ for 14-bit
     color depth. In 14-bit gray scale mode, users should still send 16-bit data
     with 2-bit ‘0’ in LSB bits. For example, {14’h1234, 2’h0}.
  */

  // RGB colour data provided is only 8bits, so we'll fill it from bit 16 down
  // to bit 8 14-bit resolution = 16,384
  //  8-bit resolutoin = 255
  int subpixel_colour_bit = 8;
  uint8_t mask;
  while (subpixel_colour_bit > 0)  // shift out MSB first per the documentation.
  {
    subpixel_colour_bit--;  // start from 7
    dma_grey_gpio_data[bit_start_pos] &=
        _colourbitsclear;  // celear what was there before

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

    // ESP_LOGV(TAG, "Setting dma_grey_gpio_data from bit_start_pos %d. Value
    // %d", bit_start_pos, dma_grey_gpio_data[bit_start_pos]);
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
  int current = brightness;

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
