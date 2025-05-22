#ifndef GPS_UBLOX_H
#define GPS_UBLOX_H

/**
 * @brief Driver
 * 
 * @author José Antonio Septién Hernández
 * 
 */

#include <stdbool.h>

#include "board.h"
#include "periph/uart.h"
#include "ztimer.h"
#include "tsrb.h"

#define TSRB_BUFFSIZE 512

static uint8_t buffer[TSRB_BUFFSIZE];
static tsrb_t _tsrb = TSRB_INIT(buffer);
static uart_t _uart_dev;

bool init_gps_ublox(uart_t _dev, uint32_t baud, uint16_t rate);


#endif      // GPS_UBLOX_H
