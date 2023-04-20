#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <X11/Xlib.h>

typedef unsigned char UBYTE;

#define FALSE 0
#define TRUE 1

#define PGM_FORMAT ((long)'P' * 256L + (long)'2')
#define RPGM_FORMAT ((long)'P' * 256L + (long)'5')
#define PPM_FORMAT ((long)'P' * 256L + (long)'3')
#define RPPM_FORMAT ((long)'P' * 256L + (long)'6')

int Width, Height;
int *buf;
int pipe = FALSE;
