/*
 * fractal.c
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include "fractal.h"

char hostname[] = "FRACTAL";

double xc, yc, size, pxc, pyc, psize;
double escape, nfac;
double complex c0;
unsigned int type = MANDEL, ctype = 0;
int julia = FALSE, pipe = FALSE, smooth = TRUE, nindex = 3, shift = 0, maxIter = 1000, nthread = 1;
long xres, yres;
float *buf;
UBYTE colors[3][MAX_INDICES] = {{0x00, 0xff, 0x00}, {0x00, 0xff, 0x00}, {0x00, 0xff, 0x00}};
int indices[MAX_INDICES] = {0, 50, 100};

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

	if ((buf = calloc((long) buflen, sizeof(float))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
		return (0);
	}
    return (1);
}

void close_buffer() {
    free(buf);
}

UBYTE get_color(float fiter, int i) {
    if (fiter >= maxIter)
        return 0;

    int n = nindex - 1;
    int maxindex = indices[n];
    float riter = fmodf(fiter + (float)shift, (float)maxindex);
    int in, dindex;

    for (in = 0; in <= n; in++) {
        if (indices[in] > riter) break;
    }
    // if in the first interval or after the last then interpolate between last and first colors
    if (in == 0 || in > n) {
        dindex = indices[0];
        if (dindex > 0) return (UBYTE)((float)colors[i][n] + (float)(colors[i][0] - colors[i][n]) * riter / (float) dindex);
	    return colors[i][0];
    }
    // otherwise interpolate between colors at the start and end of the interval
    else {
        dindex = indices[in] - indices[in-1];
        if (dindex > 0) return (UBYTE)((float)colors[i][in-1] + (float)(colors[i][in] - colors[i][in-1]) * (riter - (float)indices[in-1]) / (float) dindex);
	    return colors[i][in];
    }
}

void save_ppm(char *savefile) {
    FILE *ifp;
    long i, j, pos;
    float fiter;

    if ((ifp = open_file(savefile, "w")) == NULL) {
        fprintf(stderr, "save - could not open file!!!\n");
        return;
    }

    fprintf(ifp, "P6\n%ld %ld\n255\n", xres, yres);
    pos = 0;
    for (j = 0; j < yres; j++) {
        for (i = 0; i < xres; i++) {
        	fiter = buf[pos];
            putc(get_color(fiter, 0), ifp);
            putc(get_color(fiter, 1), ifp);
            putc(get_color(fiter, 2), ifp);
            pos++;
        }
    }
    fflush(ifp);
    close_file(ifp);
}

double crad2(double complex z) {
    double x, y;
    if (ctype == 0 || smooth) {
        x = creal(z);
        y = cimag(z);
        return x * x + y * y;
    }
    switch (ctype) {
    case REAL:
        return creal(z * z);
    case IMAG:
        return cimag(z * z);
    }
    return creal(z);
}

float iterate(double complex z, double complex c) {
    int iter = 0;
    float fiter = 0.0;
    double r = 0.0, r2;

    while (((r2 = crad2(z)) <= escape) && iter < maxIter) {
        z = z * z + c; // type == MANDEL
        r = r2;
        iter++;
    }
    fiter = (float) iter;
    if (smooth && iter < maxIter) {
    	fiter += (float)(1.0 - (double)(log((double)(log(r) / 2.0) / nfac) / nfac));
    }
    return fiter;
}

void generate_fractal(int x1) {
    double dx, dy, x, y;
    double complex z;
    int pos;

    if (xres > yres) {
        dx = size * (double) xres / yres;
        dy = size;
    } else {
        dx = size;
        dy = size * (double) yres / xres;
    }

    for (int py = 0; py < yres; py++) {
        y = yc + dy * (((double) py / yres) - 0.5);
        for (int px = x1; px < xres; px += nthread) {
            x = xc + dx * (((double) px / xres) - 0.5);
            pos = xres * py + px;
            z = x + y * I;

            if (julia) buf[pos] = iterate(z, c0);
            else buf[pos] = iterate(0.0, z);
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
    pthread_attr_t attr;

    /* Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

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
    return 1;
}

int spectrum() {
    int bpos;
    float fiter;
    int max = indices[nindex - 1];

    for (int px = 0; px < xres; px++) {
        fiter = (double)(max * px) / (double) xres;
        bpos = px;

        for (int py = 0; py < yres; py++) {
            buf[bpos] = fiter;
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
    escape = 256.0;
    nfac = log(2.0);
    nthread = 1;
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
    int i, m, n, nc, argn, scale;
    unsigned int r, g, b;
    double cx = 0.0, cy = 0.0;

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
         palette(scale, shift, {index, color}...) - palette
         where:   colors   = RGB (24 bit hexadecimal) values
         result:  none
         *******************************************************************************/
        case 1:
            if (argn > 1) {
                sscanf(args[1], "%d", &scale);
            }
            if (argn > 2) {
                sscanf(args[2], "%d", &n);
                shift = scale * n;
            }
            if (argn > 3) {
                nindex = (argn - 3) / 2 ;
                if (nindex >= MAX_INDICES) {
                    fprintf(stderr,
                            "main - number of palette indices must be less than %d\n",
                            MAX_INDICES);
                    return (0);
                }
                for (i = 0; i < nindex; ++i) {
                    if (sscanf(args[i*2 + 3], "%d", &n) == 1) indices[i] = scale * n;
                    if (sscanf(args[i*2 + 4], "%2x%2x%2x", &r, &g, &b) == 3) {
                        colors[0][i] = r;
                        colors[1][i] = g;
                        colors[2][i] = b;
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
         fractal(type,[cx,cy,]ctype) - generate fractal (julia if c0 = cx + cy I defined)
         where:   type  = 0 (MANDEL)
                  cx    = constant x coordinate
                  cy    = constant y coordinate
                  ctype = escape type (0 = re(z^2) + im(r^2), 1 = re(z^2), 2 = im(z^2))
         result:  none
         *******************************************************************************/
        case 3:
            if (argn > 1)
                sscanf(args[1], "%d", &type);
            if (argn > 3) {
                sscanf(args[2], "%lg", &cx);
                sscanf(args[3], "%lg", &cy);
                c0 = cx + cy * I;
                julia = TRUE;
            } else {
            	julia = FALSE;
            }
            if (argn == 3 || argn == 5) {
                sscanf(args[argn-1], "%d", &ctype);
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
    pthread_exit(NULL);
    return rc;
}
