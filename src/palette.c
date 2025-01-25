#include "fractal.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <math.h>
#include <stdlib.h>

extern struct ColorData colorData;
extern UBYTE *get_color(float fiter);

int *palette_image = NULL;
int color_index = 0, last_x = 0, last_y = 0, palette_y = 0, dx = 0, dy = 0;
long palette_width, palette_height = 50, shift_height = 60, hsl_height = 190,
                    total_height = 200;
double hue = 0.0, saturation = 0.0, intensity = 0.0;
Display *palette_display = NULL;
XImage *palette_ximage;
Window palette_window;

void teardown_palette() {
  if (palette_display != NULL) {
    XCloseDisplay(palette_display);
    XDestroyWindow(palette_display, palette_window);
    palette_display = NULL;
  }
  if (palette_image != NULL) {
    free(palette_image);
    palette_image = NULL;
  }
}

// set current hue, saturation, intensity values from specified rgb value
void rgb2hsl(int rgb) {
  double r = (rgb >> 16) / 255.0;
  double g = ((rgb >> 8) & 0x0000ff) / 255.0;
  double b = (rgb & 0x0000ff) / 255.0;
  double max, min, chroma;

  max = fmax(fmax(r, g), b);
  min = fmin(fmin(r, g), b);
  chroma = max - min;
  intensity = 0.5 * (max + min);
  if (chroma != 0.0) {
    saturation = chroma / (1.0 - fabs(2.0 * intensity - 1.0));
    if (max == r) {
      hue = fmod(((g - b) / chroma), 6.0);
    } else if (max == g) {
      hue = ((b - r) / chroma) + 2.0;
    } else /*if(M==b)*/
    {
      hue = ((r - g) / chroma) + 4.0;
    }
    hue *= 60.0;
    if (hue < 0.0)
      hue += 360.0;
  }
}

// get rgb color from rgb component values
int rgb2color(double r, double g, double b) {
  UBYTE c[3];

  c[0] = (UBYTE)(r * 255.0);
  c[1] = (UBYTE)(g * 255.0);
  c[2] = (UBYTE)(b * 255.0);
  return c[0] << 16 | c[1] << 8 | c[2];
}

// get rgb color from hsl component values
int hsl2rgb(double h, double s, double l) {
  int color = 0;
  double c = 0.0, m = 0.0, x = 0.0;

  c = (1.0 - fabs(2 * l - 1.0)) * s;
  m = 1.0 * (l - 0.5 * c);
  x = c * (1.0 - fabs(fmod(h / 60.0, 2) - 1.0));
  if (h >= 0.0 && h < 60.0) {
    color = rgb2color(c + m, x + m, m);
  } else if (h >= 60.0 && h < 120.0) {
    color = rgb2color(x + m, c + m, m);
  } else if (h >= 120.0 && h < 180.0) {
    color = rgb2color(m, c + m, x + m);
  } else if (h >= 180.0 && h < 240.0) {
    color = rgb2color(m, x + m, c + m);
  } else if (h >= 240.0 && h < 300.0) {
    color = rgb2color(x + m, m, c + m);
  } else if (h >= 300.0 && h < 360.0) {
    color = rgb2color(c + m, m, x + m);
  } else {
    color = rgb2color(m, m, m);
  }
  return color;
}

void init_palette_display() {
  if (palette_display == NULL) {
    int first = colorData.indices[0];
    int last = colorData.indices[colorData.nindex - 1];

    palette_width = first + last;

    long buflen = palette_width * total_height;

    if ((palette_image = calloc((long)buflen, sizeof(int))) == NULL) {
      fprintf(stderr, "main - insufficient memory!!!\n");
    }

    rgb2hsl(colorData.comps[0][color_index] << 16 |
            colorData.comps[1][color_index] << 8 |
            colorData.comps[2][color_index]);

    palette_display = XOpenDisplay(NULL);
    Visual *visual = DefaultVisual(palette_display, 0);
    palette_ximage = XCreateImage(
        palette_display, visual,
        DefaultDepth(palette_display, DefaultScreen(palette_display)), ZPixmap,
        0, (char *)palette_image, palette_width, total_height, 32, 0);
    palette_window =
        XCreateSimpleWindow(palette_display, RootWindow(palette_display, 0), 0,
                            0, palette_width, total_height, 1, 0, 0);

    XSelectInput(palette_display, palette_window,
                 ButtonPressMask | Button1MotionMask | KeyPressMask |
                     ExposureMask);
    XMapWindow(palette_display, palette_window);
  }
}

