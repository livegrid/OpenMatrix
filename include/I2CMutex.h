#pragma once

// Serialize all TwoWire (I2C) access across FreeRTOS tasks. ESP32 Wire is not
// thread-safe; concurrent transactions from TOF, SCD40, BH1750, ADXL345, etc.
// corrupt the bus and can crash or hang the device.

void i2cMutexInit();
void i2cLock();
void i2cUnlock();
