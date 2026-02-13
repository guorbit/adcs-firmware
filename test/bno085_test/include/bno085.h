#ifndef BNO085_H
#define BNO085_H

#include <stdint.h>
#include <stdbool.h>
#include "sh2.h"
#include "sh2_hal.h"


#define BNO085_I2C        i2c0
#define BNO085_ADDR       0x4A

#define BNO085_SDA_PIN    8
#define BNO085_SCL_PIN    9
#define BNO085_INT_PIN    6   // set to -1 if not used
#define BNO085_RST_PIN    5   // currently connected by jumper cable...
#define I2C_BUFFER_MAX    512 // 512 bytes max for buffer
#define I2C_TIMEOUT_US    200000 // 100000 μs = 0.1s, exceeding results in pico timeout error

extern volatile bool reset_occurred;
typedef struct {
    float accel[3];
    float quat[4];
    float mag[3];
} bno085_state_t;

// sh2 hal
int sh2chal_open(sh2_Hal_t *self);
void sh2_hal_close(sh2_Hal_t *self);
int sh2_hal_write(sh2_Hal_t *self, uint8_t *buf, unsigned len);
int sh2_hal_read(sh2_Hal_t *self, uint8_t *i2c_buffer, unsigned len, uint32_t *timestamp_us);

// sh2 callbacks
static void sh2_async_event_handler(void *cookie, sh2_AsyncEvent_t *event);
static void sh2_sensor_event_handler(void *cookie, sh2_SensorEvent_t *event);
uint32_t hal_getTimeUs(sh2_Hal_t *self);

// bno085 driver
bool bno085_init(void);
bool enable_report(sh2_SensorId_t sensorId, uint32_t interval_us);
bool bno085_get_report(bno085_state_t *out);
void bno085_poll(void);

#endif