#ifndef I2C_H
#define I2C_H

#define ADCS_I2C        i2c0
#define ADCS_SDA_PIN    8
#define ADCS_SCL_PIN    9

#define I2C_BUFFER_MAX    512 // 512 bytes max for buffer
#define I2C_TIMEOUT_US    100000 // 100000 μs = 0.1s, exceeding results in pico timeout error

void adcs_i2c_init(void);

#endif