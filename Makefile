# SPDX-License-Identifier: Apache-2.0

CC=gcc
CFLAGS=-O2 -Wall -g
LDFLAGS = -lpthread

SRCS = 360-gadget.c usb_descriptors.c usb_io.c usb_helpers.c
OBJS = $(SRCS:.c=.o)
TARGET = 360-gadget


$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

.PHONY: rpi4
rpi4: 
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_RPI4" $(TARGET)

.PHONY: rpi5
rpi5:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_RPI5" $(TARGET)

.PHONY: nanopi-neo2
nanopi-neo2:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_NANOPI_NEO2" $(TARGET)

.PHONY: usb-armory-mk2
usb-armory-mk2:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_USB_ARMORY_MK2" $(TARGET)

.PHONY: orange-pi-pc
orange-pi-pc:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_ORANGE_PI_PC" $(TARGET)

.PHONY: khadas-vim1
khadas-vim1:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_KHADAS_VIM1" $(TARGET)

.PHONY: thinkpad-x1
thinkpad-x1:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_THINKPAD_X1" $(TARGET)

.PHONY: nxp-imx8mp
nxp-imx8mp:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_NXP_IMX8MP" $(TARGET)

.PHONY: beaglebone-black
beaglebone-black:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_BEAGLEBONE_BLACK" $(TARGET)

.PHONY: beaglebone-ai
beaglebone-ai:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_BEAGLEBONE_AI" $(TARGET)

.PHONY: ec3380-ab
ec3380-ab:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_EC3380_AB" $(TARGET)

.PHONY: odroid-c2
odroid-c2:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_ODROID_C2" $(TARGET)

.PHONY: generic
generic:
	$(MAKE) CFLAGS="$(CFLAGS) -DHW_GENERIC" $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
