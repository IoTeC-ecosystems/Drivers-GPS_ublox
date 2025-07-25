#include <stdio.h>
#include <string.h>

#include "gps_ublox.h"

#define AUX_BUF_SIZE 124

void gps_cb(void *arg, uint8_t data)
{
    (void) arg;
    // Add data to the ring buffer
    tsrb_add_one(&_tsrb, data);
    if (data == '\n') {
        msg_t _msg;
        _msg.content.value = (uint32_t)data;
        msg_send(&_msg, main_pid);
    }
}

void checksum(uint8_t *packet, uint32_t n)
{
    uint8_t ck_a = 0, ck_b = 0;
    for (uint8_t i = 2; i < n - 2; i++) {
        ck_a += packet[i];
        ck_b += ck_a;
    }
    packet[n - 2] = ck_a;
    packet[n - 1] = ck_b;
}

bool init_gps_ublox(uart_t _dev, uint32_t baud, uint16_t rate)
{
    // Clear buffer and variables
    memset((void *)&_gll, 0, sizeof(_gll));
    memset((void *)&_rmc, 0, sizeof(_rmc));
    memset((void *)&_vtg, 0, sizeof(_vtg));
    memset((void *)&_gga, 0, sizeof(_gga));
    memset((void *)&buffer, 0, strlen((char *)buffer));

    main_pid = thread_getpid();
    tsrb_clear(&_tsrb);
    msg_init_queue(q, MSG_QUEUE_SIZE);

    _uart_dev = _dev;
    int ret = uart_init(_uart_dev, baud, gps_cb, NULL);
    if (ret < 0) {
        return false;
    }

    // Restore defaults
    // CFG-CFG
    uint8_t cfg_cfg[] = {
        0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
        // Payload
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x17,
        // CK_A, CK_B
        0x00, 0x00,
    };
    checksum(cfg_cfg, sizeof(cfg_cfg));
    uart_write(_uart_dev, cfg_cfg, sizeof(cfg_cfg));
    ztimer_sleep(ZTIMER_MSEC, 50);
    msg_t msg;

    // CFG-RATE - set update rate
    uint8_t cfg_rate[] = {
        0xB5, 0x62, 0x06, 0x08, 0x06, 0x00,
        // Payload
        rate & 0x00FF, (rate & 0xFF00) >> 8, 0x01, 0x00, 0x01, 0x00,
        // CK_A, CK_B
        0x00, 0x00,
    };
    checksum(cfg_rate, sizeof(cfg_rate));
    uart_write(_uart_dev, cfg_rate, sizeof(cfg_rate));
    ztimer_sleep(ZTIMER_MSEC, 50);

    // Nav engine settings
    uint8_t cfg_nav5[] = {
        0xB5, 0x62, 0x06, 0x24, 0x24, 0x00,
        // payload
        0x07, 0x00,                 // mask
        0x02,                       // dynModel - static
        0x03,                       // Fix mode - auto
        0x00, 0x00, 0x00, 0x00,     // fixedAlt
        0x00, 0x00, 0x00, 0x00,     // fixedAltVar
        0x0A,                       // minElev - for satellite: 10° above horizon
        0x00,
        0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // CK_A, CK_B
        0x00, 0x00,
    };
    checksum(cfg_nav5, sizeof(cfg_nav5));
    uart_write(_uart_dev, cfg_nav5, sizeof(cfg_nav5));
    ztimer_sleep(ZTIMER_MSEC, 50);

    // Disable nmea messages
    uint8_t messages[][2] = {
        {0xF0, 0x03},   // GSV
        {0xF0, 0x02},   // GSA
    };
    uint8_t cfg_msg[] = {
        0xB5, 0x62, 0x06, 0x01, 0x03, 0x00,
        // payload
        0x00, 0x00, 0x00,
        // CK_A, CK_B
        0x00, 0x00
    };
    uint8_t offset = 6;
    for (uint8_t i = 0; i < sizeof(messages) / sizeof(*messages); i++) {
        for (uint8_t j = 0; j < sizeof(*messages); j++) {
            cfg_msg[offset + j] = messages[i][j];
        }
        checksum(cfg_msg, sizeof(cfg_msg));
        uart_write(_uart_dev, cfg_msg, sizeof(cfg_msg));
        ztimer_sleep(ZTIMER_MSEC, 50);
    }

    //
    return true;

    while (1) {
        if (msg_try_receive(&msg) == -1) {
            continue;
        }
        uint8_t c = 0;
        char sentence[MINMEA_MAX_SENTENCE_LENGTH];
        int i = 0;
        // Monitor module until the message
        // $GPTXT,01,01,02,ANTSTATUS=OK*3B is received
        while (c != '\r') {
            c = tsrb_get_one(&_tsrb);
            sentence[i++] = (char)c;
        }
        if (memcmp(sentence + 3, "TXT", 3) == 0) {
            if (memcmp(sentence + 26, "OK", 2) == 0) {
                // OK received
                break;
            }
        }
    }

    return true;
}

