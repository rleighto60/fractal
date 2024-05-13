/*
 * fractal.h
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include <X11/Xlib.h>
#include <complex.h>
#include <ctype.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

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

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define MANDEL 0
#define REAL 1
#define IMAG 2

#define MAX_THREAD 64
#define MAX_INDICES 100

struct ViewData {
  long xres, yres;
};

struct FractalData {
  double xc, yc, size;
  double x0, y0;
};

struct ColorData {
  int shift, nindex;
  int indices[MAX_INDICES];
  UBYTE comps[3][MAX_INDICES];
};

APIRET APIENTRY handler(PRXSTRING, PUSHORT, PRXSTRING);
APIRET APIENTRY sqrtHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
APIRET APIENTRY powHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
