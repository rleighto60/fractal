#
CC = gcc -O3 -Wall -fomit-frame-pointer -pthread
LIBS = -lregina -lm -ldl

OFILES = fractal.o parse.o

../bin/fractal:	$(OFILES)
	$(CC) -o ../bin/fractal $(OFILES) $(LIBS)

fractal.o:  fractal.c fractal.h
	$(CC) -c -o fractal.o fractal.c

parse.o:    parse.c
	$(CC) -c -o parse.o parse.c

clean:
	rm -f *.o
