#
CC = gcc -O3 -Wall -fomit-frame-pointer
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4)
LIBS = $(shell $(PKGCONFIG) --libs gtk4)

all: ../bin/fractal ../bin/ffftoppm ../bin/fffinfo ../bin/display

test: ../bin/arbfunctest ../bin/arbfractaltest

../bin/fractal:	fractal.o parse.o fractalfunc.o complexfunc.o arbfunc.o fff.o
	$(CC) -o ../bin/fractal fractal.o parse.o fractalfunc.o complexfunc.o arbfunc.o fff.o -lregina -lX11 -lm -lflint -ldl -pthread

../bin/ffftoppm:ffftoppm.o fff.o
	$(CC) -o ../bin/ffftoppm ffftoppm.o fff.o -lm

../bin/fffinfo:fffinfo.o fff.o
	$(CC) -o ../bin/fffinfo fffinfo.o fff.o -lm

../bin/display:display.o palette.o fff.o
	$(CC) -o ../bin/display display.o palette.o fff.o -lX11 -lm

../bin/arbfunctest: arbfunctest.o fractalfunc.o complexfunc.o arbfunc.o fff.o
	$(CC) -o ../bin/arbfunctest arbfunctest.o fractalfunc.o complexfunc.o arbfunc.o fff.o -lm -lflint

../bin/arbfractaltest: arbfractaltest.o fractalfunc.o complexfunc.o arbfunc.o fff.o
	$(CC) -o ../bin/arbfractaltest arbfractaltest.o fractalfunc.o complexfunc.o arbfunc.o fff.o -lm -lflint -pthread

fractal.o:  fractal.c fractal.h fff.h
	$(CC) -c -o fractal.o fractal.c

palette.o:  palette.c fractal.h
	$(CC) -c -o palette.o palette.c

ffftoppm.o:  ffftoppm.c fractal.h
	$(CC) -c -o ffftoppm.o ffftoppm.c

fffinfo.o:  fffinfo.c fractal.h
	$(CC) -c -o fffinfo.o fffinfo.c

display.o:  display.c fractal.h
	$(CC) -c -o display.o display.c

fff.o:    fff.c fff.h
	$(CC) -c -o fff.o fff.c

parse.o:    parse.c
	$(CC) -c -o parse.o parse.c

fractalfunc.o:	fractalfunc.c fractal.h
	$(CC) -c -o fractalfunc.o fractalfunc.c

complexfunc.o:  complexfunc.c complexfunc.h fractal.h
	$(CC) -c -o complexfunc.o complexfunc.c

arbfunc.o:  arbfunc.c arbfunc.h fractal.h
	$(CC) -c -o arbfunc.o arbfunc.c

arbfunctest.o:  arbfunctest.c arbfunc.h
	$(CC) -c -o arbfunctest.o arbfunctest.c

arbfractaltest.o:  arbfractaltest.c arbfunc.h fff.h
	$(CC) -c -o arbfractaltest.o arbfractaltest.c

clean:
	rm -f *.o
