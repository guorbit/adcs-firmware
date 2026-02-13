/*
 * GPS.h
 *
 * Read data location from ATGM336H-5N
 *
 *  Created on: 3 Jan 2025
 *      Author: jondurrant
 */

/*
 * gps.h
 *
 * Read data location from ATGM336H-5N (C Version)
 */

#ifndef GPS_H_
#define GPS_H_

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include <stdint.h> 

// Constants
#define GPS_BUF_LEN 1024

// -- Public API --

// Initialize the GPS (Replaces setUp and getInstance)
void GPS_Init(uart_inst_t *uart, uint8_t tx_pin, uint8_t rx_pin, uint32_t baud_rate);

// Getters
float GPS_GetLat(void);
float GPS_GetLon(void);
float GPS_GetSpeed(void);
int GPS_GetNumSat(void);
const char* GPS_GetTime(void);
float GPS_GetHeight(void);

#endif