/*
 * fractal.c
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include "fractal.h"
#include <X11/Xlib.h>
#include <rexxsaa.h>
#include <string.h>

char hostname[] = "FRACTAL";

double xc, yc, size, pxc, pyc, psize;
double escape, nfac;
double complex c0;
UINT type = MANDEL, ctype = 0;
int julia = FALSE, pipe = FALSE, maxIter = 1000, nthread = 1, is = 0;
int *image;
long xres, yres;
float *buf, *pbuf;
struct FDATA saveData[MAX_THREAD];
pthread_t thr[MAX_THREAD];

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
    if ((pbuf = calloc((long) buflen, sizeof(float))) == NULL) {
        fprintf(stderr, "main - insufficient memory!!!\n");
        return (0);
    }
    return (1);
}

void close_buffer() {
    free(buf);
    free(pbuf);
}

UINT interpolate_color(UINT comp1, UINT comp2, float rf) {
    UBYTE r1 = (UBYTE)((comp1 >> 16) & 0xff);
    UBYTE r2 = (UBYTE)((comp2 >> 16) & 0xff);
    UBYTE g1 = (UBYTE)((comp1 >> 8) & 0xff);
    UBYTE g2 = (UBYTE)((comp2 >> 8) & 0xff);
    UBYTE b1 = (UBYTE)(comp1 & 0xff);
    UBYTE b2 = (UBYTE)(comp2 & 0xff);
    return (UINT) ((r2 - r1) * rf + r1) << 16 |
            (UINT) ((g2 - g1) * rf + g1) << 8 |
            (UINT) ((b2 - b1) * rf + b1);
}

UINT get_color(float fiter, int nindex, int shift, int indices[MAX_INDICES], UINT comps[MAX_INDICES]) {
    UINT color;

    if (fiter < 0.0) {
        color = 0;
    } else {
        int nlast = nindex - 1;
        float riter = fmodf(fiter + (float)shift, (float)indices[nlast]);
        float rf;
        int ni;
        UINT comp1, comp2;

        for (ni = 0; ni <= nlast; ni++) {
            if (indices[ni] > riter) break;
        }
        // if in the first interval or after the last then interpolate between last and first colors
        if (ni == 0 || ni > nlast) {
            if (indices[0] > 0) {
                rf = riter / (float) indices[0];
                comp1 = comps[nlast];
                comp2 = comps[0];
                color = interpolate_color(comp1, comp2, rf);
            } else {
                color = comps[0];
            }
        }
        // otherwise interpolate between colors at the start and end of the interval
        else {
            if (indices[ni] > indices[ni-1]) {
                rf = (riter - (float)indices[ni-1]) / (float) (indices[ni] - indices[ni-1]);
                comp1 = comps[ni-1];
                comp2 = comps[ni];
                color = interpolate_color(comp1, comp2, rf);
            } else {
                color = comps[ni];
            }
        }
    }
    return color;
}

void save_ppm(void *data) {
    FILE *ifp;
    struct FDATA *threadData;
    UINT color;

    threadData = (struct FDATA *) data;

    if ((ifp = open_file(threadData->file, "w")) == NULL) {
        fprintf(stderr, "save - could not open file!!!\n");
    } else {
        long i, j, pos = 0;
        float fiter;

        fprintf(ifp, "P6\n%ld %ld\n255\n", xres, yres);
        for (j = 0; j < yres; j++) {
            for (i = 0; i < xres; i++) {
                fiter = buf[pos++];
                color = get_color(fiter, threadData->nindex, threadData->shift, threadData->indices, threadData->comps);
                putc((UBYTE)((color >> 16) & 0x0000ff) , ifp);
                putc((UBYTE)((color >> 8) & 0x0000ff) , ifp);
                putc((UBYTE)(color & 0x0000ff) , ifp);
            }
        }
        fflush(ifp);
        close_file(ifp);
    }
}

void* _save_ppm(void *data) {
    save_ppm(data);
    pthread_exit(NULL);
}

int save(int it) {
    int rc;

    if ((rc = pthread_create(&thr[it], NULL, _save_ppm, &saveData[it]))) {
        fprintf(stderr, "main - error: pthread_create, rc: %d\n",
                rc);
        return 0;
    }
    return 1;
}

void set_rexx_var(char* name, char* value)
{
    SHVBLOCK rexxvar;
    RXSTRING rexxName, rexxValue;

    rexxValue.strptr = value;
    rexxValue.strlength = rexxvar.shvvaluelen = strlen(rexxValue.strptr);
    rexxName.strptr = name;
    rexxName.strlength = rexxvar.shvnamelen = strlen(rexxName.strptr);
    rexxvar.shvnext = NULL;
    rexxvar.shvname = rexxName;
    rexxvar.shvvalue = rexxValue;
    rexxvar.shvcode = RXSHV_SET;
    RexxVariablePool(&rexxvar);
}

void set_rexx_int(char* name, int value)
{
    char strvalue[10];

    sprintf(strvalue, "%d", value);
    set_rexx_var(name, strvalue);
}

void set_rexx_double(char* name, double value)
{
    char strvalue[30];

    sprintf(strvalue, "%.16e", value);
    set_rexx_var(name, strvalue);
}

int process_event(Display *display, Window window, XImage *ximage)
{
    double dx, dy, x, y;

    if (xres > yres) {
        dx = size * (double) xres / yres;
        dy = size;
    } else {
        dx = size;
        dy = size * (double) yres / xres;
    }

    Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
    XEvent ev;

    XSetWMProtocols(display, window, &wmDeleteWindow, 1);
    XNextEvent(display, &ev);

    switch(ev.type)
    {
        case Expose:
            XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, xres, yres);
            break;
        case ButtonPress:
            x = xc + dx * (((double) ev.xbutton.x / xres) - 0.5);
            y = yc + dy * (((double) ev.xbutton.y / yres) - 0.5);
            set_rexx_double("DISPLAY.X", x);
            set_rexx_double("DISPLAY.Y", y);
            set_rexx_int("DISPLAY.BUTTON", ev.xbutton.button);
            return FALSE;
        case ClientMessage:
            // handle close window gracefully
            if (ev.xclient.data.l[0] == wmDeleteWindow) {
                set_rexx_double("DISPLAY.X", xc);
                set_rexx_double("DISPLAY.Y", yc);
                set_rexx_int("DISPLAY.BUTTON", -1);
                XCloseDisplay(display);
                return FALSE;
            }
            break;
    }
    return TRUE;
}

Display *display = NULL;

void init_display() {
    if (display == NULL) {
        display = XOpenDisplay(NULL);
    }
}

void display_image(struct FDATA *data) {
    int open = TRUE;
    long i, j, pos = 0;
    float fiter;
    UINT color;

    long buflen = (long) xres * yres;

	if ((image = calloc((long) buflen, sizeof(int))) == NULL) {
		fprintf(stderr, "main - insufficient memory!!!\n");
	}

    pos = 0;

    for (j = 0; j < yres; j++) {
        for (i = 0; i < xres; i++) {
            fiter = buf[pos++];
            color = get_color(fiter, data->nindex, data->shift, data->indices, data->comps);
            image[pos] = color;
        }
    }

    init_display();

    Visual *visual = DefaultVisual(display, 0);
    XImage *ximage = XCreateImage(display, visual, DefaultDepth(display,DefaultScreen(display)), ZPixmap, 0, (char *)image, xres, yres, 32, 0);
    Window window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, xres, yres, 1, 0, 0);

    XSelectInput(display, window, ButtonPressMask|ExposureMask);
    XMapWindow(display, window);

    while (open) {
        open = process_event(display, window, ximage);
    }

    XDestroyWindow(display, window);
    free(image);
}

void wait() {
    if (is == 0) return;
    for (int i = 0; i < is; ++i) {
        pthread_join(thr[i], NULL);
    }
    is = 0;
}

double crad2(double complex z) {
    double x, y;
    if (ctype == 0) {
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
    if (iter < maxIter) {
        fiter = (float) iter;
        if (!julia && ctype == 0) {
            fiter += (float)(1.0 - (double)(log((double)(log(r) / 2.0) / nfac) / nfac));
        }
        return fiter;
    }
    return -1.0;
}

/*
 * Get interpolated position.
 *
 * - cr Current resolution
 * - cc Current coordinate
 * - pc Prior center
 * - pw Prior distance
 */