int find_color_index(int n) {
  int nmin = palette_width, nclosest = -1, nlast = colorData.nindex - 1;
  int dn;

  for (int i = 0; i <= nlast; i++) {
    dn = abs(colorData.indices[i] - n);
    if (dn < nmin) {
      nmin = dn;
      nclosest = i;
    }
  }
  return nclosest;
}

// get interpolated color value at palette position
int get_palette_color(long x) {
  int indice = FALSE;
  UBYTE *c;

  if (x == colorData.indices[color_index]) {
    // x is at selected indice so return highlight color (light gray)
    return 0xcccccc;
  } else {
    for (int i = 0; i < colorData.nindex; i++) {
      if (x == colorData.indices[i])
        indice = TRUE;
    }
    if (indice)
      // x is at an indice so return marker color (black)
      return 0;
    // otherwise return interpolated color
    c = get_color((float)x);
    return c[0] << 16 | c[1] << 8 | c[2];
  }
}

// add color indice at palette position
void add_color_index(long x) {
  int ipos = -1;
  UBYTE *c = get_color((float)x);

  for (int i = 0; i < colorData.nindex; i++) {
    if (x > colorData.indices[i])
      ipos = i;
  }
  ipos++;
  // shift indices and color components to the right
  for (int i = colorData.nindex; i > ipos; i--) {
    colorData.indices[i] = colorData.indices[i - 1];
    for (int j = 0; j < 3; j++)
      colorData.comps[j][i] = colorData.comps[j][i - 1];
  }

  // insert the indice and color components
  colorData.indices[ipos] = (int)x;
  for (int j = 0; j < 3; j++)
    colorData.comps[j][ipos] = c[j];
  colorData.nindex++;
  color_index = ipos;
}

// remove color indice at current selected index
void remove_color_index() {
  int nlast = colorData.nindex - 1;
  // shift indices and color components to the left to "remove" the index
  for (int i = color_index; i < nlast; i++) {
    colorData.indices[i] = colorData.indices[i + 1];
    for (int j = 0; j < 3; j++)
      colorData.comps[j][i] = colorData.comps[j][i + 1];
  }
  colorData.nindex--;
}

// update palette selection panels
void update_palette_image() {
  long x, y, pos, hpos, ipos, spos;
  int color;
  double i, s;

  hpos = (long)((hue / 360.0) * palette_width);
  ipos = (long)(intensity * palette_width);
  spos = (long)((1.0 - saturation) * (hsl_height - palette_height)) +
         palette_height;
  // generate palette
  for (x = 0; x < palette_width; x++) {
    color = get_palette_color(x);
    for (y = 0; y < palette_height; y++) {
      pos = palette_width * y + x;
      palette_image[pos] = color;
    }
  }
  for (x = 0; x < palette_width; x++) {
    if (x == colorData.shift)
      color = 0;
    else
      color = 0xffffff;
    for (y = palette_height; y < shift_height; y++) {
      pos = palette_width * y + x;
      palette_image[pos] = color;
    }
  }
  if (hue >= 0) {
    // generate hsl selection panel
    for (x = 0; x < palette_width; x++) {
      i = (double)x / palette_width;
      // generate intensity (x) by saturation (y) selection
      for (y = shift_height; y < hsl_height; y++) {
        s = 1.0 - (double)(y - shift_height) / (hsl_height - shift_height);
        pos = palette_width * y + x;
        color = hsl2rgb(hue, s, i);
        if (x == ipos || y == spos)
          // x is at current intensity or y is at current saturation so set
          // color to white xor interpolated color
          palette_image[pos] = 0xffffff ^ color;
        else
          // else set interpolated color
          palette_image[pos] = color;
      }
      // generate hue selection
      if (x == hpos)
        // x is at selected hue so set highlight color (black)
        color = 0;
      else
        // otherwise set interpolated color
        color = hsl2rgb(i * 360.0, (double)0.5, (double)0.5);
      for (y = hsl_height; y < total_height; y++) {
        pos = palette_width * y + x;
        palette_image[pos] = color;
      }
    }
  }

  XPutImage(palette_display, palette_window, DefaultGC(palette_display, 0),
            palette_ximage, 0, 0, 0, 0, palette_width, total_height);
}

