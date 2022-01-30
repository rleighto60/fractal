/*
 * fractal.h
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include <stdio.h>

#include <complex.h>
#include <ctype.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#define FALSE 0
#define TRUE 1
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef short WORD;
typedef char *STRPTR;

#define INCL_RXSUBCOM
#define INCL_RXSHV
#define INCL_RXFUNC
#define ULONG_TYPEDEFED
#include <rexxsaa.h>

#define MAX_THREAD 10
#define MAX_COLOR 50

SHVBLOCK result = {
    NULL,
    { 6, (char *)"result" },
    { 0, NULL },
    6,
    320,
    RXSHV_SYSET,
    0
};

APIRET APIENTRY handler( PRXSTRING, PUSHORT, PRXSTRING);

#define RED        0x0001
#define GREEN      0x0002
#define BLUE       0x0004

#define MANDEL     1
#define JULIA      2

#define REAL       1
#define IMAG       2

char hostname[] = "FRACTAL";

double xc, yc, size, escape;
double complex c;
unsigned int mask = 0, type = MANDEL, ctype = 0;
int pipe = FALSE, ncolor = 2, shift = 0, maxIter = 1000, dmax = 100, nthread = 1;
long xres, yres;
UBYTE *rbuf, *gbuf, *bbuf;
UBYTE palette[3][MAX_COLOR];

void *malloc();
void *realloc();
