#include "sh2.h"
#include "sh2_hal.h"
#include "sh2_SensorValue.h"

#include <stdint.h>
#include <stdbool.h>

/* ===================== HAL ===================== */

static int sh2_hal_open(sh2_Hal_t *self) {
    // init I2C, reset BNO085
    return 0;
}

static void sh2_hal_close(sh2_Hal_t *self) {
}

static int sh2_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    // i2c write
    return len;
}

static int sh2_hal_read(sh2_Hal_t *self, uint8_t *pBuffer,
                        unsigned len, uint32_t *t_us) {
    // i2c read
    // *t_us = timestamp if you care
    return len;
}

static void sh2_hal_delay_us(sh2_Hal_t *self, uint32_t us) {
    // sleep_us(us)
}

static uint32_t sh2_hal_get_time_us(sh2_Hal_t *self) {
    // return time_us_32()
    return 0;
}

/* HAL descriptor passed to SH-2 */
static sh2_Hal_t hal = {
    .open = sh2_hal_open,
    .close = sh2_hal_close,
    .read = sh2_hal_read,
    .write = sh2_hal_write,
    .delay_us = sh2_hal_delay_us,
    .getTimeUs = sh2_hal_get_time_us
};

/* ================= SH-2 CALLBACKS ================= */

static float quat_w, quat_x, quat_y, quat_z;
static bool quat_valid = false;

static void sensor_handler(void *cookie, sh2_SensorEvent_t *event) {
    sh2_SensorValue_t value;
    sh2_decodeSensorEvent(&value, event);

    if (value.sensorId == SH2_ROTATION_VECTOR) {
        quat_w = value.un.rotationVector.real;
        quat_x = value.un.rotationVector.i;
        quat_y = value.un.rotationVector.j;
        quat_z = value.un.rotationVector.k;
        quat_valid = true;
    }
}

static void async_event_handler(void *cookie, sh2_AsyncEvent_t *event) {
    // optional: reset, errors, etc.
}

/* ================= DRIVER API ================= */

bool bno085_init(void) {
    if (sh2_open(&hal, async_event_handler, NULL) != SH2_OK)
        return false;

    sh2_setSensorCallback(sensor_handler, NULL);

    return true;
}

void bno085_enable_rotation_vector(uint32_t interval_us) {
    sh2_SensorConfig_t cfg = {0};
    cfg.reportInterval_us = interval_us;
    sh2_setSensorConfig(SH2_ROTATION_VECTOR, &cfg);
}

void bno085_poll(void) {
    sh2_service();
}

bool bno085_get_quaternion(float *w, float *x, float *y, float *z) {
    if (!quat_valid) return false;

    *w = quat_w;
    *x = quat_x;
    *y = quat_y;
    *z = quat_z;
    return true;
}

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BNO085_I2C        i2c0
#define BNO085_ADDR       0x4A

#define BNO085_SDA_PIN    4
#define BNO085_SCL_PIN    5
#define BNO085_RST_PIN    6   // set to -1 if not used

/* ================= SH2 HAL ================= */

static int sh2_hal_open(sh2_Hal_t *self) {
    // I2C init
    i2c_init(BNO085_I2C, 400 * 1000);
    gpio_set_function(BNO085_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(BNO085_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(BNO085_SDA_PIN);
    gpio_pull_up(BNO085_SCL_PIN);

#if (BNO085_RST_PIN >= 0)
    gpio_init(BNO085_RST_PIN);
    gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);

    // Hard reset (recommended)
    gpio_put(BNO085_RST_PIN, 0);
    sleep_ms(10);
    gpio_put(BNO085_RST_PIN, 1);
    sleep_ms(50);
#endif

    return 0;
}

static void sh2_hal_close(sh2_Hal_t *self) {
    // nothing needed on Pico
}

static int sh2_hal_write(sh2_Hal_t *self, uint8_t *buf, unsigned len) {
    int ret = i2c_write_blocking(
        BNO085_I2C,
        BNO085_ADDR,
        buf,
        len,
        false
    );
    return (ret < 0) ? 0 : ret;
}

static int sh2_hal_read(sh2_Hal_t *self,
                        uint8_t *buf,
                        unsigned len,
                        uint32_t *t_us) {
    int ret = i2c_read_blocking(
        BNO085_I2C,
        BNO085_ADDR,
        buf,
        len,
        false
    );

    if (t_us)
        *t_us = time_us_32();

    return (ret < 0) ? 0 : ret;
}

static void sh2_hal_delay_us(sh2_Hal_t *self, uint32_t us) {
    sleep_us(us);
}

static uint32_t sh2_hal_get_time_us(sh2_Hal_t *self) {
    return time_us_32();
}
