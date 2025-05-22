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
    for (uint8_t i = 0; i < sizeof(cfg_rate); i++) {
        printf("%x ", cfg_rate[i]);
    }

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

    return true;
}
