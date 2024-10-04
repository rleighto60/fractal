#
CC = gcc -O3 -Wall -fomit-frame-pointer
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4)
LIBS = $(shell $(PKGCONFIG) --libs gtk4)

all: ../bin/fractal ../bin/ifftoppm ../bin/iffinfo

test: ../bin/arbfunctest ../bin/arbfractaltest

../bin/fractal:	fractal.o parse.o fractalfunc.o complexfunc.o arbfunc.o iff.o
	$(CC) -o ../bin/fractal fractal.o parse.o fractalfunc.o complexfunc.o arbfunc.o iff.o -lregina -lX11 -lm -lflint -ldl -pthread

../bin/ifftoppm:ifftoppm.o iff.o
	$(CC) -o ../bin/ifftoppm ifftoppm.o iff.o -lm

../bin/iffinfo:iffinfo.o iff.o
	$(CC) -o ../bin/iffinfo iffinfo.o iff.o -lm

../bin/arbfunctest: arbfunctest.o fractalfunc.o complexfunc.o arbfunc.o iff.o
	$(CC) -o ../bin/arbfunctest arbfunctest.o fractalfunc.o complexfunc.o arbfunc.o iff.o -lm -lflint

../bin/arbfractaltest: arbfractaltest.o fractalfunc.o complexfunc.o arbfunc.o iff.o
	$(CC) -o ../bin/arbfractaltest arbfractaltest.o fractalfunc.o complexfunc.o arbfunc.o iff.o -lm -lflint -pthread

fractal.o:  fractal.c fractal.h iff.h
	$(CC) -c -o fractal.o fractal.c

ifftoppm.o:  ifftoppm.c fractal.h iff.h
	$(CC) -c -o ifftoppm.o ifftoppm.c

iffinfo.o:  iffinfo.c fractal.h iff.h
	$(CC) -c -o iffinfo.o iffinfo.c

iff.o:    iff.c
	$(CC) -c -o iff.o iff.c

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

arbfractaltest.o:  arbfractaltest.c arbfunc.h iff.h
	$(CC) -c -o arbfractaltest.o arbfractaltest.c

clean:
	rm -f *.o
