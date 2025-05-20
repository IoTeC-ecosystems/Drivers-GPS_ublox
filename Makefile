APPLICATION = gps_ublox_driver
BOARD = bluepill-stm32f103cb
RIOTBASE ?= ../../../RTOS/RIOT
DEVHELP ?= 1
DEVELHELP ?= 1

INCLUDES += -I$(CURDIR)/include

FEATURE_REQUIRED += periph_gpio
FEATURE_REQUIRED += periph_uart

USEMODULE += ztimer
USEMODULE += ztimer_msec
USEMODULE += tsrb

ifneq ($(BOARD), native)
USEMODULE += stdio_uart
USEMODULE += printf_float
endif

ifdef TEST
USEMODULE += tests
EXTERNAL_MODULE_DIRS += $(CURDIR)
endif

include $(RIOTBASE)/Makefile.include
