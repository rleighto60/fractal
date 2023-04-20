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

#define MAX_THREAD 64
#define MAX_COLOR 50

APIRET APIENTRY handler( PRXSTRING, PUSHORT, PRXSTRING);
APIRET APIENTRY sqrtHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
APIRET APIENTRY powHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);

#define MANDEL     0

#define REAL       1
#define IMAG       2

char hostname[] = "FRACTAL";

double xc, yc, size, pxc, pyc, psize;
double escape;
double complex c0;
unsigned int type = MANDEL, ctype = 0;
int julia = FALSE, pipe = FALSE, ncolor = 2, shift = 0, maxIter = 1000, dmax = 100, nthread = 1;
long xres, yres;
int *buf, *pbuf;
UBYTE palette[3][MAX_COLOR];

void *malloc();
void *realloc();
double pow(), sqrt();
