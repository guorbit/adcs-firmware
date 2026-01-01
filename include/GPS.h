/*
 * GPS.h
 *
 * Read data location from ATGM336H-5N
 *
 *  Created on: 3 Jan 2025
 *      Author: jondurrant
 */

#ifndef EXP_DECODEINT_SRC_GPS_H_
#define EXP_DECODEINT_SRC_GPS_H_

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


#define GPS_BUF_LEN 1024


class GPS {
public:

	virtual ~GPS();

	static GPS * getInstance();
	void setUp(uart_inst_t *uart, uint8_t tx, uint8_t rx, uint16_t baud);

	float getLat();
	float getLon();
	float getSpeed();
	int getNumSat();
	const char* getTime();
	float getHeight();

protected:
	static void onUartRxCB();

	void onUartRx();

private:
	GPS();
	void decodeBuffer();


	static GPS * pSelf;
	uart_inst_t *pUart = NULL;

	char xBuffer[GPS_BUF_LEN];
	uint xBufferInd = 0;

	float xLat = 0.0;
	float xLon = 0.0;
	float xSpeed = 0.0;
	char xTime[16] = "";
	uint xSat = 0;
	float xHeight = 0.0;
};

#endif /* EXP_DECODEINT_SRC_GPS_H_ */
