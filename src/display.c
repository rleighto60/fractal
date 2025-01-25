#include "fractal.h"
#include <X11/X.h>
#include <X11/Xlib.h>

extern float *buf;
extern int read_fff(char *file);
extern int save_fff(char *file);
extern void close_buf();
extern void print_info();
extern void palette(void (*update_image)());
extern struct ViewData viewData;
extern UBYTE *get_color(float fiter);

char *file;
int *image = NULL, scale, max_scale = 1;
long xres = 1200, yres = 1200, xoff = 0, yoff = 0;
Display *display = NULL;
XImage *ximage;
Window window;

void teardown() {
  if (display != NULL) {
    XCloseDisplay(display);
    XDestroyWindow(display, window);
    display = NULL;
  }
  if (image != NULL) {
    free(image);
    image = NULL;
  }
}

// get color at scaled position
int get_scale_color(long x, long y) {
  int n = 0;
  long i, j, sx, sy, pos;
  float fiter = 0.0, titer = 0.0;
  UBYTE *c;

  sx = x + xoff;
  sy = y + yoff;

  if (sx < 0 || sy < 0 || sx >= viewData.xres || sy >= viewData.yres)
    return 0;

  pos = (viewData.xres * sy + sx) * scale;
  for (i = 0; i < scale; i++) {
    for (j = 0; j < scale; j++) {
      fiter = buf[pos + (viewData.xres * i) + j];
      if (fiter >= 0) {
        titer += fiter;
        n++;
      }
    }
  }
  if (n > 0)
    fiter = titer / n;
  c = get_color(fiter);
  return c[0] << 16 | c[1] << 8 | c[2];
}

void update_image() {
  long x, y, pos;

  for (y = 0; y < yres; y++) {
    for (x = 0; x < xres; x++) {
      pos = xres * y + x;
      image[pos] = get_scale_color(x, y);
    }
  }

  XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, xres,
            yres);
}

void init_display() {
  if (display == NULL) {
    max_scale = (int)(0.5 + (double)viewData.yres / (double)yres);
    scale = max_scale;
    xres = viewData.xres / scale;
    yres = viewData.yres / scale;

    long buflen = xres * yres;

    if ((image = calloc((long)buflen, sizeof(int))) == NULL) {
      fprintf(stderr, "main - insufficient memory!!!\n");
    }

    display = XOpenDisplay(NULL);
    Visual *visual = DefaultVisual(display, 0);
    ximage = XCreateImage(display, visual,
                          DefaultDepth(display, DefaultScreen(display)),
                          ZPixmap, 0, (char *)image, xres, yres, 32, 0);
    window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, xres,
                                 yres, 1, 0, 0);

    XSelectInput(display, window,
                 ButtonPressMask | KeyPressMask | ExposureMask);
    XMapWindow(display, window);
  }
}

int process_event() {
  Atom wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", FALSE);
  XEvent ev;
  long sx, sy;

  XSetWMProtocols(display, window, &wmDeleteWindow, 1);
  XNextEvent(display, &ev);

  switch (ev.type) {
  case Expose:
    XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, xres,
              yres);
    break;
  case ButtonPress:
    sx = (long)(ev.xbutton.x + xoff) * scale / max_scale;
    sy = (long)(ev.xbutton.y + yoff) * scale / max_scale;
    if (ev.xbutton.button == 4) {
      // scroll up - decrease scale
      if (scale > 1)
        scale--;
    } else if (ev.xbutton.button == 5) {
      // scroll down - increase scale
      if (scale < max_scale)
        scale++;
    } else {
      // any other button - exit display
      close_buf();
      teardown();
      return FALSE;
    }
    xoff = sx * (max_scale - scale) / scale;
    yoff = sy * (max_scale - scale) / scale;
    break;
  case KeyPress:
    //printf("%d\n", ev.xkey.keycode);
    switch (ev.xkey.keycode) {
    case 9: // esc - exit display
      close_buf();
      teardown();
      return FALSE;
    case 27: // r - reset palette (todo)
      break;
    case 31: // i - info
      print_info();
      break;
    case 33: // p - show palette editor
      palette(update_image);
      break;
    case 39: // s - save with current settings (palette)
      save_fff(file);
      break;
    }
    break;
  case ClientMessage:
    // handle close window gracefully
    if (ev.xclient.data.l[0] == wmDeleteWindow) {
      close_buf();
      teardown();
      return FALSE;
    }
    break;
  }
  return TRUE;
}

int main(int argc, char **argv) {
  int open = TRUE;

  if (argc > 1)
    file = argv[1];
  else
    file = 0;

  if (!read_fff(file)) {
    free(buf);
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }

  init_display();

  while (open) {
    update_image();
    open = process_event();
  }
}
