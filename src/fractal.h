/*
 * fractal.h
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include <X11/Xlib.h>
#include <malloc.h>
#include <pthread.h>

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned int UINT;
typedef unsigned long ULONG;
typedef short WORD;
typedef char *STRPTR;

#define INCL_RXSUBCOM
#define INCL_RXSHV
#define INCL_RXFUNC
#define ULONG_TYPEDEFED

#include <rexxsaa.h>

#define MANDEL 0

#define MAX_THREAD 64
#define MAX_INDICES 100

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define REAL 1
#define IMAG 2

struct ColorData {
  int shift, nindex;
  int indices[MAX_INDICES];
  UBYTE comps[3][MAX_INDICES];
};

struct ViewData {
  long xres, yres;
};

struct FractalData {
  char x0[128], y0[128], xc[128], yc[128], size[128];
};

APIRET APIENTRY handler(PRXSTRING, PUSHORT, PRXSTRING);
APIRET APIENTRY sqrtHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
APIRET APIENTRY powHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
