# SPDX-License-Identifier: Apache-2.0

CC=gcc
CFLAGS=-O2 -Wall -g

.PHONY: all

all: keyboard 360-gadget

keyboard: keyboard.c
	$(CC) -o $@ $< $(CFLAGS) -lpthread
360-gadget: 360-gadget.c
	$(CC) -o $@ $< $(CFLAGS) -lpthread
