#include <stdio.h>
#include <string.h>

#include "gps_ublox.h"

void gps_cb(void *arg, uint8_t data)
{
    (void) arg;
    if (data == '\n') return;
    // Add data to the ring buffer
    tsrb_add_one(&_tsrb, data);
    if (data == '\r') {
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
    }

    while (1) {
        msg_t msg;
        msg_receive(&msg);
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
                break;
            }
        }
    }

    return true;
}

bool parse_nmea_message(void)
{
    char line[MINMEA_MAX_SENTENCE_LENGTH];
    // Read the tsrb buffer
    msg_t msg;
    if (msg_try_receive(&msg) == -1) {
        return false;
    }
    int i = 0;
    uint8_t c = 0;
    while (c  != '\r') {
        if (tsrb_avail(&_tsrb)) {
            c = tsrb_get_one(&_tsrb);
            line[i++] = (char)c;
        }
    }

    for (int i = 0; i < MINMEA_MAX_SENTENCE_LENGTH; i++) {
        if (line[i] == '\r') {
            printf("\\r\n");
            break;
        } else {
            printf("%c", line[i]);
        }
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
