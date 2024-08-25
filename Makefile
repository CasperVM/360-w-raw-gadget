# SPDX-License-Identifier: Apache-2.0

CC=gcc
CFLAGS=-O2 -Wall -g
LDFLAGS = -lpthread

SRCS = 360-gadget.c usb_descriptors.c usb_io.c usb_helpers.c
OBJS = $(SRCS:.c=.o)
TARGET = 360-gadget

.PHONY: all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LDFLAGS)

