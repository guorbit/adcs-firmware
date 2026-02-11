#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/i2c_slave.h"
#include "pico/binary_info.h"
#include "bno085.h"

// sh2 libraries https://github.com/ceva-dsp/sh2
// based on this library https://github.com/robotcopper/BNO08x_Pico_Library, i took out a lot of stuff though
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"

// to get acceleration x,y,z; yaw, pitch, roll; magnetic heading

static sh2_Hal_t HAL;
volatile bool reset_occurred = false;
static bool bno085_has_new = false;
static sh2_SensorValue_t sensorValue; // initialise struct of type sh2_SensorValue_t
static sh2_SensorValue_t * sensor_value = &sensorValue; // sensor_value is pointer to the struct provided by the sh2 driver
// probably need to provide a definition for the sensor_value struct, for now look at the sh2.h file



///// sh2 hal, this is the protocol running on the bno085
int sh2chal_open(sh2_Hal_t *self) {
    // Serial.println("I2C HAL open");
    
    // from pico sdk
    gpio_init(BNO085_INT_PIN);
    gpio_set_dir(BNO085_INT_PIN, GPIO_IN);
    gpio_pull_up(BNO085_INT_PIN); // since int pin is active low

    uint8_t softreset_pkt[] = {
    0x05,  // length LSB
    0x00,  // length MSB
    0x01,  // control channel
    0x00,  // sequence
    0x01   // reset command
    };

    // initialise bool as false
    bool success = false;

    // attempt to reset sensor
    for (uint8_t attempts = 0; attempts < 5; attempts++) {
        printf("attempt %d: sending reset...\n", attempts+1);
        sleep_ms(10);
        int result = i2c_write_timeout_us(BNO085_I2C, BNO085_ADDR, softreset_pkt, 5, false, I2C_TIMEOUT_US);
        
        // if result is 5 bytes (softrest_pkt)
        if (result == 5) {
            printf("reset packet sent\n");
            success = true;
            break;
            }
            else{
                printf("i2c write failed. result code: %d \n", result);
            }
        sleep_ms(30);
    }
    if (!success) {// <- if success = false
        printf("could not reset bno085\n");
        return -1;
    }

    sleep_ms(300);
    return 0;
}

void sh2_hal_close(sh2_Hal_t *self) {
    // so sh2 doesn't complain
}

int sh2_hal_write(sh2_Hal_t *self, uint8_t *buf, unsigned len){
    int write;
    write = i2c_write_timeout_us(BNO085_I2C, BNO085_ADDR, buf, len, false, I2C_TIMEOUT_US);
    
    if (write != (int)len) {
        printf("debug: i2c write failed. requested length: %d, error code: %d \n", len, write);
        return 0;
    }
    return len;
}

int sh2_hal_read(sh2_Hal_t *self, uint8_t *i2c_buffer, unsigned len, uint32_t *timestamp_us) {
    int return_value;
    uint16_t packet_size;
    uint16_t data_size;
    

    if (gpio_get(BNO085_INT_PIN) != 0) {
        // printf("pin is high, nothing to read \n");
        return 0;
    }

    return_value = i2c_read_timeout_us(BNO085_I2C, BNO085_ADDR, i2c_buffer, len, false, I2C_TIMEOUT_US);

    if (return_value <= 0) {
        return 0;
    }

    // getting the packet size
    packet_size = (uint16_t)i2c_buffer[0] | ((uint16_t)i2c_buffer[1] << 8);
    packet_size &= ~0x8000;  // clear continue bit

    printf("pkt: len = %d, channel = %d \n", packet_size, i2c_buffer[2]);

    // caller buffer wrong size
    if (packet_size > len || packet_size == 0) {
        printf("packet size %d not correct \n", packet_size);
        return 0;            
    }

    if (timestamp_us) {
        *timestamp_us = (uint32_t)to_us_since_boot(get_absolute_time());
    }

    if (packet_size > 0 && packet_size < 100) { // Filter out the huge boot packets
         printf("DEBUG: read packet of %d bytes. header channel: %d\n", packet_size, i2c_buffer[2]);
    }

    return packet_size;
}

