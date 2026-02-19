#ifndef I2C_UTILS_H
#define I2C_UTILS_H

#include "hardware/i2c.h"

// Scan the I2C bus for devices. Prints found addresses to stdout.
// Returns the number of devices found.
int i2c_bus_scan(i2c_inst_t *i2c, int sda_pin, int scl_pin);

// Attempt to recover a stuck I2C bus
void i2c_bus_reset(i2c_inst_t *i2c, int sda_pin, int scl_pin); 

#endif
