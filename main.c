#include <stdio.h>

#include "periph/uart.h"
#include "ztimer.h"

#include "gps_ublox.h"

#ifdef TEST
#include "tests.h"
#endif

#define GPS_UART UART_DEV(2)
#define GPS_BAUDRATE 9600u

int main(void)
{
    init_gps_ublox(GPS_UART, GPS_BAUDRATE, 2000);

#ifdef TEST
    //tests();
#endif
    while(1) {
        parse_nmea_message();
    }
    return 0;
}