bool parse_nmea_message(void)
{
    char line[2 * MINMEA_MAX_SENTENCE_LENGTH];
    memset(line, 0, 2 * MINMEA_MAX_SENTENCE_LENGTH);
    // Read the tsrb buffer
    msg_t msg;
    if (msg_try_receive(&msg) == -1) {
        return false;
    }
    unsigned int i = 0;
    uint8_t c = 0;
    while (tsrb_avail(&_tsrb)) {
        c = tsrb_get_one(&_tsrb);
        line[i++] = (char)c;
    }

    enum minmea_sentence_id id = minmea_sentence_id(line, false);
    if (id == MINMEA_INVALID) {
        return false;
    }
    bool can_parse = true;
    switch (id) {
        case MINMEA_SENTENCE_GLL:
            if (!minmea_parse_gll(&_gll, line)) can_parse = false;
            break;
        case MINMEA_SENTENCE_RMC:
            if (!minmea_parse_rmc(&_rmc, line)) can_parse = false;
            break;
        case MINMEA_SENTENCE_VTG:
            if (!minmea_parse_vtg(&_vtg, line)) can_parse = false;
            break;
        case MINMEA_SENTENCE_GGA:
            if (!minmea_parse_gga(&_gga, line)) can_parse = false;
            break;
        default:
            // Ignore any other sentence
            break;
    }

    return can_parse;
}

bool get_nmea_rmc_json(char *json)
{
    strcat(json, "{\n");

    // Get datetime from rmc sentence
    strcat(json, "\t\"datetime\": ");
    struct tm tm;
    int ret = minmea_getdatetime(&tm, &_rmc.date, &_rmc.time);
    char buffer[AUX_BUF_SIZE];
    memset(buffer, 0, AUX_BUF_SIZE);
    // Datetime format: dd/mm/yyyy HH:mm
    sprintf(buffer, "%02d/%02d/%4d %2d:%02d,\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_sec);
    strcat(json, buffer);

    // Get course
    strcat(json, "\t\"course\": ");
    memset(buffer, 0, AUX_BUF_SIZE);
    sprintf(buffer, "%f,\n", minmea_tofloat(&_rmc.course));
    strcat(json, buffer);

    // Get latitude
    strcat(json, "\t\"latitude\": ");
    memset(buffer, 0, AUX_BUF_SIZE);
    sprintf(buffer, "%f,\n", minmea_tocoord(&_rmc.latitude));
    strcat(json, buffer);

    // Get longitude
    strcat(json, "\t\"longitude\": ");
    memset(buffer, 0, AUX_BUF_SIZE);
    sprintf(buffer, "%f,\n", minmea_tocoord(&_rmc.longitude));
    strcat(json, buffer);

    // Get speed
    strcat(json, "\t\"speed\": ");
    memset(buffer, 0, AUX_BUF_SIZE);
    sprintf(buffer, "%f\n", minmea_tofloat(&_rmc.speed));
    strcat(json, buffer);

    strcat(json, "}");
    return true;
}

bool get_nmea_gga_json(char *json)
{
    char buffer[AUX_BUF_SIZE];
    memset(buffer, 0, AUX_BUF_SIZE);

    strcat(json, "{\n");

    // Get time
    strcat(json, "\t\"time\": ");
    sprintf(buffer, "%02d:%02d:%02d.%02d,\n", _gga.time.hours, _gga.time.minutes, _gga.time.seconds, _gga.time.microseconds);
    strcat(json, buffer);

    // latitude
    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"latitude\": ");
    sprintf(buffer, "%f,\n", minmea_tocoord(&_gga.latitude));
    strcat(json, buffer);

    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"longitude\": ");
    sprintf(buffer, "%f,\n", minmea_tocoord(&_gga.longitude));
    strcat(json, buffer);

    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"altitude\": ");
    sprintf(buffer, "%f,\n", minmea_tofloat(&_gga.altitude));
    strcat(json, buffer);

    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"a units\": ");
    sprintf(buffer, "\"%c\",\n", _gga.altitude_units);
    strcat(json, buffer);

    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"height\": ");
    sprintf(buffer, "%f,\n", minmea_tofloat(&_gga.height));
    strcat(json, buffer);

    memset(buffer, 0, AUX_BUF_SIZE);
    strcat(json, "\t\"h units\": ");
    sprintf(buffer, "\"%c\",\n", _gga.altitude_units);
    strcat(json, buffer);

    strcat(json, "}");
    return true;
}

void clear_buffer(void)
{
    tsrb_clear(&_tsrb);
}
