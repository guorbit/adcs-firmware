/*
 * Copyright © 2014 Kosma Moczek <kosma@cloudyourcar.com>
 * This program is free software. It comes without any warranty, to the extent
 * permitted by applicable law. You can redistribute it and/or modify it under
 * the terms of the Do What The Fuck You Want To Public License, Version 2, as
 * published by Sam Hocevar. See the COPYING file for more details.
 */

// my goat lol, based on https://github.com/kosma/minmea/blob/master/example.c
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/sync.h"

#include "gtu7.h"

#define ADCS_DEBUG true
#define GTU7_TELEM_LEN 44

// Shared resources
char shared_nmea_raw[MINMEA_MAX_SENTENCE_LENGTH];
gps_data_t shared_gps;
extern critical_section_t gps_crit;
extern gps_data_t shared_gps;


// initialise storage struct
static gps_data_t gps_state = {0};

bool gps_init(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud) {
    sleep_ms(2000);

    uint16_t return_baud = uart_init(uart, baud);
    printf("baud rate: %d\n",return_baud);
    
    // prevent buffer from getting filled with junk
    //uart_set_fifo_enabled(uart, false);
	// set the TX and RX pins
	gpio_set_function(tx, UART_FUNCSEL_NUM(uart, tx));
	gpio_set_function(rx, UART_FUNCSEL_NUM(uart, rx));
	// set UART flow control CTS/RTS
	uart_set_hw_flow(uart, false, false);
    gpio_pull_up(17);

    // clear buffer during initialisation
    while (uart_is_readable(uart)){
        uart_getc(uart);
    }

    bool uart_status = uart_is_enabled(GTU7_UART);
    printf("uart enabled: %d, baud rate: %d\n", uart_status, return_baud);
    return uart_status;
}

bool read_gtu7_uart(char *line, int max_length){
    static uint16_t index = 0;

    if (uart_is_readable(GTU7_UART)){
        char get_char = uart_getc(GTU7_UART);
        
        // 1st line of a gps sentence is '$'
        if (get_char == '$'){
            index = 0; // reset to start
            line[index++] = get_char;
        } else if (get_char == '\n'){
            line[index] = '\0';
            index = 0;
            return true; // should return true at the end of a sentence
        } else if (get_char != '\r' && index < max_length - 1){
            line[index++] = get_char;
        }
    } 
    return false;
}

void gps_get_sentence(char *line) {
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {
                // update the static struct, convert frame from struct to float
                gps_state.lat = minmea_tocoord(&frame.latitude);
                gps_state.lon = minmea_tocoord(&frame.longitude);
                gps_state.speed = minmea_tofloat(&frame.speed);
            }
        } break;
        
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                gps_state.fix_quality = frame.fix_quality;
                gps_state.alt = minmea_tofloat(&frame.altitude);
                gps_state.hour = frame.time.hours;
                gps_state.min = frame.time.minutes;
                gps_state.sec = frame.time.seconds;
            }
        } break;
    }
}

gps_data_t gps_data(void){
    return gps_state;
}

void gps_update_shared(gps_data_t gps, char *nmea_raw){
    #if ADCS_DEBUG
    strncpy(shared_nmea_raw, nmea_raw, sizeof(shared_nmea_raw) - 1);
    shared_nmea_raw[sizeof(shared_nmea_raw) - 1] = '\0';
    #endif
    shared_gps = gps;
}

uint32_t gtu7_print(char* buf, size_t max_len){
    char tmp[96];
    gps_data_t gps;

    // Fetch GPS data from shared variable
    critical_section_enter_blocking(&gps_crit);
    gps = shared_gps;
    critical_section_exit(&gps_crit);
    
    // check length of string
    int len_test = snprintf(tmp, sizeof(tmp),
        "t%02d%02d%02d|N%+09.5f|E%+010.5f|h%+07.2fm|f%1d|",
        gps.hour, gps.min, gps.sec, 
        gps.lat, gps.lon, gps.alt, 
        gps.fix_quality);
    
    if (len_test <= GTU7_TELEM_LEN){
        strncpy(buf, tmp, GTU7_TELEM_LEN);
        return len_test;
    }else{
        printf("gtu7_print: len test [%d], GTU7_TELEM_LEN [%d], strlen [%d]\n", len_test, GTU7_TELEM_LEN, len_test);
        //                      "t000000|N+00.00000|E+000.00000|h+000.00m|f0|";
        const char* error_msg = "tERROR.|N+ERROR...|E+ERROR....|h+ERROR.m|f0|";
        strncpy(buf, error_msg, GTU7_TELEM_LEN);
        return GTU7_TELEM_LEN;
    }
}

