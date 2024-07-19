#include "complexfunc.h"
#include "fractal.h"
#include <stdio.h>

unsigned int ctype = 0;
int julia = FALSE, maxIter = 1000;

extern double get_world_ord(double d, double c, long scr, long wd);
extern struct Coord get_delta(struct ViewData viewData, double size);
extern float iterate(double complex z, double complex c);

void test_fractal() {
  double x, y, xc, yc, size;
  double complex z;
  struct ViewData viewData;
  struct Coord delta;
  float fiter;

  viewData.xres = 100;
  viewData.yres = 100;
  xc = 0.0;
  yc = 0.0;
  size = 2;
  delta = get_delta(viewData, size);

  for (long py = 0; py < viewData.yres; py++) {
    y = get_world_ord(delta.y, yc, py, viewData.yres);
    for (long px = 0; px < viewData.xres; px++) {
      x = get_world_ord(delta.x, xc, px, viewData.xres);
      z = x + y * I;
      fiter = iterate(0.0, z);
      if (fiter > 0.0) {
        printf("%g %g %f\n", x, y, fiter);
      }
    }
  }
}

int main(int argc, char **argv) {
  test_fractal();
}
