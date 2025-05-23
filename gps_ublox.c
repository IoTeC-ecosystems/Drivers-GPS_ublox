#include <stdio.h>

#include "gps_ublox.h"

static void gps_cb(void *arg, uint8_t data)
{
    (void) arg;
    // Add datat to the ring buffer
    tsrb_add_one(&_tsrb, data);
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
    (void) rate;
    tsrb_clear(&_tsrb);

    _uart_dev = _dev;
    uart_init(_uart_dev, baud, gps_cb, NULL);

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
        0xb5, 0x62, 0x06, 0x24, 0x24, 0x00,
        // payload
        0x07, 0x00,                 // mask
        0x02,                       // dynModel - static
        0x03,                       // Fix mode - auto
        0x00, 0x00, 0x00, 0x00,     // fixedAlt
        0x00, 0x00, 0x00, 0x00,     // fixedAltVar
        0x14,                       // minElev - for satellite: 20° above horizon
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

    return true;
}
