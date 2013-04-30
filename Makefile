# Copyright 2008-2013, Michiel Buddingh, All rights reserved.
# Use of this code is governed by version 2.0 or later of the Apache
# License, available at http://www.apache.org/licenses/LICENSE-2.0
LANGFLAGS=-pedantic -std=c99
WARNFLAGS=-W -Wall -Wfatal-errors -Wconversion -Wno-sign-conversion -Wstrict-prototypes -Wunreachable-code  -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-align -Wcast-qual
CC=gcc
XCBLIBS=$(shell pkg-config --cflags --libs xcb xcb-screensaver)

on_idle:	on_idle.c
	$(CC) $(CFLAGS) $(LANGFLAGS) $(WARNFLAGS) $(XCBLIBS) -o on_idle on_idle.c

clean:
	-rm on_idle
