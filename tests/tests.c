#include <stdio.h>

#include "gps_ublox.h"
#include "tests.h"
#include "cUnit.h"

#define STR_LEN 512

struct data {
    uint8_t *_data;
};

void setup(void *arg) {
    (void) arg;
}

void teardown(void *arg) {
    (void) arg;
}

bool test_init_gps_ublox(void *arg)
{
    (void) arg;

    char str[STR_LEN];
    memset(str, 0, STR_LEN);

    // No device connected for the tests, it should return false
    bool ret = init_gps_ublox(UART_DEV(2), 9600, 2000);
    bool passed = check_condition(true, ret, "Init gps driver\0", str);

    return passed;
}

bool test_parse_nmea_messages_empty_buffer(void *arg)
{
    (void) arg;

    char str[STR_LEN];
    memset(str, 0, STR_LEN);

    bool ret = parse_nmea_message();
    bool passed = true;
    passed = check_condition(passed, ret == false, "Parse nmea message with emtpy buffer\0", str);

    return passed;
}

bool test_parse_nmea_messages_no_cr_lf(void *arg)
{
    (void) arg;
    char line[] = "$GPGLL,2109.97510,N,10141.45142,W,002006.00,A,A*7C\r\n\0";
    for (uint8_t i = 0; i < sizeof(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    char str[124];
    memset(str, 0, 12);
    bool ret = parse_nmea_message();
    bool passed = true;
    passed = check_condition(passed, ret, "Parse nmea message with no CR and LN\0", str);

    return passed;
}

void tests(void)
{
    printf("Testing the ublox driver.\n");

    cUnit_t *tests;
    struct data data;

    cunit_init(&tests, &setup, &teardown, (void *)&data);

    cunit_add_test(tests, test_init_gps_ublox, "init_gps_ublox");
    cunit_add_test(tests, test_parse_nmea_messages_empty_buffer, "parse_nmea_messages");
    cunit_add_test(tests, test_parse_nmea_messages_no_cr_lf, "parse_nmea_messages");

    cunit_execute_tests(tests);

    cunit_terminate(&tests);
}
