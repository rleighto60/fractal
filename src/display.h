#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <gtk/gtk.h>

typedef unsigned char UBYTE;

#define PGM_FORMAT ((long)'P' * 256L + (long)'2')
#define RPGM_FORMAT ((long)'P' * 256L + (long)'5')
#define PPM_FORMAT ((long)'P' * 256L + (long)'3')
#define RPPM_FORMAT ((long)'P' * 256L + (long)'6')
