#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "i2c_utils.h"
#include "hardware/gpio.h"

// sh2 libraries https://github.com/ceva-dsp/sh2
// based on this library https://github.com/robotcopper/BNO08x_Pico_Library, i took out a lot of stuff though
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "bno085.h"
#include "sensor_i2c.h"

#define BNO085_TELEM_LEN 79 // not tested either
// driver for the imu, to get acceleration x,y,z; yaw, pitch, roll; magnetic heading

static sh2_Hal_t HAL;
volatile bool reset_occurred = false;
static bool bno085_has_new = false;
static sh2_SensorValue_t sensorValue; // initialise struct of type sh2_SensorValue_t
static sh2_SensorValue_t * sensor_value = &sensorValue; // sensor_value is pointer to the struct provided by the sh2 driver
static bno085_state_t bno085_data; // internal storage struct for bno085 data
static bno085_state_t internal_state;

// enable report bools
bool accel_report_check = 0;
bool mag_report_check = 0;
bool rotation_report_check = 0;

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

    // ensure I2C pins are using I2C function and pulled up
    gpio_set_function(SENSOR_SDA, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_SDA);
    gpio_pull_up(SENSOR_SCL);

    // if we have a hardware reset pin, pulse it to ensure sensor is in a known state
    if (BNO085_RST_PIN >= 0) {
        gpio_init(BNO085_RST_PIN);
        gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);
        // drive low to reset
        gpio_put(BNO085_RST_PIN, 0);
        sleep_ms(10);
        // release
        gpio_put(BNO085_RST_PIN, 1);
        sleep_ms(50);
    }

    // attempt to reset sensor, this is the main point of this function
    for (uint8_t attempts = 0; attempts < 5; attempts++) {
        printf("attempt %d: sending reset...\n", attempts+1);
        sleep_ms(10);
        int result = i2c_write_timeout_us(SENSOR_I2C, BNO085_ADDR, softreset_pkt, 5, false, I2C_TIMEOUT_US);
        
        // if result is 5 bytes (softrest_pkt)
        if (result == 5) {
            printf("reset packet sent\n");
            success = true;
            break;
            }
            else{
                printf("i2c_write_timeout_us failed. error code: %d \n", result);
            }
        sleep_ms(30);
    }

    if (!success) {
        printf("sh2chal_open failed, resetting i2c bus\n");
        i2c_bus_reset(SENSOR_I2C, SENSOR_SDA, SENSOR_SCL);
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
    write = i2c_write_timeout_us(SENSOR_I2C, BNO085_ADDR, buf, len, false, I2C_TIMEOUT_US);
    
    if (write != (int)len) {
        printf("i2c write failed. requested length: %d, error code: %d \n", len, write);
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

    return_value = i2c_read_timeout_us(SENSOR_I2C, BNO085_ADDR, i2c_buffer, len, false, I2C_TIMEOUT_US);

    if (return_value <= 0) {
        return 0;
    }

    // getting the packet size
    packet_size = (uint16_t)i2c_buffer[0] | ((uint16_t)i2c_buffer[1] << 8);
    packet_size &= ~0x8000;  // clear continue bit

    // caller buffer wrong size
    if (packet_size > len || packet_size == 0) {
        printf("packet size %d not correct \n", packet_size);
        return 0;            
    }

    if (timestamp_us) {
        *timestamp_us = (uint32_t)to_us_since_boot(get_absolute_time());
    }

    if (packet_size < 0 && packet_size > 100) { // Filter out the huge boot packets
         printf("pkt: len = %d, channel = %d \n", packet_size, i2c_buffer[2]);
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
    int res = i2c_read_timeout_us(SENSOR_I2C, BNO085_ADDR, &dummy, 1, false, I2C_TIMEOUT_US);
    if (res <= 0) {
       printf("failed: bno085 didn't ack. error code : %d\n", res);
       sleep_ms(100);
       return false; // return to main.c
    }

    // fill HAL struct
    HAL.open      = sh2chal_open;
    HAL.close     = sh2_hal_close;
    HAL.read      = sh2_hal_read;
    HAL.write     = sh2_hal_write;
    HAL.getTimeUs = hal_getTimeUs;

    // open sh2, sh2_open is from the ceva sh2 api (sh2.c)
    int status = sh2_open(&HAL, sh2_async_event_handler, NULL);
    if (status != SH2_OK) {
        printf("sh2_open failed, returned error code: %d\n", status);
        sleep_ms(1000);
        return false;
    }

    // register sensor callback, also from the ceva api
    sh2_setSensorCallback(sh2_sensor_event_handler, NULL);
    reset_occurred = false;

    // fetch inital boot packet to clear sensor
    for (int i = 0; i < 200; i++) {
        // check if pulled low, i.e. bno085 has ack/data
        if (gpio_get(BNO085_INT_PIN) == 0){
            bno085_poll(); // poll sensor for advert packet
            break;
        }  
        sleep_ms(5);
    }    

    

    printf("bno085_init: done\n");
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
        printf("enable_report: failed, error code: %d \n", status);
        sleep_ms(1000);
    }
    // 1 = true, 0 = false
    return (status == SH2_OK);
}

bool bno085_get_report(bno085_state_t *out) {
    if(!sensor_value || !bno085_has_new) {
        return false;
    }

    // uint8_t status; /**< @brief bits 7-5: reserved, 4-2: exponent delay, 1-0: Accuracy */, so use a mask for just accuracy
    internal_state.status[0] =  (sensor_value->status & 0x03);
    bno085_has_new = false; // reset flag
    switch (sensor_value->sensorId) {
        case SH2_ACCELEROMETER:
        internal_state.accel[0] = sensor_value->un.accelerometer.x;
        internal_state.accel[1] = sensor_value->un.accelerometer.y;
        internal_state.accel[2] = sensor_value->un.accelerometer.z;
        return false; // Don't return yet, wait for orientation

        case SH2_MAGNETIC_FIELD_CALIBRATED:
        internal_state.mag[0] = sensor_value->un.magneticField.x;
        internal_state.mag[1] = sensor_value->un.magneticField.y;
        internal_state.mag[2] = sensor_value->un.magneticField.z;
        return false;

        case SH2_MAGNETIC_FIELD_UNCALIBRATED:
        internal_state.mag[0] = sensor_value->un.magneticFieldUncal.x;
        internal_state.mag[1] = sensor_value->un.magneticFieldUncal.y;
        internal_state.mag[2] = sensor_value->un.magneticFieldUncal.z;
        return false;

        case SH2_ROTATION_VECTOR:
        internal_state.quat[0] = sensor_value->un.rotationVector.real;
        internal_state.quat[1] = sensor_value->un.rotationVector.i;
        internal_state.quat[2] = sensor_value->un.rotationVector.j;
        internal_state.quat[3] = sensor_value->un.rotationVector.k;
        
        // Copy the whole state to the output
        *out = internal_state;
        return true; // Return true to trigger a print in main
    }
    return false;
}

void bno085_poll(void) {
    // ask sh2 to process any pending transfers
    sh2_service();
}

// enable different reports, at 100 hz
uint8_t bno085_enable_reports(void) {
    // enable reports
    enable_report(SH2_ROTATION_VECTOR, 10000);
    enable_report(SH2_ACCELEROMETER, 10000);
    enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 50000);

    // checks
    if (!enable_report(SH2_ROTATION_VECTOR, 10000)) {
        printf("enable_report failed (rotation vector)\n");
        accel_report_check = 0;        
    } else{
        accel_report_check = 1; 
    }
    if (!enable_report(SH2_ACCELEROMETER, 10000)) {
        printf("enable_report failed (accelerometer)\n");
        rotation_report_check = 0;
    } else{
        rotation_report_check = 1;
    }
    if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 50000)) {
        printf("enable_report failed (magnetometer)\n");
        mag_report_check = 0;
    } else {
        mag_report_check = 1;
    }

    // bit masking
    uint8_t report_check = (rotation_report_check << 0) | (accel_report_check << 1) | (mag_report_check << 2);

    return report_check;
}

