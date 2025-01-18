#include "fractal.h"
#include "iff.h"
#include <X11/X.h>
#include <X11/Xlib.h>

extern float *buf;
extern int read_iff(char *file);
extern int save_iff(char *file);
extern void close_buf();
extern void palette(void (*update_image)());
extern struct ViewData viewData;
extern UBYTE *get_color(float fiter);

char *file;
int *image = NULL, zoom = 1;
long xres = 1200, yres = 1200;
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

int get_zoom_color(long x, long y) {
  int n = 0;
  long i, j, pos;
  float fiter = 0.0, titer = 0.0;
  UBYTE *c;

  pos = (viewData.xres * y + x) * zoom;
  for (i = 0; i < zoom; i++) {
    for (j = 0; j < zoom; j++) {
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
      image[pos] = get_zoom_color(x, y);
    }
  }

  XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, xres,
            yres);
}

void init_display() {
  if (display == NULL) {
    xres = viewData.xres / zoom;
    yres = viewData.yres / zoom;

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

  XSetWMProtocols(display, window, &wmDeleteWindow, 1);
  XNextEvent(display, &ev);

  switch (ev.type) {
  case Expose:
    XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, xres,
              yres);
    break;
  case ButtonPress:
    if (ev.xbutton.button == 4) {
      zoom++;
    } else if (ev.xbutton.button == 5) {
      if (zoom > 1)
        zoom--;
    } else {
      close_buf();
      teardown();
      return FALSE;
    }
    printf("%d\n", zoom);
    teardown();
    break;
  case KeyPress:
    switch (ev.xkey.keycode) {
    case 9: // Esc
      close_buf();
      teardown();
      return FALSE;
    case 33: // p - palette
      palette(update_image);
      break;
    case 27: // r - reset
      break;
    case 39: // s - save
      save_iff(file);
      break;
    }
    printf("key code %d\n", ev.xkey.keycode);
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

  if (!read_iff(file)) {
    free(buf);
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }

  while (open) {
    if (display == NULL) {
      init_display();
      update_image();
    }
    open = process_event();
  }
}
