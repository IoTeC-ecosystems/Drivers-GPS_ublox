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
    char line[] = "$GPGLL,2109.97510,N,10141.45142,W,002006.00,A,A*7C\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    char str[STR_LEN];
    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();
    bool passed = true;
    passed = check_condition(passed, !ret, "Parse nmea message with no CR and LN\0", str);
    if (!passed) {
        printf("%s\n", str);
    }
    clear_buffer();

    return passed;
}

bool test_get_rmc_json(void *arg)
{
    (void) arg;

    char line[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\r\n\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    char str[STR_LEN];
    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();

    bool passed = true;
    passed = check_condition(passed, ret, "Correctly parsed RMC sentence.\0", str);

    char json[256];
    memset(json, 0, 256);
    ret = get_nmea_rmc_json(json);

    passed = check_condition(passed, ret, "Json correctly parsed", str);
    ret = strstr(json, "datetime");
    passed = check_condition(passed, ret, "Json contains \"datetime\" field", str);
    ret = strstr(json, "course");
    passed = check_condition(passed, ret, "Json contains \"course\" field", str);
    ret = strstr(json, "latitude");
    passed = check_condition(passed, ret, "Json contains \"latitude\" field", str);
    ret = strstr(json, "longitude");
    passed = check_condition(passed, ret, "Json contains \"longitude\" field", str);
    ret = strstr(json, "speed");
    passed = check_condition(passed, ret, "Json contains \"speed\" field", str);

    if (!passed) {
        printf("%s\n", str);
    }

    clear_buffer();
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
    cunit_add_test(tests, test_get_rmc_json, "get_nmea_rmc_json");

    cunit_execute_tests(tests);

    cunit_terminate(&tests);
}
