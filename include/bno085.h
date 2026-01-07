#ifndef BNO085_H
#define BNO085_H

#include <stdint.h>
// sh2 libraries https://github.com/ceva-dsp/sh2
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "hardware/i2c.h"

#define BNO085_I2C        i2c0
#define BNO085_ADDR       0x4A

#define BNO085_SDA_PIN    4
#define BNO085_SCL_PIN    5
#define BNO085_RST_PIN    6   // set to -1 if not used
#define I2C_BUFFER_MAX    32
#define I2C_TIMEOUT_US    100000 // 100000 μs = 0.1s

#endif