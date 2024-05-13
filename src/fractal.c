/*
 * fractal.c
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include "fractal.h"
#include "iff.h"
#include <X11/Xlib.h>
#include <rexxsaa.h>
#include <string.h>

char hostname[] = "FRACTAL";

double escape, nfac;
unsigned int type = MANDEL, ctype = 0;
int julia = FALSE, maxIter = 1000, nthread = 1, is = 0;
int *image;
float *pbuf;
struct FractalData pdata;
pthread_t thr[MAX_THREAD];

extern float *buf;
extern UBYTE color[3];
extern long open_buf();
extern void close_buf();
extern struct ViewData viewData;
extern struct FractalData data;
extern struct ColorData colorData;

extern UBYTE *get_color(float fiter, int nindex, int shift,
                        int indices[MAX_INDICES], UBYTE comps[3][MAX_INDICES]);
extern int save_iff(char *file);

extern int parse(char *input, char *args[], int narg);

int open_buffer() {
  long buflen;

  buflen = open_buf();

  if (buflen == 0) {
    return (0);
  }
  if ((pbuf = calloc(buflen, sizeof(float))) == NULL) {
    fprintf(stderr, "main - insufficient memory!!!\n");
    return (0);
  }
  return (1);
}

void close_buffer() {
  close_buf();
  free(pbuf);
}

void set_rexx_var(char *name, char *value) {
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

void set_rexx_int(char *name, int value) {
  char strvalue[10];

  sprintf(strvalue, "%d", value);
  set_rexx_var(name, strvalue);
}

void set_rexx_double(char *name, double value) {
  char strvalue[30];

  sprintf(strvalue, "%.16e", value);
  set_rexx_var(name, strvalue);
}

int process_event(Display *display, Window window, XImage *ximage) {
  double dx, dy, x, y;

  Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
  XEvent ev;

  XSetWMProtocols(display, window, &wmDeleteWindow, 1);
  XNextEvent(display, &ev);

  switch (ev.type) {
  case Expose:
    XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0,
              viewData.xres, viewData.yres);
    break;
  case ButtonPress:
    if (viewData.xres > viewData.yres) {
      dx = data.size * (double)viewData.xres / viewData.yres;
      dy = data.size;
    } else {
      dx = data.size;
      dy = data.size * (double)viewData.yres / viewData.xres;
    }
    x = data.xc + dx * (((double)ev.xbutton.x / viewData.xres) - 0.5);
    y = data.yc + dy * (((double)ev.xbutton.y / viewData.yres) - 0.5);
    set_rexx_double("DISPLAY.X", x);
    set_rexx_double("DISPLAY.Y", y);
    set_rexx_int("DISPLAY.BUTTON", ev.xbutton.button);
    return FALSE;
  case ClientMessage:
    // handle close window gracefully
    if (ev.xclient.data.l[0] == wmDeleteWindow) {
      set_rexx_double("DISPLAY.X", data.xc);
      set_rexx_double("DISPLAY.Y", data.yc);
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

void display_image() {
  int open = TRUE;
  long i, j, pos = 0;
  float fiter;
  UBYTE *c;

  long buflen = (long)viewData.xres * viewData.yres;

  if ((image = calloc((long)buflen, sizeof(int))) == NULL) {
    fprintf(stderr, "main - insufficient memory!!!\n");
  }

  pos = 0;

  for (j = 0; j < viewData.yres; j++) {
    for (i = 0; i < viewData.xres; i++) {
      fiter = buf[pos++];
      c = get_color(fiter, colorData.nindex, colorData.shift, colorData.indices,
                    colorData.comps);
      image[pos] = c[0] << 16 | c[1] << 8 | c[2];
    }
  }

  init_display();

  Visual *visual = DefaultVisual(display, 0);
  XImage *ximage = XCreateImage(
      display, visual, DefaultDepth(display, DefaultScreen(display)), ZPixmap,
      0, (char *)image, viewData.xres, viewData.yres, 32, 0);
  Window window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0,
                                      viewData.xres, viewData.yres, 1, 0, 0);

  XSelectInput(display, window, ButtonPressMask | ExposureMask);
  XMapWindow(display, window);

  while (open) {
    open = process_event(display, window, ximage);
  }

  XDestroyWindow(display, window);
  free(image);
}

void wait() {
  if (is == 0)
    return;
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
    fiter = (float)iter;
    if (!julia && ctype == 0) {
      fiter +=
          (float)(1.0 - (double)(log((double)(log(r) / 2.0) / nfac) / nfac));
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
  return (double)cr * (((cc - pc) / pd) + 0.5);
}

int sample_pbuf(int dx, int pos) {
  if (pos < 0)
    return FALSE;
  if (pbuf[pos] == 0.0)
    return FALSE;
  if (dx == 1)
    return (pbuf[pos] < 0.0);
  if (dx > 0) {
    int flag = TRUE;
    int dy = dx * viewData.xres;
    for (int py = 0; py <= dy; py += viewData.xres) {
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
  if ((x < d) || (x > viewData.xres - d))
    return -1;
  if ((y < d) || (y > viewData.yres - d))
    return -1;
  return viewData.xres * y + x;
}

void generate_fractal(int x1) {
  double dx, dy, pdx, pdy, x, y;
  double complex z;
  int pos, ppos, ppx, ppy, dp;

  if (viewData.xres > viewData.yres) {
    dx = data.size * (double)viewData.xres / viewData.yres;
    dy = data.size;
    if (pdata.size >= data.size) {
      dp = (0.9999 + pdata.size / data.size);
      pdx = pdata.size * (double)viewData.xres / viewData.yres;
      pdy = pdata.size;
    } else {
      dp = 0;
      pdx = dx;
      pdy = dy;
    }
  } else {
    dx = data.size;
    dy = data.size * (double)viewData.yres / viewData.xres;
    if (pdata.size >= data.size) {
      dp = (0.9999 + pdata.size / data.size);
      pdx = pdata.size;
      pdy = pdata.size * (double)viewData.yres / viewData.xres;
    } else {
      dp = 0;
      pdx = dx;
      pdy = dy;
    }
  }

  for (int py = 0; py < viewData.yres; py++) {
    y = data.yc + dy * (((double)py / viewData.yres) - 0.5);
    ppy = interpolated_position(viewData.yres, y, pdata.yc, pdy);
    for (int px = x1; px < viewData.xres; px += nthread) {
      x = data.xc + dx * (((double)px / viewData.xres) - 0.5);
      ppx = interpolated_position(viewData.xres, x, pdata.xc, pdx);
      pos = viewData.xres * py + px;
      ppos = get_pos(ppx, ppy, 0);
      if (sample_pbuf(dp, ppos)) {
        buf[pos] = -1.0;
        continue;
      }
      z = x + y * I;

      if (julia)
        buf[pos] = iterate(z, data.x0 + data.y0 * I);
      else
        buf[pos] = iterate(0.0, z);
    }
  }
}

void *_generate_fractal(void *arg) {
  int *px1 = (int *)arg;

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
      fprintf(stderr, "main - error: pthread_create, rc: %d\n", rc);
      return 0;
    }
  }

  for (it = 0; it < nthread; ++it) {
    pthread_join(thr[it], NULL);
  }
  memcpy(pbuf, buf, viewData.xres * viewData.yres * sizeof(float));
  return 1;
}

void init_color_data() {
  colorData.comps[0][0] = 0x00;
  colorData.comps[0][1] = 0xff;
  colorData.comps[0][2] = 0x00;
  colorData.comps[1][0] = 0x00;
  colorData.comps[1][1] = 0xff;
  colorData.comps[1][2] = 0x00;
  colorData.comps[2][0] = 0x00;
  colorData.comps[2][1] = 0xff;
  colorData.comps[2][2] = 0x00;
  colorData.indices[0] = 0;
  colorData.indices[1] = 50;
  colorData.indices[2] = 100;
  colorData.nindex = 3;
}

void reset() {
  viewData.xres = 320L;
  viewData.yres = 200L;
  data.xc = pdata.xc = -0.75;
  data.yc = pdata.yc = 0.0;
  data.x0 = data.y0 = 0.0;
  data.size = 2.5;
  pdata.size = 0.0;
  escape = 256.0;
  nfac = log(2.0);
  nthread = 1;
  type = MANDEL;
  ctype = 0;
  init_color_data();
}

#define MAXARG 80

APIRET APIENTRY handler(PRXSTRING command, PUSHORT flags,
                        PRXSTRING returnstring) {
  char *args[MAXARG], *args0;
  char *icom, *ocom;
  char commands[7][10] = {"reset", "view", "fractal", "read",
                          "save", "display", "spectrum"};
  char file[2048];
  int ncom = 7;
  int i, m, n, nc, argn, scale;
  unsigned int r, g, b;

  if (command->strptr != NULL) {
    for (i = 0; i < MAXARG; i++)
      args[i] = NULL;

    args0 = (char *)malloc((long)(command->strlength + 1));
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
        sscanf(args[1], "%ld", &viewData.xres);
      if (argn > 2)
        sscanf(args[2], "%ld", &viewData.yres);
      if (argn > 3)
        sscanf(args[3], "%d", &maxIter);
      if (argn > 4)
        sscanf(args[4], "%d", &nthread);
      if (!open_buffer()) {
        fprintf(stderr, "main - could not allocate memory for bitmap\n");
        return 0;
      }

      if (nthread > MAX_THREAD) {
        fprintf(stderr, "main - number of threads must be no greater than %d\n",
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
      pdata.xc = data.xc;
      pdata.yc = data.yc;
      pdata.size = data.size;
      if (argn > 1)
        sscanf(args[1], "%lg", &data.xc);
      if (argn > 2)
        sscanf(args[2], "%lg", &data.yc);
      if (argn > 3)
        sscanf(args[3], "%lg", &data.size);
      break;

    /*******************************************************************************
     fractal(type,[cx,cy,]ctype) - generate fractal (julia if c0 = cx + cy I
     defined) where:   type  = 0 (MANDEL) cx    = constant x coordinate cy    =
     constant y coordinate ctype = escape type (0 = re(z^2) + im(r^2), 1 =
     re(z^2), 2 = im(z^2)) result:  none
     *******************************************************************************/
    case 2:
      if (argn > 1)
        sscanf(args[1], "%d", &type);
      if (argn > 3) {
        sscanf(args[2], "%lg", &data.x0);
        sscanf(args[3], "%lg", &data.y0);
        julia = TRUE;
      } else {
        julia = FALSE;
      }
      if (argn == 3 || argn == 5) {
        sscanf(args[argn - 1], "%d", &ctype);
      }
      if (type != MANDEL) {
        fprintf(stderr, "main - type not supported: %d\n", type);
        return (0);
      }
      type = MANDEL;
      wait();
      if (!fractal()) {
        return (0);
      }
      pdata.xc = data.xc;
      pdata.yc = data.yc;
      break;

    /*******************************************************************************
     read(filename) - read image
     where:   filename = the name of the file to store image
     result:  none
     *******************************************************************************/
    case 3:
      break;

    /*******************************************************************************
     save(filename) - save image
     where:   filename = the name of the file to store image
     result:  none
     *******************************************************************************/
    case 4:
      if (argn > 1) {
        strcpy(file, args[1]);
        save_iff(file);
      }
      break;

    /*******************************************************************************
     display() - display image
     result:  none
     *******************************************************************************/
    case 5:
      display_image();
      break;

    /*******************************************************************************
     spectrum(scale, shift, {index, color}...) - set spectrum
     where:   scale    = the color spectrum scale factor (see note 2)
              shift    = the color spectrum shift (scaled)
              index    = a color index value (scaled)
              color    = a color value
     result:  none
     notes :  (1) color spectrum will be stretched by multplying shift and
     indices by the amount specified by scale
     *******************************************************************************/
    case 6:
      init_color_data();
      if (argn > 1) {
        sscanf(args[1], "%d", &scale);
      }
      if (argn > 2) {
        sscanf(args[2], "%d", &n);
        colorData.shift = scale * n;
      }
      if (argn > 3) {
        colorData.nindex = (argn - 3) / 2;
        if (colorData.nindex >= MAX_INDICES) {
          fprintf(stderr,
                  "main - number of palette indices must be less than %d\n",
                  MAX_INDICES);
          return (0);
        }
        for (i = 0; i < colorData.nindex; ++i) {
          if (sscanf(args[i * 2 + 3], "%d", &n) == 1)
            colorData.indices[i] = scale * n;
          if (sscanf(args[i * 2 + 4], "%2x%2x%2x", &r, &g, &b) == 3) {
            colorData.comps[0][i] = r;
            colorData.comps[1][i] = g;
            colorData.comps[2][i] = b;
          }
        }
      }
      break;

    case -1:
      fprintf(stderr, "main - ambiguous command\n");
      break;

    default:
      fprintf(stderr, "main - unknown command\n");
      break;
    }

    returnstring->strptr = NULL;
    returnstring->strlength = 0;
    if (args0)
      free(args0);
  }
  return 0;
}

APIRET APIENTRY sqrtHandler(PSZ name, ULONG argc, PRXSTRING argv, PSZ queuename,
                            PRXSTRING returnstring) {
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

APIRET APIENTRY powHandler(PSZ name, ULONG argc, PRXSTRING argv, PSZ queuename,
                           PRXSTRING returnstring) {
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
    RexxRegisterSubcomExe(hostname, (PFN)handler, NULL);
    RexxRegisterFunctionExe("sqrt", (PFN)sqrtHandler);
    RexxRegisterFunctionExe("pow", (PFN)powHandler);

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
