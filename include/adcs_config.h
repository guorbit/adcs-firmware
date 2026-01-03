#ifndef ADCS_CONFIG_H
#define ADCS_CONFIG_H

#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments

// Sensors (Pico = master)
#define I2C_SENSORS      i2c0
#define I2C_SENSORS_SDA  8   // GPIO8
#define I2C_SENSORS_SCL  9   // GPIO9

// OBC link (Pico = slave)
#define I2C_OBC          i2c1
#define I2C_OBC_SDA      14  // GPIO14
#define I2C_OBC_SCL      15  // GPIO15

#define ADCS_ADDR    0x08

#endif