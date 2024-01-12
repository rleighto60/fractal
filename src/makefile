#
CC = gcc -O3 -Wall -fomit-frame-pointer
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4)
LIBS = $(shell $(PKGCONFIG) --libs gtk4)

all: ../bin/fractal ../bin/display

../bin/fractal:	fractal.o parse.o
	$(CC) -o ../bin/fractal fractal.o parse.o -lregina -lX11 -lm -ldl -pthread

../bin/display:	display.o
	$(CC) -o ../bin/display display.o $(LIBS)

fractal.o:  fractal.c fractal.h
	$(CC) -c -o fractal.o fractal.c

display.o:  display.c display.h
	$(CC) -c -o display.o $(CFLAGS) display.c

parse.o:    parse.c
	$(CC) -c -o parse.o parse.c

clean:
	rm -f *.o