void update_palette() {
  int color = hsl2rgb(hue, saturation, intensity);
  colorData.comps[0][color_index] = (color & 0xff0000) >> 16;
  colorData.comps[1][color_index] = (color & 0x00ff00) >> 8;
  colorData.comps[2][color_index] = color & 0x0000ff;
  update_palette_image();
}

void process_button_event(int button, int x, int y, void (*update_image)()) {
  UBYTE *c;
  int color, c0, c1;

  if (palette_y < palette_height) {
    // button press in palette selection panel
    switch (button) {
    case 1: // primary button - select palette indice
      color_index = find_color_index(x);
      c = get_color((float)colorData.indices[color_index]);
      color = c[0] << 16 | c[1] << 8 | c[2];
      rgb2hsl(color);
      update_palette_image();
      break;
    case 2: // middle button - change indice to position in palette
      if (color_index > 0)
        c0 = colorData.indices[color_index - 1];
      else
        c0 = 0;
      if (color_index < colorData.nindex - 1)
        c1 = colorData.indices[color_index + 1];
      else
        c1 = palette_width;
      if (x > c0 && x < c1) {
        colorData.indices[color_index] = x;
        update_palette_image();
        update_image();
      }
      break;
    case 3: // secondary button - add indice at position in palette
      add_color_index(x);
      if (x >= palette_width) {
        teardown_palette();
        init_palette_display();
      }
      update_palette_image();
      break;
    }
  } else if (palette_y < shift_height) {
    colorData.shift = x;
    update_palette();
    update_image();
  } else if (hue >= 0 && palette_y < hsl_height) {
    // else button press in intensity by saturation selection panel
    // any button - select current intensity (x) / saturation (y)
    intensity = (double)x / palette_width;
    saturation =
        1.0 - (double)(y - palette_height) / (hsl_height - palette_height);
    update_palette();
    update_image();
  } else {
    // else button press in hue selection panel
    // any button - select current hue
    hue = 360.0 * (double)x / palette_width;
    update_palette();
    update_image();
  }
}

int process_palette_event(void (*update_image)()) {
  Atom wmDeleteWindow = XInternAtom(palette_display, "WM_DELETE_WINDOW", FALSE);
  XEvent ev;

  XSetWMProtocols(palette_display, palette_window, &wmDeleteWindow, 1);
  XNextEvent(palette_display, &ev);

  switch (ev.type) {
  case Expose:
    XPutImage(palette_display, palette_window, DefaultGC(palette_display, 0),
              palette_ximage, 0, 0, 0, 0, palette_width, total_height);
    break;
  case MotionNotify:
    dx = abs(last_x - ev.xbutton.x);
    dy = abs(last_y - ev.xbutton.y);
    if (dx > 5 || dy > 5) {
      last_x = ev.xbutton.x;
      last_y = ev.xbutton.y;
      process_button_event(1, ev.xbutton.x, ev.xbutton.y, update_image);
    }
    break;
  case ButtonPress:
    last_x = ev.xbutton.x;
    last_y = ev.xbutton.y;
    palette_y = ev.xbutton.y;
    process_button_event(ev.xbutton.button, ev.xbutton.x, ev.xbutton.y,
                         update_image);
    break;
  case KeyPress:
    switch (ev.xkey.keycode) {
    case 9: // esc - exit palette display
      teardown_palette();
      return FALSE;
    case 119: // delete - delete current selected indice
      remove_color_index();
      if (color_index == colorData.nindex) {
        teardown_palette();
        init_palette_display();
      }
      color_index = -1;
      update_palette_image();
      update_image();
      break;
    }
  case ClientMessage:
    // handle close window gracefully
    if (ev.xclient.data.l[0] == wmDeleteWindow) {
      teardown_palette();
      return FALSE;
    }
    break;
  }
  return TRUE;
}

void palette(void (*update_image)()) {
  int open = TRUE;

  while (open) {
    if (palette_display == NULL) {
      init_palette_display();
      update_palette_image();
    }
    open = process_palette_event(update_image);
  }
}
