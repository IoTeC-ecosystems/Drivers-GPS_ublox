#include <stdio.h>

#include "gps_ublox.h"
#include "tests.h"
#include "cUnit.h"

#define STR_LEN 512
#define JSON_LEN 256

char str[STR_LEN];
char json[JSON_LEN];

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

    memset(str, 0, STR_LEN);

    // No device connected for the tests, it should return false
    bool ret = init_gps_ublox(UART_DEV(2), 9600, 2000);
    bool passed = check_condition(true, ret, "Init gps driver\0", str);

    return passed;
}

bool test_parse_nmea_messages_empty_buffer(void *arg)
{
    (void) arg;

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

    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();

    bool passed = true;
    passed = check_condition(passed, ret, "Correctly parsed RMC sentence.\0", str);

    memset(json, 0, JSON_LEN);
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

bool test_get_gga_json(void *arg)
{
    (void) arg;
    char line[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,01,0000*6E\r\n";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();

    bool passed = true;
    passed = check_condition(passed, ret, "Correctly parsed GGA sentence\0", str);

    memset(json, 0, JSON_LEN);
    ret = get_nmea_gga_json(json);

    passed = check_condition(passed, ret, "Json parsed correctly", str);
    ret = strstr(json, "{");
    passed = check_condition(passed, ret, "json contains {", str);
    ret = strstr(json, "}");
    passed = check_condition(passed, ret, "json contains }", str);
    ret = strstr(json, "\"time\": 11:57:39.00");
    passed = check_condition(passed, ret, "json containes \"time\": 11:57:39.00", str);
    ret = strstr(json, "\"latitude\": ");
    passed = check_condition(passed, ret, "json contains \"latitude\": 41.980763", str);
    ret = strstr(json, "\"longitude\": ");
    passed = check_condition(passed, ret, "json contains \"longitude\": ", str);
    ret = strstr(json, "\"altitude\": ");
    passed = check_condition(passed, ret, "json contains \"altitude\": ", str);
    ret = strstr(json, "\"a units\": ");
    passed = check_condition(passed, ret, "json contains \"a units\": ", str);
    ret = strstr(json, "\"height\": ");
    passed = check_condition(passed, ret, "json contains \"height\": ", str);
    ret = strstr(json, "\"h units\": ");
    passed = check_condition(passed, ret, "json contains \"h units\": ", str);

    if (!passed) {
        printf("%s\n", str);
    }

    return passed;
}

bool test_get_gll_json(void *arg)
{
    (void) arg;
    char line[] = "$GPGLL,4112.26,N,11332.22,E,213276,A*29\r\n\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();

    bool passed = true;
    passed = check_condition(passed, ret, "Correclty parsed GLL sentence", str);

    memset(json, 0, JSON_LEN);
    ret = get_nmea_gll_json(json);

    passed = check_condition(passed, ret, "Json parsed correctly", str);
    ret = strstr(json, "{");
    passed = check_condition(passed, ret, "json contains {", str);
    ret = strstr(json, "}");
    passed = check_condition(passed, ret, "json contains }", str);
    ret = strstr(json, "\"time\":");
    passed = check_condition(passed, ret, "json containes \"time\":", str);
    ret = strstr(json, "\"latitude\":");
    passed = check_condition(passed, ret, "json containes \"longitude\":", str);
    ret = strstr(json, "\"latitude\":");
    passed = check_condition(passed, ret, "json containes \"latitude\":", str);

    if (!passed) {
        printf("%s\n", str);
    }

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
    cunit_add_test(tests, test_get_gga_json, "get_nmea_gga_json");
    cunit_add_test(tests, test_get_gll_json, "get_nmea_gll_json");

    cunit_execute_tests(tests);

    cunit_terminate(&tests);
}
