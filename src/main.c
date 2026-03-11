#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "pico/binary_info.h"

#include "gtu7.h"

char line[MINMEA_MAX_SENTENCE_LENGTH];

int main(void) {
    stdio_init_all();

    sleep_ms(10000);

    // gps initialisation
    while(!gps_init(GTU7_UART, GTU7_TX, GTU7_RX, GTU7_BAUD)){
        sleep_ms(1000);
        printf("init failed, retrying...\n");
    }
    printf("init successful v2.1\n");

    
    // getting data from gps
    while(1){
        if (read_uart(line, MINMEA_MAX_SENTENCE_LENGTH) == true) {
            printf("nmea sentence: %s\n", line);
            // translate gps data
            gps_get_sentence(line);
            // copy data from gps_data into gps
            gps_data_t gps = gps_data();
            // lat and lon currently to 5dp, lmk if it should be more accurate
            // -180.28881
            printf("UTC: %02d:%02d:%02d |Lat: %+09.5f, Lon: %+010.5f, Alt: %+07.2fm, Fix: %d\n", 
            gps.hour, gps.min, gps.sec, gps.lat, gps.lon, gps.alt, gps.fix_quality);
        } 
        sleep_ms(1);
    }

}