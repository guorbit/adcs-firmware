/*
 * gps.c
 *
 * Implementation of GPS driver in C
 */

#include "gps.h"
#include <stdio.h>
#include <string.h>
#include <minmea.h>        // Requires the minmea library
#include "hardware/irq.h"
#include "hardware/gpio.h"

// -- Internal State --

// This struct holds all variables that were previously private class members
typedef struct {
    uart_inst_t *uart;
    char buffer[GPS_BUF_LEN];
    uint32_t buffer_index;
    
    float lat;
    float lon;
    float speed;
    float height;
    int satellites;
    char time_str[16];
} GPS_State_t;

// The "Singleton" instance - static limits scope to this file
static GPS_State_t state = {
    .uart = NULL,
    .buffer_index = 0,
    .lat = 0.0f,
    .lon = 0.0f,
    .speed = 0.0f,
    .height = 0.0f,
    .satellites = 0,
    .time_str = ""
};

// -- Internal Prototypes --
static void GPS_OnUartRx(void);
static void GPS_DecodeBuffer(void);

// -- Public Function Implementations --

void GPS_Init(uart_inst_t *uart, uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate) {
    state.uart = uart;

    uart_init(state.uart, baud_rate);

    // Set the TX and RX pins
    gpio_set_function(tx_pin, UART_FUNCSEL_NUM(state.uart, tx_pin));
    gpio_set_function(rx_pin, UART_FUNCSEL_NUM(state.uart, rx_pin));

    // Set UART flow control CTS/RTS (false, false)
    uart_set_hw_flow(state.uart, false, false);

    // Determine the UART IRQ number
    int uart_irq = (state.uart == uart0) ? UART0_IRQ : UART1_IRQ;

    // Register and enable UART RX interrupt
    irq_set_exclusive_handler(uart_irq, GPS_OnUartRx);
    irq_set_enabled(uart_irq, true);

    // Enable RX interrupt only
    uart_set_irq_enables(state.uart, true, false);
}

float GPS_GetLat(void) {
    return state.lat;
}

float GPS_GetLon(void) {
    return state.lon;
}

float GPS_GetSpeed(void) {
    return state.speed;
}

int GPS_GetNumSat(void) {
    return state.satellites;
}

const char* GPS_GetTime(void) {
    return state.time_str;
}

float GPS_GetHeight(void) {
    return state.height;
}

// -- Internal Helper Implementations --

// UART RX interrupt handler
static void GPS_OnUartRx(void) {
    while (uart_is_readable(state.uart)) {
        uint8_t ch = uart_getc(state.uart);
        
        if ((ch == '\n') || (ch == '\r')) {
            if (state.buffer_index > 0) {
                state.buffer[state.buffer_index] = 0; // Null terminate
                GPS_DecodeBuffer();
                state.buffer_index = 0; // Reset
            }
        } else {
            if (state.buffer_index < GPS_BUF_LEN - 1) {
                state.buffer[state.buffer_index++] = ch;
            } else {
                // Buffer overflow protection: reset
                state.buffer_index = 0;
            }
        }
    }
}

// Decode the NMEA sentence
static void GPS_DecodeBuffer(void) {
    // minmea_sentence_id detects the type of NMEA sentence (RMC, GGA, etc.)
    switch (minmea_sentence_id(state.buffer, false)) {
        
        // Position, Velocity, and Time
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, state.buffer)) {
                state.lat = minmea_tocoord(&frame.latitude);
                state.lon = minmea_tocoord(&frame.longitude);
                state.speed = minmea_tofloat(&frame.speed);
                
                snprintf(state.time_str, sizeof(state.time_str), "%02d:%02d:%02d", 
                         frame.time.hours, frame.time.minutes, frame.time.seconds);
            }
        } break;

        // Satellites in view
        case MINMEA_SENTENCE_GSV: {
            struct minmea_sentence_gsv frame;
            if (minmea_parse_gsv(&frame, state.buffer)) {
                state.satellites = frame.total_sats;
            }
        } break;

        // Global Positioning System Fix Data    
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, state.buffer)) {
                state.height = minmea_tofloat(&frame.altitude);
            }
        } break;
        
        default:
            break;
    }
}