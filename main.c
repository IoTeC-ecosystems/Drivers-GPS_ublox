#include <stdio.h>

#include "periph/uart.h"
#include "ztimer.h"

#include "gps_ublox.h"

#ifdef TEST
#include "tests.h"
#endif

int main(void)
{
#ifdef TEST
    tests();
#endif
    return 0;
}
