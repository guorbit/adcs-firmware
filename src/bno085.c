#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "pico/binary_info.h"
#include "bno085.h"

// to get acceleration x,y,z; yaw, pitch, roll; magnetic heading

static bool reset_occured = false;
static sh2_SensorValue_t sensorValue;
static sh2_SensorValue_t * sensor_value = &sensorValue // pointer to the struct provided by the sh2 driver

///// sh2 hal, this is the protocol running on the bno085
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

static void sh2_hal_close(sh2_Hal_t *self) {
    // so sh2 doesn't complain
}

static int sh2_hal_write(const uint8_t *buf, unsigned len){
    int write;

    write = i2c_write_timeout_us(
        BNO085_I2C, BNO085_ADDR,
        buf, len, false,
        I2C_TIMEOUT_US
    );
    
    if (write != (int)len) {
        return 0;
    }
    return write;
}

static int sh2_hal_read(uint8_t *i2c_buffer, unsigned len, uint32_t *timestamp_us) {
    uint8_t header[4]; // buffer for header
    int return_value;
    uint16_t packet_size;
    uint16_t data_size;

    return_value = i2c_read_timeout_us(BNO085_I2C, BNO085_ADDR, header, 4, false, I2C_TIMEOUT_US);

    if (return_value != 4) {
        return 0; // read failed, it should be a 4 byte header
    }

    // getting the packet size
    packet_size = (uint16_t)header[0] | ((uint16_t)header[1] << 8);
    packet_size &= ~0x8000;  // clear continue bit

    if (packet_size > len) {
        return 0;            // caller buffer too small
    }

    // add header into the buffer
    memcpy(i2c_buffer, header, 4);

    data_size = packet_size - 4; // everything but the 4 byte header

    if (data_size > 0) {
        return_value = i2c_read_timeout_us(BNO085_I2C, BNO085_ADDR, i2c_buffer + 4, data_size, false, I2C_TIMEOUT_US);

        if (return_value != data_size) {
            return 0;
        }
    }

    if (timestamp_us) {
        *timestamp_us = (uint32_t)to_us_since_boot(get_absolute_time());
    }

    return packet_size;
}

///// sh2 callbacks
static void sh2_async_event_handler(void *cookie, sh2_AsyncEvent_t *event) {
    if (event->eventId == SH2_RESET) {
        reset_occurred = true;
    }
}

static void sh2_sensor_event_handler(void *cookie, sh2_SensorEvent_t *event) {
    if (sh2_decodeSensorEvent(sensor_value, event) != SH2_OK) {
        sensor_value->timestamp = 0;
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

void bno085_poll(void) {
    // Ask sh2 to process any pending transfers
    sh2_service();

    // If sensorHandler ran and decoded something, _sensor_value->timestamp != 0
    if (_sensor_value && _sensor_value->timestamp != 0) {
        bno085_has_new = true;
    }
}
