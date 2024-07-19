#include "complexfunc.h"
#include "fractal.h"
#include <math.h>

double escape, logtwo;

extern unsigned int ctype;
extern int julia, maxIter, nthread;
extern float *buf, *pbuf;
extern int sample_pbuf(long dx, long pos);
extern struct ViewData viewData;
extern struct FractalData data, pdata;

/**
 * @brief Get the world ordinate value - translate screen ordinate value to world ordinate value - ord = c + d * ((scr / wd) - 0.5);
 * 
 * @param ord World ordinate value to set translated value to
 * @param d World distance
 * @param c World center
 * @param scr Screen ordinate value
 * @param wd Screen distance
 * @return double World ordinate value
 */
double get_world_ord(double d, double c, long scr, long wd) {
  return c + d * (((double)scr /wd) - 0.5);
}

/**
 * @brief Get the screen ordinate value - translate world ordinate value to screen ordinate value - ord = wd * (((wld - c) / d) + 0.5);
 * 
 * @param d World distance
 * @param c World center
 * @param wld World ordinate value
 * @param wd Screen distance
 * @return long Screen ordinate value
 */
long get_screen_ord(double d, double c, double wld, long wd) {
  return (double)wd * (((wld - c) / d) + 0.5); 
}

struct Coord get_delta(struct ViewData viewData, double size) {
  struct Coord delta;
  double aspect;

  aspect = (double)viewData.xres / viewData.yres;

  if (viewData.xres > viewData.yres) {
    delta.x = size * aspect;
    delta.y = size;
  } else {
    delta.x = size;
    delta.y = size / aspect;
  }

  return delta;
}

struct Coord get_coord(struct ViewData viewData, double xc, double yc, double size, long ix, long iy) {
  struct Coord delta, coord;

  delta = get_delta(viewData, size);

  // x = data.xc + delta.x * ((ix / viewData.xres) - 0.5);
  coord.x = get_world_ord(delta.x, xc, ix, viewData.xres);

  // y = data.yc + delta.y * ((iy / viewData.yres) - 0.5);
  coord.y = get_world_ord(delta.y, yc, iy, viewData.yres);

  return coord;
}

double bailout(double complex z) {
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

  while (((r2 = bailout(z)) <= escape) && iter < maxIter) {
    z = z * z + c;
    r = r2;
    iter++;
  }
  if (iter < maxIter) {
    fiter = (float)iter;
    if (!julia && ctype == 0) {
      fiter +=
          (float)(1.0 - (double)(log((double)(log(r) / 2.0) / logtwo) / logtwo));
    }
    return fiter;
  }
  return -1.0;
}

void complex_setup() {
  escape = 256.0;
  logtwo = log(2.0);
}

int get_pos(int x, int y, int d) {
  if ((x < d) || (x > viewData.xres - d))
    return -1;
  if ((y < d) || (y > viewData.yres - d))
    return -1;
  return viewData.xres * y + x;
}

void generate_fractal(int x1) {
  double x, y;
  double x0, y0, xc, yc, size;
  double pxc, pyc, psize;
  double complex z;
  int pos, ppos, ppx, ppy, dp;
  struct Coord delta, pdelta;

  complex_setup();
  sscanf(data.size, "%lg", &size);
  sscanf(pdata.size, "%lg", &psize);
  sscanf(data.xc, "%lg", &xc);
  sscanf(pdata.xc, "%lg", &pxc);
  sscanf(data.yc, "%lg", &yc);
  sscanf(pdata.yc, "%lg", &pyc);
  delta = get_delta(viewData, size);

  if (psize >= size) {
    dp = (0.9999 + psize / size);
    pdelta = get_delta(viewData, psize);
  } else {
    dp = 0;
    pdelta.x = delta.x;
    pdelta.y = delta.y;
  }

  for (int py = 0; py < viewData.yres; py++) {
    y = get_world_ord(delta.y, yc, py, viewData.yres);
    ppy = get_screen_ord(pdelta.y, pyc, y, viewData.yres);
    for (int px = x1; px < viewData.xres; px += nthread) {
      x = get_world_ord(delta.x, xc, px, viewData.xres);
      ppx = get_screen_ord(pdelta.x, pxc, x, viewData.xres);
      pos = viewData.xres * py + px;
      ppos = get_pos(ppx, ppy, 0);
      if (sample_pbuf(dp, ppos)) {
        buf[pos] = -1.0;
        continue;
      }
      z = x + y * I;

      if (julia) {
        sscanf(data.x0, "%lg", &x0);
        sscanf(data.y0, "%lg", &y0);
        buf[pos] = iterate(z, x0 + y0 * I);
      } else {
        buf[pos] = iterate(0.0, z);
      }
    }
  }
}
