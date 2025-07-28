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

bool test_parse_nmea_rmc_imcomplete(void *arg)
{
    // Is able to parse incomplete nmea sentences
    (void) arg;
    char line[] = "$GPRMC,123519,A,4807.083,N,01131.000,E,022.4,,230394,003.1,W*4C\r\n\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();

    bool passed = true;
    passed = check_condition(passed, ret, "Correctly parsed RMC sentence with no course.\0", str);

    char line2[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,,*11\r\n\0";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Correctly parsed RMC sentence with no variation.\0", str);

    char line3[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,,230394,,*37\r\n\0";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Correctly parsed RMC sentence with no course and no variation\0", str);

    return passed;
}

bool test_parse_nmea_rmc_incomplete_invalid(void *arg)
{
    (void) arg;
    // No time
    char line[] = "$GPRMC,,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*67\r\n\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }
    int ret = parse_nmea_message();
    bool passed = true;
    memset(str, 0, STR_LEN);
    passed = check_condition(passed, !ret, "Parse RMC with no time\0", str);

    char line2[] = "$GPRMC,123519,A,,,01131.000,E,022.4,084.4,230394,003.1,W*3A\r\n\0";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse RMC with no latitude\0", str);

    char line3[] = "$GPRMC,123519,A,4807.038,N,,,022.4,084.4,230394,003.1,W*03\r\n\0";
    for (uint8_t i = 0; i < strlen(line3); i++) {
        gps_cb(NULL, (uint8_t)line3[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse RMC with no longitude\0", str);

    char line4[] = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,,003.1,W*65\r\n\0";
    for (uint8_t i = 0; i < strlen(line4); i++) {
        gps_cb(NULL, (uint8_t)line4[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse RMC with no date\0", str);

    if (!passed) {
        printf("%s\n", str);
    }

    return passed;
}

bool test_get_nmea_gga_incomplete_valid(void *arg)
{
    (void) arg;

    char line[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,,13,0.9,255.747,M,-32.00,M,01,0000*5A\r\n";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }

    memset(str, 0, STR_LEN);
    bool ret = parse_nmea_message();
    bool passed = true;
    passed = check_condition(passed, ret, "Parse GGA with no gps quality indicator", str);

    char line2[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,,0.9,255.747,M,-32.00,M,01,0000*6C\r\n";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no number of satellites", str);

    char line3[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,,255.747,M,-32.00,M,01,0000*49\r\n";
    for (uint8_t i = 0; i < strlen(line3); i++) {
        gps_cb(NULL, (uint8_t)line3[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no HDOP", str);

    char line4[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,,,-32.00,M,01,0000*0B\r\n";
    for (uint8_t i = 0; i < strlen(line4); i++) {
        gps_cb(NULL, (uint8_t)line4[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no orthomeric height", str);

    char line5[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,,,01,0000*21\r\n";
    for (uint8_t i = 0; i < strlen(line5); i++) {
        gps_cb(NULL, (uint8_t)line5[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no geoid separation", str);

    char line6[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,,0000*6F\r\n";
    for (uint8_t i = 0; i < strlen(line6); i++) {
        gps_cb(NULL, (uint8_t)line6[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no diferential gps", str);

    char line7[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,01,*6E\r\n";
    for (uint8_t i = 0; i < strlen(line7); i++) {
        gps_cb(NULL, (uint8_t)line7[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, ret, "Parse GGA with no diferential gps", str);

    if (!passed) {
        printf("%s\n", str);
    }
    return passed;
}

bool test_get_nmea_gga_incomplete_invalid(void *arg)
{
    (void) arg;
    memset(str, 0, STR_LEN);
    bool passed = true;

    char line[] = "$GPGGA,,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,01,0000*48\r\n";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }
    bool ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GGA with no UTC time", str);

    char line2[] = "$GPGGA,115739.00,,,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,01,0000*3D\r\n";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GGA with no latitude", str);

    char line3[] = "$GPGGA,115739.00,4158.8441367,N,,,4,13,0.9,255.747,M,-32.00,M,01,0000*19\r\n";
    for (uint8_t i = 0; i < strlen(line3); i++) {
        gps_cb(NULL, (uint8_t)line3[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GGA with no longitude", str);

    if (!passed) {
        printf("%s", str);
    }
    //char line[] = "$GPGGA,115739.00,4158.8441367,N,09147.4416929,W,4,13,0.9,255.747,M,-32.00,M,01,0000*6E\r\n";
    return passed;
}

bool test_get_gll_incomplete_invalid(void *arg)
{
    (void) arg;
    memset(str, 0, STR_LEN);
    bool passed = true;
    char line[] = "$GPGLL,,,11332.22,E,213276,A*4B\r\n\0";
    for (uint8_t i = 0; i < strlen(line); i++) {
        gps_cb(NULL, (uint8_t)line[i]);
    }
    bool ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GLL with no longitude", str);

    char line2[] = "$GPGLL,4112.26,N,,,213276,A*70\r\n\0";
    for (uint8_t i = 0; i < strlen(line2); i++) {
        gps_cb(NULL, (uint8_t)line2[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GLL with no latitude", str);

    char line3[] = "$GPGLL,4112.26,N,11332.22,E,,A*2A\r\n\0";
    for (uint8_t i = 0; i < strlen(line3); i++) {
        gps_cb(NULL, (uint8_t)line3[i]);
    }
    ret = parse_nmea_message();
    passed = check_condition(passed, !ret, "Parse GLL with no time", str);

    if (!passed) {
        printf("%s", str);
    }

    //char line[] = "$GPGLL,4112.26,N,11332.22,E,213276,A*29\r\n\0";
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
    cunit_add_test(tests, test_parse_nmea_rmc_imcomplete, "parse_nmea_messages rmc incomplete valid");
    cunit_add_test(tests, test_parse_nmea_rmc_incomplete_invalid, "parse_nmea rmc incomplete invalid");
    cunit_add_test(tests, test_get_nmea_gga_incomplete_valid, "parse_nmea_gga imcomplete valid");
    cunit_add_test(tests, test_get_nmea_gga_incomplete_invalid, "parse_nmea_gga incomplete invalid");
    /* There is only one posible GLL valid sentence */
    cunit_add_test(tests, test_get_gll_incomplete_invalid, "parse_nmea_gll incomplete invalid");
    cunit_add_test(tests, test_get_rmc_json, "get_nmea_rmc_json");
    cunit_add_test(tests, test_get_gga_json, "get_nmea_gga_json");
    cunit_add_test(tests, test_get_gll_json, "get_nmea_gll_json");

    cunit_execute_tests(tests);

    cunit_terminate(&tests);
}
