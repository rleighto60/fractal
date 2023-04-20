/*
 * fractal.c
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include "fractal.h"

extern int parse(char *input, char *args[], int narg);

/* Open stream. If file specification begins with '|' then open as a piped
 stream otherwise open as a file */

FILE* open_file(char *fspec, char *mode) {
    if (*fspec == '|') {
        ++fspec;
        pipe = TRUE;
        return popen(fspec, mode);
    } else {
        pipe = FALSE;
        return fopen(fspec, mode);
    }
}

/* Close stream. */

int close_file(FILE *stream) {
    int rc = 0;

    if (pipe)
        rc = pclose(stream);
    else
        rc = fclose(stream);
    pipe = FALSE;

    return rc;
}

int open_buffer() {
    long buflen;

    buflen = xres * yres;

	if ((buf = calloc((long) buflen, sizeof(int))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
		return (0);
	}
	if ((pbuf = calloc((long) buflen, sizeof(int))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
		return (0);
	}
    return (1);
}

void close_buffer() {
    free(buf);
    free(pbuf);
}

void save_ppm(char *savefile) {
    FILE *ifp;
    long i, j, pos;

    if ((ifp = open_file(savefile, "w")) == NULL) {
        fprintf(stderr, "save - could not open file!!!\n");
        return;
    }

    fprintf(ifp, "P6\n%ld %ld\n255\n", xres, yres);
    pos = 0;
    for (j = 0; j < yres; j++) {
        for (i = 0; i < xres; i++) {
            putc((buf[pos] & 0xff0000) >> 16, ifp);
            putc((buf[pos] & 0x00ff00) >> 8, ifp);
            putc((buf[pos] & 0x0000ff), ifp);
            pos++;
        }
    }
    fflush(ifp);
    close_file(ifp);
}

UBYTE get_color(int iter, int i) {
    if (iter == maxIter)
        return 0;

    int n = (double) iter / dmax;
    int diter = iter - n * dmax;
    double f = 1.0 - (double) diter / dmax;
    int j = (n + shift) % ncolor;
    int k = j == ncolor - 1 ? 0 : j + 1;

    return palette[i][k] + (palette[i][j] - palette[i][k]) * f;
}

double crad2(double complex z) {
    switch (ctype) {
    case REAL:
        return creal(z * z);
    case IMAG:
        return cimag(z * z);
    default:
        double x = creal(z);
        double y = cimag(z);
        return x * x + y * y;
    }
    return creal(z);
}

int iterate(double complex z, double complex c) {
    int iter = 0;

    while (crad2(z) <= escape && iter < maxIter) {
        z = z * z + c; // type == MANDEL
        iter++;
    }
    return iter;
}

int sample_pbuf(int dx, int pos) {
	if (pos < 0) return FALSE;
	if (dx == 1) return (pbuf[pos] >> 24);
	if (dx > 0) {
		int flag = TRUE;
		int dy = dx * xres;
		for (int py = 0; py <= dy; py += xres) {
			if (!(pbuf[pos + py] >> 24))
				flag = FALSE;
			for (int px = 1; px <= dx; px++) {
				if (!(pbuf[pos + py + px] >> 24))
					flag = FALSE;
				if (!(pbuf[pos + py - px] >> 24))
					flag = FALSE;
				if (!(pbuf[pos - py + px] >> 24))
					flag = FALSE;
				if (!(pbuf[pos - py - px] >> 24))
					flag = FALSE;
			}
		}
		return flag;
	}
	return FALSE;
}

int get_pos(int x, int y, int d) {
	if ((x < d) || (x > xres - d)) return -1;
	if ((y < d) || (y > yres - d)) return -1;
	return xres * y + x;
}

void generate_fractal(int x1) {
    double dx, dy, pdx, pdy, x, y;
    double complex z;
    int iter, pos, ppos, ppx, ppy, dp;
    UBYTE rc, gc, bc;

    if (xres > yres) {
        dx = size * (double) xres / yres;
        dy = size;
        if (psize >= size) {
        	dp = (0.9999 + psize / size);
            pdx = psize * (double) xres / yres;
            pdy = psize;
        }
        else {
        	dp = 0;
            pdx = dx;
            pdy = dy;
        }
    } else {
        dx = size;
        dy = size * (double) yres / xres;
        if (psize >= size) {
        	dp = (0.9999 + psize / size);
            pdx = psize;
            pdy = psize * (double) yres / xres;
        }
        else {
        	dp = 0;
            pdx = dx;
            pdy = dy;
        }
    }

    for (int py = 0; py < yres; py++) {
        y = yc + dy * (((double) py / yres) - 0.5);
        ppy = (double) yres * (((y - pyc) / pdy) + 0.5);
        for (int px = x1; px < xres; px += nthread) {
            x = xc + dx * (((double) px / xres) - 0.5);
            ppx = (double) xres * (((x - pxc) / pdx) + 0.5);
            ppos = get_pos(ppx, ppy, dp);
            pos = xres * py + px;
            if (sample_pbuf(dp, ppos)) {
            	buf[pos] = 1 << 24;
            	continue;
            }
            z = x + y * I;
            iter = maxIter;

            if (julia) iter = iterate(z, c0);
            else iter = iterate(0.0, z);

            if (iter == maxIter) {
            	buf[pos] = 1 << 24;
            } else {
				rc = get_color(iter, 0);
				gc = get_color(iter, 1);
				bc = get_color(iter, 2);
				buf[pos] = rc << 16 | gc << 8 | bc;
            }
        }
    }
}

void* _generate_fractal(void *arg) {
    int *px1 = (int*) arg;

    generate_fractal(*px1);
    pthread_exit(NULL);
}

int fractal() {
    int it, rc, xs[MAX_THREAD];
    pthread_t thr[MAX_THREAD];

    for (it = 0; it < nthread; ++it) {
        xs[it] = it;
        if ((rc = pthread_create(&thr[it], NULL, _generate_fractal, &xs[it]))) {
            fprintf(stderr, "main - error: pthread_create, rc: %d\n",
                    rc);
            return 0;
        }
    }

    for (it = 0; it < nthread; ++it) {
        pthread_join(thr[it], NULL);
    }
    memcpy(pbuf, buf, xres * yres * sizeof(int));
    return 1;
}

int spectrum() {
    int iter, bpos, max = ncolor * dmax;
    UBYTE rc, gc, bc;

    for (int px = 0; px < xres; px++) {
        iter = (double) (max * px) / xres;
        rc = get_color(iter, 0);
        gc = get_color(iter, 1);
        bc = get_color(iter, 2);
        bpos = px;

        for (int py = 0; py < yres; py++) {
            buf[bpos] = rc << 16 | gc << 8 | bc;
            bpos += xres;
        }
    }
    return 0;
}

void reset() {
    xres = 320L;
    yres = 200L;
    xc = pxc= -0.75;
    yc = pyc = 0.0;
    c0 = 0.0;
    size = 2.5;
    psize = 0.0;
    escape = 4.0;
    dmax = 100;
    nthread = 1;
    palette[0][0] = palette[1][0] = palette[2][0] = 0;
    palette[0][1] = palette[1][1] = palette[2][1] = 255;
    ncolor = 2;
    shift = 0;
    type = MANDEL;
    ctype = 0;
}

#define MAXARG 80

APIRET APIENTRY handler(PRXSTRING command, PUSHORT flags,
        PRXSTRING returnstring) {
    char *args[MAXARG], *args0;
    char *icom, *ocom;
    char commands[6][10] = { "reset", "palette", "view", "fractal", "spectrum", "save" };
    int ncom = 6;
    int i, m, n, nc, argn;

    if (command->strptr != NULL) {
        for (i = 0; i < MAXARG; i++)
            args[i] = NULL;

        args0 = (char*) malloc((long) (command->strlength + 1));
        if (args0 == NULL) {
            fprintf(stderr, "main - could not allocate memory for args\n");
            return 0;
        }
        strcpy(args0, command->strptr);
        argn = parse(args0, args, MAXARG);

        m = 0;
        nc = ncom;
        for (i = 0; i < ncom; i++) {
            icom = args[0];
            ocom = commands[i];
            n = 0;
            while (((*icom | ' ') == *ocom) && (*icom != 0) && (*ocom != 0)) {
                n++;
                icom++;
                ocom++;
            }
            if (*icom == 0) {
                if (n > m) {
                    m = n;
                    nc = i;
                } else if (n == m)
                    nc = -1;
            }
        }

        switch (nc) {

        /*******************************************************************************
         reset(xres,yres,maxIter,nthread) - reset
         where:   xres    = x resolution of view
                  yres    = y resolution of view
                  maxIter = maximum number of iterations
                  nthread = number of threads
         result:  none
         *******************************************************************************/
        case 0:
            close_buffer();
            reset();
            if (argn > 1)
                sscanf(args[1], "%ld", &xres);
            if (argn > 2)
                sscanf(args[2], "%ld", &yres);
            if (argn > 3)
                sscanf(args[3], "%d", &maxIter);
            if (argn > 4)
                sscanf(args[4], "%d", &nthread);
            if (!open_buffer()) {
                fprintf(stderr,
                        "main - could not allocate memory for bitmap\n");
                return 0;
            }

            if (nthread > MAX_THREAD) {
                fprintf(stderr,
                        "main - number of threads must be no greater than %d\n",
                        MAX_THREAD);
                return 0;
            }
            break;

        /*******************************************************************************
         palette(dmax, shift, colors...) - palette
         where:   colors   = RGB (24 bit hexadecimal) values
         result:  none
         *******************************************************************************/
        case 1:
            unsigned int r, g, b;

            if (argn > 1)
                sscanf(args[1], "%d", &dmax);
            if (argn > 2)
                sscanf(args[2], "%d", &shift);
            if (argn > 3) {
                ncolor = argn - 3;
                if (ncolor >= MAX_COLOR) {
                    fprintf(stderr,
                            "main - number of palette colors must be less than %d\n",
                            MAX_COLOR);
                    return (0);
                }
                for (i = 0; i < ncolor; ++i) {
                    if (sscanf(args[i + 3], "%2x%2x%2x", &r, &g, &b) == 3) {
                        palette[0][i] = r;
                        palette[1][i] = g;
                        palette[2][i] = b;
                    }
                }
            }
            break;

        /*******************************************************************************
         view(xc,yc,size) - view
         where:   xc    = center x coordinate
                  yc    = center y coordinate
                  size  = size
         result:  none
         *******************************************************************************/
        case 2:
        	pxc = xc;
        	pyc = yc;
        	psize = size;
            if (argn > 1)
                sscanf(args[1], "%lg", &xc);
            if (argn > 2)
                sscanf(args[2], "%lg", &yc);
            if (argn > 3)
                sscanf(args[3], "%lg", &size);
            break;

        /*******************************************************************************
         fractal(type,ctype,cx,cy) - generate fractal (julia if c0 = cx + cy I defined)
         where:   type = 0 (MANDEL)
                  ctype = escape type (0 = re(z^2) + im(r^2), 1 = re(z^2), 2 = im(z^2))
                  cx    = constant x coordinate
                  cy    = constant y coordinate
         result:  none
         *******************************************************************************/
        case 3:
            double cx = 0.0, cy = 0.0;

            if (argn > 1)
                sscanf(args[1], "%d", &type);
            if (argn > 2)
                sscanf(args[2], "%d", &ctype);
            if (argn > 4) {
                sscanf(args[3], "%lg", &cx);
                sscanf(args[4], "%lg", &cy);
                c0 = cx + cy * I;
                julia = TRUE;
            } else {
            	julia = FALSE;
            }
            if (type != MANDEL) {
                fprintf(stderr,
                        "main - type not supported: %d\n",
                        type);
                return (0);
            }
        	type = MANDEL;
            if (!fractal()) {
                return (0);
            }
        	pxc = xc;
        	pyc = yc;
            break;

        /*******************************************************************************
         spectrum - view palette color spectrum
         result:  none
         *******************************************************************************/
        case 4:
            if (!open_buffer()) {
                close_buffer();
                fprintf(stderr,
                        "main - could not allocate memory for bitmap\n");
                return (0);
            }
            spectrum();
            break;

        /*******************************************************************************
         save(filename) - save image
         where:   filename = the name of the file to store image in
         result:  none
         *******************************************************************************/
        case 5:
            save_ppm(args[1]);
            break;

        case 6:
            fprintf(stderr, "main - unknown command\n");
            break;

        case -1:
            fprintf(stderr, "main - ambiguous command\n");
            break;
        }

        returnstring->strptr = NULL;
        returnstring->strlength = 0;
        if (args0)
            free(args0);
    }
    return 0;
}

APIRET APIENTRY sqrtHandler(
   PSZ name,
   ULONG argc,
   PRXSTRING argv,
   PSZ queuename,
   PRXSTRING returnstring)
{
  extern double sqrt(double arg);
  float arg, val = 0.0;
  char strval[40];

  if (argc > 0) {
    sscanf(argv[0].strptr, "%g", &arg);
    val = (float)sqrt((double)arg);
  }
  sprintf(strval, "%g", val);
  strcpy(returnstring->strptr, strval);
  returnstring->strlength = strlen(strval);
  return 0L;
}

APIRET APIENTRY powHandler(
   PSZ name,
   ULONG argc,
   PRXSTRING argv,
   PSZ queuename,
   PRXSTRING returnstring)
{
  double arg1, arg2, val = 0.0;
  char strval[40];

  if (argc > 1) {
    sscanf(argv[0].strptr, "%lg", &arg1);
    sscanf(argv[1].strptr, "%lg", &arg2);
    val = pow(arg1, arg2);
  }
  sprintf(strval, "%1.28lg", val);
  strcpy(returnstring->strptr, strval);
  returnstring->strlength = strlen(strval);
  return 0L;
}

int main(int argc, char *argv[]) {
    int rc = 0;
    short returnCode;
    RXSTRING Result;

    if (argc > 1) {
        reset();
        RexxRegisterSubcomExe(hostname, (PFN) handler, NULL);
        RexxRegisterFunctionExe( "sqrt", (PFN) sqrtHandler ) ;
        RexxRegisterFunctionExe( "pow", (PFN) powHandler ) ;

        Result.strlength = 200;
        Result.strptr = malloc(200);

        rc = RexxStart(0, NULL, argv[1], 0, hostname, RXCOMMAND, NULL,
                &returnCode, &Result);
        if (rc < 0)
            rc = -rc;
    }
    return rc;
}
