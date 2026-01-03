#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "adcs_config.h"
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

static int sh2_hal_write(const uint8_t *buf, unsigned len){
    size_t i2c_buffer_max = maxBufferSize();
    uint16_t write_size = (len > i2c_buffer_max) ? i2c_buffer_max : len;

    if (!i2c_write(buf, write_size, true, NULL, 0)) {
        return 0;
    }
    return write_size;
}

static int sh2_hal_read(uint8_t *pBuffer, unsigned len, uint32_t *timestamp_us) {
    uint8_t header[4];
    if (!i2c_read(header, 4, true)) {
        return 0;
    }

    uint16_t packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    packet_size &= ~0x8000;  // clear continue bit

    if (packet_size > len) {
        return 0;            // caller buffer too small
    }

    size_t i2c_buffer_max = maxBufferSize();
    uint16_t cargo_remaining = packet_size;
    uint8_t i2c_buffer[i2c_buffer_max];
    uint16_t read_size;
    uint16_t cargo_read_amount = 0;
    bool first_read = true;

    while (cargo_remaining > 0) {
        if (first_read) {
            read_size = (cargo_remaining > i2c_buffer_max) ? i2c_buffer_max : cargo_remaining;
        } else {
            read_size = (cargo_remaining + 4 > i2c_buffer_max) ? i2c_buffer_max : (cargo_remaining + 4);
        }

        if (!i2c_read(i2c_buffer, read_size, true)) {
            return 0;
        }

        if (first_read) {
            cargo_read_amount = read_size;
            memcpy(pBuffer, i2c_buffer, cargo_read_amount);
            first_read = false;
        } else {
            cargo_read_amount = read_size - 4;
            memcpy(pBuffer, i2c_buffer + 4, cargo_read_amount);
        }

        pBuffer += cargo_read_amount;
        cargo_remaining -= cargo_read_amount;
    }

    if (timestamp_us) {
        *timestamp_us = to_us_since_boot(get_absolute_time());
    }

    return packet_size;
}

static int sh2_hal_delay_us(uint32_t us) {
    sleep_us(us);
    return 0;
}


///// bno085 hal
static void bno085_hal_reset(void) {
    // if you have a reset GPIO, do the same sequence, otherwise leave empty
    // or just power-cycle externally.
}

static void bno085_hal_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}
 // optional, so i wont after i read it

///// sh2 callbacks
static void sh2_async_event_handler(void *cookie, sh2_AsyncEvent_t *event) {
    if (event->eventId == SH2_RESET) {
        _reset_occurred = true;
    }
}

static void sh2_sensor_event_handler(void *cookie, sh2_SensorEvent_t *event) {
    if (sh2_decodeSensorEvent(_sensor_value, event) != SH2_OK) {
        _sensor_value->timestamp = 0;
    }
}


///// bno085 driver
bool bno085_init(i2c_inst_t *i2c, uint8_t addr) {
    _i2cPort = i2c;
    _deviceAddress = addr;

    // Optional: check device present
    uint8_t dummy;
    int res = i2c_read_timeout_us(_i2cPort, _deviceAddress, &dummy, 1, false, CONFIG::I2C_TIMEOUT_US);
    if (res <= 0) {
        return false;
    }

    // Fill HAL struct (names must match your sh2.h)
    _HAL.open      = sh2chal_open;
    _HAL.close     = sh2_hal_close;      // or &sh2_hal_close if signature matches
    _HAL.read      = sh2_hal_read;
    _HAL.write     = sh2_hal_write;
    _HAL.getTimeUs = hal_getTimeUs;      // from section 2.5

    bno085_hal_reset();                  // hardware reset if you have it

    // Open SH2
    int status = sh2_open(&_HAL, sh2_async_event_handler, NULL);
    if (status != SH2_OK) {
        return false;
    }

    // Probe product IDs (sanity check)
    sh2_ProductIds_t prodIds;
    memset(&prodIds, 0, sizeof(prodIds));
    status = sh2_getProdIds(&prodIds);
    if (status != SH2_OK) {
        return false;
    }

    // Register sensor callback
    sh2_setSensorCallback(sh2_sensor_event_handler, NULL);

    _sensor_value = &sensorValue;
    _reset_occurred = false;

    return true;
}

static bool enable_report(sh2_SensorId_t sensorId, uint32_t interval_us) {
    sh2_SensorConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.reportInterval_us = interval_us;
    cfg.changeSensitivityEnabled = false;
    cfg.wakeupEnabled = false;
    cfg.alwaysOnEnabled = false;
    cfg.batchInterval_us = 0;
    cfg.sensorSpecific = 0;

    int status = sh2_setSensorConfig(sensorId, &cfg);
    return (status == SH2_OK);
}

bool bno085_enable_rotation_vector(uint32_t interval_us) {
    return enable_report(SH2_ROTATION_VECTOR, interval_us);
}

bool bno085_get_quaternion(float *qw, float *qx, float *qy, float *qz) {
    if (!_sensor_value || _sensor_value->sensorId != SH2_ROTATION_VECTOR) {
        return false;
    }

    if (!bno085_has_new) {
        return false;
    }

    *qw = _sensor_value->un.rotationVector.real;
    *qx = _sensor_value->un.rotationVector.i;
    *qy = _sensor_value->un.rotationVector.j;
    *qz = _sensor_value->un.rotationVector.k;

    bno085_has_new = false;   // consume the sample
    return true;
}


static bool bno085_has_new = false;

bool bno085_data_ready(void) {
    return bno085_has_new;
}

void bno085_poll(void) {
    // Ask sh2 to process any pending transfers
    sh2_service();

    // If sensorHandler ran and decoded something, _sensor_value->timestamp != 0
    if (_sensor_value && _sensor_value->timestamp != 0) {
        bno085_has_new = true;
    }
}
