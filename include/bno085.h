#ifndef BNO085_H
#define BNO085_H

#include <stdint.h>
#include <stdbool.h>


#define BNO085_I2C        i2c0
#define BNO085_ADDR       0x4A

#define BNO085_SDA_PIN    4
#define BNO085_SCL_PIN    5
#define BNO085_RST_PIN    6   // set to -1 if not used
#define I2C_BUFFER_MAX    32
#define I2C_TIMEOUT_US    100000 // 100000 μs = 0.1s

// sh2 hal
static int sh2chal_open(sh2_Hal_t *self);
static void sh2_hal_close(sh2_Hal_t *self);
static int sh2_hal_write(sh2_Hal_t *self, uint8_t *buf, unsigned len);
static int sh2_hal_read(sh2_Hal_t *self, uint8_t *i2c_buffer, unsigned len, uint32_t *timestamp_us);

// sh2 callbacks
static void sh2_async_event_handler(void *cookie, sh2_AsyncEvent_t *event);
static void sh2_sensor_event_handler(void *cookie, sh2_SensorEvent_t *event);
static uint32_t hal_getTimeUs(sh2_Hal_t *self);

// bno085 driver
bool bno085_init(void);
static bool enable_report(sh2_SensorId_t sensorId, uint32_t interval_us);
bool bno085_get_quaternion(float *qw, float *qx, float *qy, float *qz);
void bno085_poll(void);

#endif