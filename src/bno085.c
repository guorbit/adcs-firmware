#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "adcs_defines.h"
#include <string.h>
#include "pico/binary_info.h"

///// sh2 hal
static int sh2chal_open(sh2_Hal_t *self) {
    // Serial.println("I2C HAL open");


    uint8_t softreset_pkt[] = {
    0x05,  // length LSB
    0x00,  // length MSB
    0x01,  // control channel
    0x00,  // sequence
    0x01   // reset command
    };
    // initialise bool as false
    bool success = false;
    for (uint8_t attempts = 0; attempts < 5; attempts++) {
        if (i2c_write(softreset_pkt, 5)) {
            success = true;
            break;
        }
        sleep_ms(30);
    }
    if (!success) // <- if success = false
    return -1;
    sleep_ms(300);
    return 0;
}

static void sh2_hal_close(void) {
    // if empty why here...
}

static int sh2_hal_write(const uint8_t *buf, unsigned len);
static int sh2_hal_read(uint8_t *buf, unsigned len, uint32_t *timestamp_us);
static int sh2_hal_delay_us(uint32_t us);

///// bno085 hal
static void bno085_hal_reset(void);
static void bno085_hal_delay_ms(uint32_t ms); // optional, so i wont after i read it

///// sh2 callbacks
static void sh2_sensor_event_handler(void *cookie, sh2_sensorevent_t *event);
static void sh2_async_event_handler(void *cookie, sh2_asyncevent_t *event);

///// bno085 driver
bool bno085_init(void);
bool bno085_enable_rotation_vector(uint32_t interval_us);
bool bno085_get_quaternion(float *qw, float *qx, float *qy, float *qz);
void bno085_poll(void);
bool bno085_data_ready(void);