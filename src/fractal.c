/*
 * fractal.c
 *
 *  Created on: Jan 8, 2022
 *      Author: russ
 */

#include "fractal.h"
#include "fff.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <float.h>
#include <math.h>
#include <rexxsaa.h>
#include <string.h>

char hostname[] = "FRACTAL";

unsigned int type = MANDEL, ctype = 0;
int julia = FALSE, maxIter = 1000, nthread = 1;
int *image = NULL;
Display *display = NULL;
XImage *ximage;
Window window;

extern float *buf;
extern int open_buf();
extern void close_buf();
extern void clear_buf();
extern struct FractalData data;
extern struct ViewData viewData;
extern struct ColorData colorData;

extern UBYTE *get_color(float fiter);
extern int read_fff(char *file);
extern int save_fff(char *file);
extern int parse(char *input, char *args[], int narg);
extern void wait();
extern int fractal(int arb);

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

void init_display() {
  if (display == NULL) {
    long buflen = (long)viewData.xres * viewData.yres;

    if ((image = calloc((long)buflen, sizeof(int))) == NULL) {
      fprintf(stderr, "main - insufficient memory!!!\n");
    }

    display = XOpenDisplay(NULL);
    Visual *visual = DefaultVisual(display, 0);
    ximage = XCreateImage(
        display, visual, DefaultDepth(display, DefaultScreen(display)), ZPixmap,
        0, (char *)image, viewData.xres, viewData.yres, 32, 0);
    window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0,
                                 viewData.xres, viewData.yres, 1, 0, 0);

    XSelectInput(display, window, ButtonPressMask | ExposureMask);
    XMapWindow(display, window);
  }
}

void teardown() {
  close_buf();
  if (display != NULL) {
    XDestroyWindow(display, window);
    display = NULL;
  }
  if (image != NULL) {
    free(image);
  }
}

int process_event() {
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
    set_rexx_int("DISPLAY.X", ev.xbutton.x);
    set_rexx_int("DISPLAY.Y", ev.xbutton.y);
    set_rexx_int("DISPLAY.BUTTON", ev.xbutton.button);
    return FALSE;
  case ClientMessage:
    // handle close window gracefully
    if (ev.xclient.data.l[0] == wmDeleteWindow) {
      set_rexx_int("DISPLAY.BUTTON", -1);
      teardown();
      return FALSE;
    }
    break;
  }
  return TRUE;
}

void check_buttonpress() {
  Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
  XEvent ev;

  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  while (XCheckTypedEvent(display, ButtonPress, &ev)) {
    set_rexx_int("DISPLAY.X", ev.xbutton.x);
    set_rexx_int("DISPLAY.Y", ev.xbutton.y);
    set_rexx_int("DISPLAY.BUTTON", ev.xbutton.button);
  }
  if (XCheckTypedEvent(display, ClientMessage, &ev)) {
    // handle close window gracefully
    if (ev.xclient.data.l[0] == wmDeleteWindow) {
      set_rexx_int("DISPLAY.BUTTON", -1);
      teardown();
    }
  }
}

