#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"

#include "hardware/i2c.h"
#include "sh2.h"
#include "i2c_utils.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <string.h>
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "pico/sync.h"

#include "gtu7.h"
#include "bmp280.h"
#include "bno085.h"
#include "obc.h"
#include "blink.h"
#include "sensor_i2c.h"

#define ADCS_DEBUG true


// Shared resources
char shared_nmea_raw[MINMEA_MAX_SENTENCE_LENGTH];
gps_data_t shared_gps;
critical_section_t gps_crit;

// Core1
void main1(void) {
    // init stuff that will be used in while loop
    char nmea_raw[MINMEA_MAX_SENTENCE_LENGTH];
    gps_data_t gps; // struct to store incoming gps data
    while(1) {
        // polling gtu7
        while(!read_gtu7_uart(nmea_raw, MINMEA_MAX_SENTENCE_LENGTH)){
            // do nothing until all characters are fetched
        }

        // translate gps data
        gps_get_sentence(nmea_raw);
        // copy data from gps_data into gps
        gps = gps_data(); // updating persistent variable

        // Critical section to update shared_gps
        critical_section_enter_blocking(&gps_crit);
        #if ADCS_DEBUG
        strncpy(shared_nmea_raw, nmea_raw, sizeof(shared_nmea_raw) - 1);
        shared_nmea_raw[sizeof(shared_nmea_raw) - 1] = '\0';
        #endif
        shared_gps = gps;
        critical_section_exit(&gps_crit);

        sleep_ms(5);
    }
}

// Core0
int main(void) {
    stdio_init_all();
    sleep_ms(10000);   // allow usb to enumerate
    printf("\nsystem boot\n");

    adcs_slave_init();
    printf("adcs initialised as slave\n");

    // Initialize critical section
    critical_section_init(&gps_crit);
    blink_init();

    // // reset the imu by driving the reset pin low for 20ms, then releasing
    // gpio_init(BNO085_RST_PIN);
    // gpio_set_dir(BNO085_RST_PIN, GPIO_OUT);
    // // drive low to reset
    // gpio_put(BNO085_RST_PIN, 0);
    // sleep_ms(20);
    // // release
    // gpio_put(BNO085_RST_PIN, 1);
    // sleep_ms(2000);

    // init i2c for bmp280 and bno085
    sensor_i2c_init();
    
    printf("I2C initialized on pins SDA=%d, SCL=%d\n", PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN);
    // print devices visible on the bus
    i2c_bus_scan(SENSOR_I2C, SENSOR_SDA, SENSOR_SCL);

    // bno085 initialisation
    printf("Starting BNO085\n");
    while(!bno085_init()) {
        printf("retry bno085_init\n");
        i2c_bus_reset(SENSOR_I2C, SENSOR_SDA, SENSOR_SCL);
        sh2_devReset();
        sleep_ms(1000);
    }
    bno085_enable_reports();
    printf("BNO085 ready v6\n");
    sleep_ms(100);
    
    // bmp280 initialisation
    bmp280_init();
    printf("bmp280 initialised\n");

    // gps initialisation
    while(!gps_init(GTU7_UART, GTU7_TX, GTU7_RX, GTU7_BAUD)){
        sleep_ms(1000);
        printf("gtu7 init failed, retrying\n");
    }
    printf("gtu7 initialised\n");

    // Launch Core 1
    multicore_launch_core1(main1);
    printf("Core 1 launched for GPS handling\n");

    uint32_t last_sensor_read = to_ms_since_boot(get_absolute_time()); // for the watchdog
    uint32_t last_data_print = 0; // for printing
    static float last_qx, last_qy, last_qz;
    static int stale_count = 0;
    char obc_telem [136]; // internal buffer can be bigger than obc buffer but i'll just set it exactly
    gps_data_t gps; // local gps struct for core0

    // main loop
    while (1) {
        blink_polling();
        
        // bmp280 polling
        bmp280_data_t bmp280_main; // local main struct for bmp280 data
        bmp280_update();
        bmp280_get(&bmp280_main);
        
        // bno085 polling
        bno085_state_t bno085_main; // local main struct for bno085 data
        printf("int_pin: %d\n", gpio_get(BNO085_INT_PIN));
        bno085_poll(); // idk why this is here
        bno085_update();
        bno085_get(&bno085_main);

        // timer for print and watchdog
        uint32_t now = to_ms_since_boot(get_absolute_time());
        float qw, qx, qy, qz;

        // Fetch GPS data from shared variable
        critical_section_enter_blocking(&gps_crit);
        gps = shared_gps;
        critical_section_exit(&gps_crit);

        // print data, rate limited atm
        if (now - last_data_print > 100) {
            // UTC: %02d:%02d:%02d |Lat: %+09.5f, Lon: %+010.5f, Alt: %+07.2fm, Fix: %d| temp: %07.2f | pressure: %lu | bno085 status: %d | acc: %+07.2f %+07.2f %+07.2f | quat: %+07.2f %+07.2f %+07.2f %+07.2f | mag: %+07.2f %+07.2f %+07.2f\n
            uint16_t obc_msg_len = snprintf(obc_telem, sizeof(obc_telem), "t%02d%02d%02d|N%+09.5f|E%+010.5f|h%+07.2fm|f%d|c%07.2f|b%lu|i%d|a%+07.2f%+07.2f%+07.2f|q%+07.2f%+07.2f%+07.2f%+07.2f|m%+07.2f%+07.2f%+07.2f\n",
                gps.hour, gps.min, gps.sec, 
                gps.lat, gps.lon, gps.alt, gps.fix_quality,
                bmp280_main.temperature, bmp280_main.pressure_pa, bno085_main.status[0],
                bno085_main.accel[0], bno085_main.accel[1], bno085_main.accel[2],
                bno085_main.quat[0],  bno085_main.quat[1],  bno085_main.quat[2], bno085_main.quat[3],
                bno085_main.mag[0],   bno085_main.mag[1],   bno085_main.mag[2]);

            last_data_print = now;

            #if ADCS_DEBUG

            printf(shared_nmea_raw);

            printf("length of buffer: %d\n", obc_msg_len); // currently 136 but keeps changing, need to fix
            printf(obc_telem);

            #endif

            adcs_telemetry((const uint8_t *)obc_telem, strlen(obc_telem));
        }
    sleep_ms(5);
    }   
}