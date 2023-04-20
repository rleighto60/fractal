#
CC = gcc -O3 -Wall -fomit-frame-pointer -pthread

default: ../bin/fractal ../bin/display

../bin/fractal:	fractal.o parse.o
	$(CC) -o ../bin/fractal fractal.o parse.o -lregina -lm -ldl

../bin/display:	display.o
	$(CC) -o ../bin/display display.o -lX11

fractal.o:  fractal.c fractal.h
	$(CC) -c -o fractal.o fractal.c

display.o:  display.c display.h
	$(CC) -c -o display.o display.c

parse.o:    parse.c
	$(CC) -c -o parse.o parse.c

clean:
	rm -f *.o
