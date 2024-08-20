#pragma once

#include <Arduino.h>
#include "UMatrixSettings.hpp"
#include "GFX_Layer.hpp"
#include "Matrix.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_task_wdt.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_dma_parallel16.hpp"
#include <array>

#include "sdkconfig.h"
#include "spi_dma_seg_tx_loop.h"

#include <iostream>

class UMatrix : public Matrix {
 private:
  bool initialized = false;

  // D<A Data to send
  Bus_Parallel16 dma_bus;

  ESP32_GREY_DMA_STORAGE_TYPE* dma_grey_gpio_data;

  size_t dma_grey_buffer_parallel_bit_length;  // Length in bits of the buffer
                                               // -> sequance of 13 x 16 bits (2
                                               // bytes) sent in parallel =
                                               // length value of 13
  size_t dma_grey_buffer_size;  // length of buffer in memory used -> sequance
                                // of 13 x 16 bits (2 bytes) sent in parallel =
                                // value of 26 bytes

  void transform(int16_t &x, int16_t &y, int16_t &w, int16_t &h);
  void mbi_update_frame(bool configure_latches);
  void mbi_set_pixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);
  void mbi_pre_active_dma();
  void mbi_v_sync_dma();
  void mbi_soft_reset_dma();
  void mbi_set_config_dma(unsigned int& dma_output_pos, uint16_t config_reg,
                          bool latch, bool reg2);
  void mbi_send_config_reg1_dma();
  void mbi_send_config_reg2_dma();
  void refreshMatrixConfig();

 public:
  UMatrix();

  void init() override;

  void setBrightness(uint8_t newBrightness) override;
  uint8_t getBrightness() const override;
  uint8_t getXResolution() override;
  uint8_t getYResolution() override;
  
  void setRotation(uint8_t newRotation) override;
  void rotate90() override;
  void clearScreen() override;

  void update() override;

  void updateRegisters();
  void drawPixelRGB888(uint16_t x, uint16_t y, uint8_t r_data, uint8_t g_data,
                     uint8_t b_data);
};