void update_image() {
  int open = TRUE, color, grid;
  long i, j, x, y, pos;
  float fiter;
  UBYTE *c;

  grid = 1 << viewData.scale;
  for (y = 0; y < viewData.yres; y += grid) {
    for (x = 0; x < viewData.xres; x += grid) {
      pos = viewData.xres * y + x;
      fiter = buf[pos];
      c = get_color(fiter);
      color = c[0] << 16 | c[1] << 8 | c[2];
      if (grid == 1)
        image[pos] = color;
      else {
        for (i = 0; i < grid; i++) {
          for (j = 0; j < grid; j++) {
            image[pos + (viewData.xres * i) + j] = color;
          }
        }
      }
    }
  }

  XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0,
            viewData.xres, viewData.yres);

  if (grid == 1) {
    while (open) {
      open = process_event();
    }
  } else {
    check_buttonpress();
  }
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
  viewData.scale = 0;
  strcpy(data.xc, "-0.75");
  strcpy(data.yc, "0.0");
  strcpy(data.x0, "0.0");
  strcpy(data.y0, "0.0");
  strcpy(data.size, "2.5");
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
  char commands[7][10] = {"reset", "view",    "fractal", "read",
                          "save",  "display", "spectrum"};
  char file[2048];
  int ncom = 7;
  int i, m, n, nc, argn, scale, arb = 0;
  double size;
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
     *******************************************************************************/
    case 0:
      n = 0;
      wait();
      reset();
      if (argn > 1)
        sscanf(args[1], "%ld", &viewData.xres);
      if (argn > 2)
        sscanf(args[2], "%ld", &viewData.yres);
      if (argn > 3)
        sscanf(args[3], "%d", &maxIter);
      if (argn > 4)
        sscanf(args[4], "%d", &nthread);
      if (!open_buf()) {
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
     view(xc,yc,size,scale) - view
     where:   xc    = center x coordinate
              yc    = center y coordinate
              size  = size
              scale = scale (grid size = 2 ^ scale) default 0
     *******************************************************************************/
    case 1:
      scale = 0;
      if (argn > 1)
        strcpy(data.xc, args[1]);
      if (argn > 2)
        strcpy(data.yc, args[2]);
      if (argn > 3)
        strcpy(data.size, args[3]);
      if (argn > 4) {
        sscanf(args[4], "%d", &scale);
      }
      if (viewData.scale <= scale) {
        clear_buf();
      }
      viewData.scale = scale;
      break;

    /*******************************************************************************
     fractal(type,[cx,cy,]ctype) - generate fractal (julia if c0 = cx + cy I defined)
     where:   type  = 0 (MANDEL)
              cx    = constant x coordinate
              cy    = constant y coordinate
              ctype = escape type (0 = re(z)^2 + im(z)^2, 1 = re(z^2), 2 = im(z^2))
     *******************************************************************************/
    case 2:
      if (argn > 1)
        sscanf(args[1], "%d", &type);
      if (argn > 3) {
        strcpy(data.x0, args[2]);
        strcpy(data.y0, args[3]);
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
      sscanf(data.size, "%lg", &size);
      arb = size / viewData.xres < DBL_EPSILON;
      if (!fractal(arb)) {
        return (0);
      }
      if (display != NULL) {
        update_image();
      }
      break;

    /*******************************************************************************
     read(filename) - read image
     where:   filename = the name of the file to load image from
     *******************************************************************************/
    case 3:
      if (argn > 1) {
        strcpy(file, args[1]);
        if (!read_fff(file)) {
          fprintf(stderr, "main - could not read file: %s\n", file);
          return 0;
        }
      }
      set_rexx_var("FRACTAL.XC", data.xc);
      set_rexx_var("FRACTAL.YC", data.yc);
      set_rexx_var("FRACTAL.SIZE", data.size);
      set_rexx_int("DISPLAY.XRES", viewData.xres);
      set_rexx_int("DISPLAY.YRES", viewData.yres);
      set_rexx_int("DISPLAY.SCALE", viewData.scale);
      break;

    /*******************************************************************************
     save(filename) - save image
     where:   filename = the name of the file to store image to
     *******************************************************************************/
    case 4:
      if (argn > 1) {
        strcpy(file, args[1]);
        save_fff(file);
      }
      break;

    /*******************************************************************************
     display() - display image
     *******************************************************************************/
    case 5:
      init_display();
      if (display != NULL) {
        update_image();
      }
      break;

    /*******************************************************************************
     spectrum(shift, [index, color]...) - set spectrum
     where:   shift    = the color spectrum shift
              index    = a color index value
              color    = a color value
     *******************************************************************************/
    case 6:
      init_color_data();
      if (argn > 1) {
        sscanf(args[1], "%d", &colorData.shift);
      }
      if (argn > 2) {
        colorData.nindex = (argn - 2) / 2;
        if (colorData.nindex >= MAX_INDICES) {
          fprintf(stderr,
                  "main - number of palette indices must be less than %d\n",
                  MAX_INDICES);
          return (0);
        }
        for (i = 0; i < colorData.nindex; ++i) {
          if (sscanf(args[i * 2 + 2], "%d", &n) == 1)
            colorData.indices[i] = n;
          if (sscanf(args[i * 2 + 3], "%2x%2x%2x", &r, &g, &b) == 3) {
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
    } else {
      MAKERXSTRING(rargv, "", 0);
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