///// sh2 callbacks
static void sh2_async_event_handler(void *cookie, sh2_AsyncEvent_t *event) {
    if (event->eventId == SH2_RESET) {
        printf("reset detected \n");
        reset_occurred = true;
    }
}

static void sh2_sensor_event_handler(void *cookie, sh2_SensorEvent_t *event) {
    if (sh2_decodeSensorEvent(sensor_value, event) == SH2_OK) {
        bno085_has_new = true;
    }
}

uint32_t hal_getTimeUs(sh2_Hal_t *self)
{
    return (uint32_t)to_us_since_boot(get_absolute_time());
}


///// bno085 driver
bool bno085_init(void) {
    // check device present, does bno ack ?
    uint8_t dummy;
    int res = i2c_read_timeout_us(BNO085_I2C, BNO085_ADDR, &dummy, 1, false, I2C_TIMEOUT_US);
    if (res <= 0) {
        printf("failed: i2c read error. error code: %d\n", res);
        sleep_ms(1000);
        return false;
    }

    printf("success: device acknowledged\n");

    // fill HAL struct
    HAL.open      = sh2chal_open;
    HAL.close     = sh2_hal_close;
    HAL.read      = sh2_hal_read;
    HAL.write     = sh2_hal_write;
    HAL.getTimeUs = hal_getTimeUs;

    // open sh2, sh2_open is from the ceva sh2 api (sh2.c)
    int status = sh2_open(&HAL, sh2_async_event_handler, NULL);
    if (status != SH2_OK) {
        printf("failed: sh2_open returned error code: %d\n", status);
        sleep_ms(1000);
        return false;
    }

    // register sensor callback, also from the ceva api
    sh2_setSensorCallback(sh2_sensor_event_handler, NULL);
    reset_occurred = false;

    printf("bno085 initialisation complete\n");
    return true;
}

// which report to enable e.g. SH2_ACCELEROMETER, SH2_ROTATION_VECTOR and report interval
bool enable_report(sh2_SensorId_t sensorId, uint32_t interval_us) {
    sh2_SensorConfig_t cfg; // create struct called cfg
    memset(&cfg, 0, sizeof(cfg));

    cfg.reportInterval_us = interval_us;
    // disabled
    cfg.changeSensitivityEnabled = false;
    cfg.wakeupEnabled = false;
    cfg.alwaysOnEnabled = false;
    cfg.batchInterval_us = 0;
    cfg.sensorSpecific = 0;

    // ceva sh2 api, send cfg from pico to bno085
    int status = sh2_setSensorConfig(sensorId, &cfg);
    
    if (status != SH2_OK) {
        printf("enable_report failed. error code: %d \n", status);
        sleep_ms(1000);
    }
    // 1 = true, 0 = false
    return (status == SH2_OK);
}

bool bno085_get_quaternion(float *qw, float *qx, float *qy, float *qz) {
    // checks 1st, will exit the function if something goes wrong
    // if sensor_value is invalid or sensorId != rotation vector (go to struct sensor_value points to, access sensorId, check that it equals the thing)
    if (!sensor_value || sensor_value->sensorId != SH2_ROTATION_VECTOR) {
        return false;
    }
    // if false then exit, cuz no new quaternion
    if (!bno085_has_new) {
        return false;
    }

    // set de-refed pointer to the value u get when accessing struct sensor_value points to, access relevant stuff
    *qw = sensor_value->un.rotationVector.real;
    *qx = sensor_value->un.rotationVector.i;
    *qy = sensor_value->un.rotationVector.j;
    *qz = sensor_value->un.rotationVector.k;

    bno085_has_new = false;   // mark sample as consumed
    return true;
}

void bno085_poll(void) {
    // ask sh2 to process any pending transfers
    sh2_service();
}