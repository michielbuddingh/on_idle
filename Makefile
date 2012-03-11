LANGFLAGS=-pedantic -std=c89
WARNFLAGS=-W -Wall -Wfatal-errors -Wconversion -Wno-sign-conversion -Wstrict-prototypes -Wunreachable-code  -Wwrite-strings -Wpointer-arith -Wbad-function-cast -Wcast-align -Wcast-qual
CC=gcc

on_idle:	on_idle.c
	$(CC) $(LANGFLAGS) $(WARNFLAGS) -o on_idle -I/usr/X11R6/include -L/usr/X11R6/lib -lX11 -lXss on_idle.c -Wl,-rpath=/usr/X11R6/lib -ggdb3

clean:	on_idle
	rm on_idle