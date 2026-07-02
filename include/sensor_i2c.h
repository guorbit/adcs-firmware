#ifndef I2C_H
#define I2C_H

#define SENSOR_I2C        i2c0
#define SENSOR_SDA      8
#define SENSOR_SCL      9

#define I2C_BUFFER_MAX    512 // 512 bytes max for buffer
#define I2C_TIMEOUT_US    100000 // 100000 μs = 0.1s, exceeding results in pico timeout error

void sensor_i2c_init(void);

#endif