// reset stuff, trying to keep it in one place
void bno085_reset(void) {
    // handles the reset_occured flag, re-inits the reports
    printf("sensor reset detected, re-enabling reports (bno085_reset)");

    if (!enable_report(SH2_ROTATION_VECTOR, 10000)) {
        rotation_report_check = 0;
        printf("enable_report failed (rotation vector)\n");
    }
    if (!enable_report(SH2_ACCELEROMETER, 10000)) {
        accel_report_check = 0;
        printf("enable_report failed (accelerometer)\n");
    }
    if (!enable_report(SH2_MAGNETIC_FIELD_CALIBRATED, 50000)) {
        mag_report_check = 0;
        printf("enable_report failed (magnetometer)\n");
    }
    // will be checked in main

    // reset the flag
    reset_occurred = false;
}

bool bno085_hw_reset(void) {
    gpio_init(BNO085_RST_PIN);
    gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);
    // drive low to reset
    gpio_put(BNO085_RST_PIN, 0);
    sleep_ms(20);
    // release
    gpio_put(BNO085_RST_PIN, 1);
    sleep_ms(1500);

    // stop sh2, will re-init in bno085_init() later
    sh2_close();

    // need to re-init
    if (bno085_init()){
        return true;
    } else {
        printf("bno085_hw_reset failed to re-initialise\n");
        return false;
    }
}

void bno085_update(void){
    // poll bno085
    static bno085_state_t bno085_tmp;
    bno085_get_report(&bno085_tmp);

    // stale check
    if (bno085_tmp.status[0] > 0){
        bno085_data = bno085_tmp;
    }
}


uint32_t bno085_print(char *buf, size_t len){
    // temporary buffer used to check data
    char tmp[128];

    // test length of string before sending to main.c
    int len_test = snprintf(tmp, sizeof(tmp), 
    "i%d|a%+07.2f%+07.2f%+07.2f|q%+07.2f%+07.2f%+07.2f%+07.2f|m%+07.2f%+07.2f%+07.2f\n",
    bno085_data.status[0],
    bno085_data.accel[0], bno085_data.accel[1], bno085_data.accel[2],
    bno085_data.quat[0],  bno085_data.quat[1],  bno085_data.quat[2], bno085_data.quat[3],
    bno085_data.mag[0],   bno085_data.mag[1],   bno085_data.mag[2]);

    if (len_test <= BNO085_TELEM_LEN){
        strncpy(buf, tmp, BNO085_TELEM_LEN);
        return len_test;
    }else{
        // error string should match BNO085_TELEM_LEN, pls check
        const char* error_msg = "iX|a+ERR.00+ERR.00+ERR.00|q+ERR.00+ERR.00+ERR.00+ERR.00|m+ERR.00+ERR.00+ERR.00|";
        printf("bno085_print: len test [%d], BNO085_TELEM_LEN [%d], strlen [%d]\n", len_test, BNO085_TELEM_LEN, len_test);
        //                      "iX|a+.00+000.00+000.00|q+000.00+000.00+000.00+000.00|m+000.00+000.00+000.00|";
        strncpy(buf, error_msg, BNO085_TELEM_LEN);
        return BNO085_TELEM_LEN;
    }
}