int interpolated_position(long cr, double cc, double pc, double pd) {
    return (double) cr * (((cc - pc) / pd) + 0.5);
}

int sample_pbuf(int dx, int pos) {
    if (pos < 0) return FALSE;
    if (dx == 1) return (pbuf[pos] < 0.0);
    if (dx > 0) {
        int flag = TRUE;
        int dy = dx * xres;
        for (int py = 0; py <= dy; py += xres) {
            if (!(pbuf[pos + py] < 0.0))
                flag = FALSE;
            for (int px = 1; px <= dx; px++) {
                if (!(pbuf[pos + py + px] < 0.0))
                    flag = FALSE;
                if (!(pbuf[pos + py - px] < 0.0))
                    flag = FALSE;
                if (!(pbuf[pos - py + px] < 0.0))
                    flag = FALSE;
                if (!(pbuf[pos - py - px] < 0.0))
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
    double dx, dy, pdx, pdy,  x, y;
    double complex z;
    int pos, ppos, ppx, ppy, dp;

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
        ppy = interpolated_position(yres, y, pyc, pdy);
        for (int px = x1; px < xres; px += nthread) {
            x = xc + dx * (((double) px / xres) - 0.5);
            ppx = interpolated_position(xres, x, pxc, pdx);
            pos = xres * py + px;
            ppos = get_pos(ppx, ppy, 0);
            if (sample_pbuf(dp, ppos)) {
                buf[pos] = -1.0;
                continue;
            }
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
    memcpy(pbuf, buf, xres * yres * sizeof(float));
    return 1;
}

int spectrum() {
    int bpos;
    float fiter;

    for (int px = 0; px < xres; px++) {
        fiter = (float)px;
        bpos = px;

        for (int py = 0; py < yres; py++) {
            buf[bpos] = fiter;
            bpos += xres;
        }
    }
    return 0;
}

void init_data(struct FDATA *data) {
    data->comps[0] = 0x000000;
    data->comps[1] = 0xffffff;
    data->comps[2] = 0x000000;
    data->indices[0] = 0;
    data->indices[1] = 50;
    data->indices[2] = 100;
    data->nindex = 3;
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
    type = MANDEL;
    ctype = 0;
}

#define MAXARG 80

APIRET APIENTRY handler(PRXSTRING command, PUSHORT flags,
        PRXSTRING returnstring) {
    char *args[MAXARG], *args0;
    char *icom, *ocom;
    char commands[6][10] = { "reset", "view", "fractal", "spectrum", "save", "display" };
    int ncom = 6;
    int i, m, n, nc, argn, scale;
    double cx = 0.0, cy = 0.0;
    UINT color;

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
            wait();
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
         view(xc,yc,size) - view
         where:   xc    = center x coordinate
                  yc    = center y coordinate
                  size  = size
         result:  none
         *******************************************************************************/
        case 1:
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
        case 2:
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
            wait();
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
        case 3:
            if (!open_buffer()) {
                close_buffer();
                fprintf(stderr,
                        "main - could not allocate memory for bitmap\n");
                return (0);
            }
            spectrum();
            break;

        /*******************************************************************************
         save(filename, scale, shift, {index, color}...) - save image (see note 1)
         where:   filename = the name of the file to store image
                  scale    = the color spectrum scale factor (see note 2)
                  shift    = the color spectrum shift (scaled)
                  index    = a color index value (scaled)
                  color    = a color value
         result:  none
         notes :  (1) if no arguments provided then current running save threads will be
                  joined - i.e., waits for current threads to complete
                  (2) color spectrum will be stretched by multplying shift and indices
                  by the amount specified by scale
         *******************************************************************************/
        case 4:
            init_data(&saveData[is]);
            if (argn > 2) {
                sscanf(args[2], "%d", &scale);
            }
            if (argn > 3) {
                sscanf(args[3], "%d", &n);
                saveData[is].shift = scale * n;
            }
            if (argn > 4) {
                saveData[is].nindex = (argn - 4) / 2 ;
                if (saveData[is].nindex >= MAX_INDICES) {
                    fprintf(stderr,
                            "main - number of palette indices must be less than %d\n",
                            MAX_INDICES);
                    return (0);
                }
                for (i = 0; i < saveData[is].nindex; ++i) {
                    if (sscanf(args[i*2 + 4], "%d", &n) == 1) saveData[is].indices[i] = scale * n;
                    if (sscanf(args[i*2 + 5], "%6x", &color) == 1) {
                        saveData[is].comps[i] = color;
                    }
                }
            }
            if (argn > 1) {
                strcpy(saveData[is].file, args[1]);
                save(is++);
            } else {
                wait();
            }
            break;

        /*******************************************************************************
         display(scale, shift, {index, color}...) - display image
         where:   scale    = the color spectrum scale factor (see note 1)
                  shift    = the color spectrum shift (scaled)
                  index    = a color index value (scaled)
                  color    = a color value
         result:  none
         notes :  (1) color spectrum will be stretched by multplying shift and indices
                  by the amount specified by scale
         *******************************************************************************/
        case 5:
            init_data(&saveData[0]);
            if (argn > 1) {
                sscanf(args[1], "%d", &scale);
            }
            if (argn > 2) {
                sscanf(args[2], "%d", &n);
                saveData[0].shift = scale * n;
            }
            if (argn > 3) {
                saveData[0].nindex = (argn - 3) / 2 ;
                if (saveData[0].nindex >= MAX_INDICES) {
                    fprintf(stderr,
                            "main - number of palette indices must be less than %d\n",
                            MAX_INDICES);
                    return (0);
                }
                for (i = 0; i < saveData[0].nindex; ++i) {
                    if (sscanf(args[i*2 + 3], "%d", &n) == 1) saveData[0].indices[i] = scale * n;
                    if (sscanf(args[i*2 + 4], "%6x", &color) == 1) {
                        saveData[0].comps[i] = color;
                    }
                }
            }
            display_image(&saveData[0]);
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
    RXSTRING result;

    if (argc > 1) {
        RXSTRING rargv;
        if (argc > 2) {
            MAKERXSTRING(rargv, argv[2], strlen(argv[2]));
        }
        reset();
        RexxRegisterSubcomExe(hostname, (PFN) handler, NULL);
        RexxRegisterFunctionExe("sqrt", (PFN) sqrtHandler);
        RexxRegisterFunctionExe("pow", (PFN) powHandler);

        result.strlength = 200;
        result.strptr = malloc(200);

        rc = RexxStart(1, &rargv, argv[1], 0, hostname, RXCOMMAND, NULL,
                &returnCode, &result);
        if (rc < 0)
            rc = -rc;
    }
    pthread_exit(NULL);
    return rc;
}
