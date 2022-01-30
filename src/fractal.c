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

    if (!(mask & RED)) {
        if ((rbuf = calloc((long) buflen, 1L)) == NULL) {
            fprintf(stderr, "main - insufficient memory!!!\n");
            return (0);
        }
        mask |= RED;
    }
    if (!(mask & GREEN)) {
        if ((gbuf = calloc((long) buflen, 1L)) == NULL) {
            fprintf(stderr, "main - insufficient memory!!!\n");
            return (0);
        }
        mask |= GREEN;
    }
    if (!(mask & BLUE)) {
        if ((bbuf = calloc((long) buflen, 1L)) == NULL) {
            fprintf(stderr, "main - insufficient memory!!!\n");
            return (0);
        }
        mask |= BLUE;
    }
    return (1);
}

void close_buffer() {
    if (mask & RED) {
        free(rbuf);
        mask ^= RED;
    }
    if (mask & GREEN) {
        free(gbuf);
        mask ^= GREEN;
    }
    if (mask & BLUE) {
        free(bbuf);
        mask ^= BLUE;
    }
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
            putc(rbuf[pos], ifp);
            putc(gbuf[pos], ifp);
            putc(bbuf[pos], ifp);
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

int iterate_mandel(double complex v) {
    int iter = 0;
    double complex z, z2 = 0.0;

    while (creal(z2) <= escape && iter < maxIter) {
        z = z2 + v;
        z2 = z * z;
        iter++;
    }
    return iter;
}

int iterate_julia(double complex z) {
    int iter = 0;
    double complex z2 = z * z;

    while (creal(z2) <= escape && iter < maxIter) {
        z = z2 + c;
        z2 = z * z;
        iter++;
    }
    return iter;
}

int generate_fractal1(int x1) {
    double dx, dy, x, y;
    double complex z;
    int iter, bpos;
    UBYTE rc, gc, bc;

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
            bpos = xres * py + px;
            x = xc + dx * (((double) px / xres) - 0.5);
            z = x + y * I;
            iter = maxIter;

            switch (type) {
            case MANDEL:
                iter = iterate_mandel(z);
                break;
            case JULIA:
                iter = iterate_julia(z);
                break;
            }
            rc = get_color(iter, 0);
            gc = get_color(iter, 1);
            bc = get_color(iter, 2);
            rbuf[bpos] = rc;
            gbuf[bpos] = gc;
            bbuf[bpos] = bc;
        }
    }
    return 0;
}

void* _generate_fractal(void *arg) {
    int *px1 = (int*) arg;

    generate_fractal1(*px1);
    pthread_exit(NULL);
}

int fractal() {
    int it, rc, xs[MAX_THREAD];
    pthread_t thr[MAX_THREAD];

    if (!open_buffer()) {
        close_buffer();
        fprintf(stderr,
                "main - could not allocate memory for bitmap\n");
        return (0);
    }

    if (xres % nthread != 0 || nthread > MAX_THREAD) {
        fprintf(stderr,
                "main - number of threads must be a multiple of xres and no greater than %d\n",
                MAX_THREAD);
        return (0);
    }

    for (it = 0; it < nthread; ++it) {
        xs[it] = it;
        if ((rc = pthread_create(&thr[it], NULL, _generate_fractal, &xs[it]))) {
            fprintf(stderr, "main - error: pthread_create, rc: %d\n",
                    rc);
            return (0);
        }
    }

    for (it = 0; it < nthread; ++it) {
        pthread_join(thr[it], NULL);
    }
    return (1);
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
            rbuf[bpos] = rc;
            gbuf[bpos] = gc;
            bbuf[bpos] = bc;
            bpos += xres;
        }
    }
    return 0;
}

void reset() {
    xres = 320L;
    yres = 200L;
    xc = -0.75;
    yc = 0.0;
    c = 0.0;
    size = 2.5;
    escape = 4.0;
    dmax = 100;
    nthread = 1;
    palette[0][0] = palette[1][0] = palette[2][0] = 0;
    palette[0][1] = palette[1][1] = palette[2][1] = 255;
    ncolor = 2;
    shift = 0;
    type = MANDEL;
}

#define MAXARG 80

APIRET APIENTRY handler(PRXSTRING command, PUSHORT flags,
        PRXSTRING returnstring) {
    char *args[MAXARG], *args0;
    char *icom, *ocom;
    char commands[7][10] = { "reset", "palette", "view", "mandel", "julia", "spectrum", "save" };
    int ncom = 7;
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
            if (argn > 1)
                sscanf(args[1], "%lg", &xc);
            if (argn > 2)
                sscanf(args[2], "%lg", &yc);
            if (argn > 3)
                sscanf(args[3], "%lg", &size);
            break;

        /*******************************************************************************
         mandel - generate mandelbrot
         result:  none
         *******************************************************************************/
        case 3:
            type = MANDEL;
            if (!fractal()) {
                return (0);
            }
            break;

        /*******************************************************************************
         julia(cx,cy) - generate julia
         where:   cx    = constant x coordinate
                  cy    = constant y coordinate
         result:  none
         *******************************************************************************/
        case 4:
            double cx = 0.0, cy = 0.0;

            if (argn > 1)
                sscanf(args[1], "%lg", &cx);
            if (argn > 2)
                sscanf(args[2], "%lg", &cy);
            c = cx + cy * I;
            type = JULIA;
            if (!fractal()) {
                return (0);
            }
            break;

        /*******************************************************************************
         spectrum - view palette color spectrum
         result:  none
         *******************************************************************************/
        case 5:
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
        case 6:
            save_ppm(args[1]);
            break;

        case 7:
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

int main(int argc, char *argv[]) {
    int rc = 0;
    short returnCode;
    RXSTRING Result;

    if (argc > 1) {
        reset();
        rc = RexxRegisterSubcomExe(hostname, (PFN) handler, NULL);

        Result.strlength = 200;
        Result.strptr = malloc(200);

        rc = RexxStart(0, NULL, argv[1], 0, hostname, RXCOMMAND, NULL,
                &returnCode, &Result);
        if (rc < 0)
            rc = -rc;
    }
    return rc;
}
