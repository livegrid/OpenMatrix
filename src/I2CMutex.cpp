#include "I2CMutex.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static SemaphoreHandle_t s_i2cMutex = nullptr;

void i2cMutexInit() {
  if (!s_i2cMutex) {
    s_i2cMutex = xSemaphoreCreateMutex();
  }
}

void i2cLock() {
  if (s_i2cMutex) {
    xSemaphoreTake(s_i2cMutex, portMAX_DELAY);
  }
}

void i2cUnlock() {
  if (s_i2cMutex) {
    xSemaphoreGive(s_i2cMutex);
  }
}
