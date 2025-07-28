#ifndef GPS_UBLOX_H
#define GPS_UBLOX_H

/**
 * @brief Driver
 * 
 * @author José Antonio Septién Hernández
 * 
 */

#include <stdbool.h>
#include <time.h>

#include "board.h"
#include "periph/uart.h"
#include "ztimer.h"
#include "tsrb.h"
#include "msg.h"
#include "thread.h"

#include "minmea.h"

#define TSRB_BUFFSIZE 512
#define MSG_QUEUE_SIZE (8)

static uint8_t buffer[TSRB_BUFFSIZE];
static tsrb_t _tsrb = TSRB_INIT(buffer);
static uart_t _uart_dev;
static kernel_pid_t main_pid;
static msg_t q[MSG_QUEUE_SIZE];

// For nmea messages
static struct minmea_sentence_gll _gll;
static struct minmea_sentence_rmc _rmc;
static struct minmea_sentence_gga _gga;

void gps_cb(void *arg, uint8_t data);
bool init_gps_ublox(uart_t _dev, uint32_t baud, uint16_t rate);
bool parse_nmea_message(void);
bool get_nmea_rmc_json(char *json);
bool get_nmea_gga_json(char *json);
bool get_nmea_gll_json(char *json);

void clear_buffer(void);

#endif      // GPS_UBLOX_H
