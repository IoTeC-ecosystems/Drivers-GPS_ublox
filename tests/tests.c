#include <stdio.h>

#include "gps_ublox.h"
#include "tests.h"
#include "cUnit.h"

struct data {
    uint8_t *_data;
};

void setup(void *arg) {
    (void) arg;
}

void teardown(void *arg) {
    (void) arg;
}

void tests(void)
{
    printf("Testing the ublox driver.\n");

    cUnit_t *tests;
    struct data data;

    cunit_init(&tests, &setup, &teardown, (void *)&data);

    //cunit_execute_tests(tests);

    cunit_terminate(&tests);
}
