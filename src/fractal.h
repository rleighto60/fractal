/*
 * fractal.h
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include <stdio.h>
#include <complex.h>
#include <ctype.h>
#include <math.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <X11/Xlib.h>

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

#define FALSE 0
#define TRUE 1

#define MANDEL     0
#define REAL       1
#define IMAG       2

#define MAX_THREAD 64
#define MAX_INDICES 100

struct FDATA {
    char file[2048];
    int shift, nindex;
    int indices[MAX_INDICES];
    UBYTE comps[3][MAX_INDICES];
    UBYTE color[3];
};

APIRET APIENTRY handler( PRXSTRING, PUSHORT, PRXSTRING);
APIRET APIENTRY sqrtHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);
APIRET APIENTRY powHandler(PSZ, ULONG, PRXSTRING, PSZ, PRXSTRING);

void *malloc();
void *realloc();
