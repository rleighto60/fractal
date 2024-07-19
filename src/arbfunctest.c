#include "arbfunc.h"
#include "fractal.h"
#include <flint/acb.h>
#include <flint/acb_types.h>
#include <flint/arb.h>
#include <flint/arb_types.h>
#include <stdio.h>

unsigned int ctype = 0;
int julia, maxIter = 10000, nthread = 1;
struct FractalData data, pdata;

extern slong prec;
extern long arb_get_long_div(arb_t x, arb_t y);
extern long arb_get_screen_ord(arb_t d, arb_t c, arb_t wld, long wd);
extern void arb_get_world_ord(arb_t ord, arb_t d, arb_t c, long scr, long wd);
extern void arb_get_delta(struct Coord *delta, struct ViewData viewData, arb_t size);
extern void arb_get_coord(struct Coord *coord, struct ViewData viewData, arb_t xc, arb_t yc, arb_t size, long ix, long iy);
extern void arb_setup();
extern void arb_teardown();
extern float arb_iterate(acb_t z, acb_t c);

void test_get_long_div() {
  arb_t x, y;

  arb_init(x);
  arb_init(y);

  arb_set_str(x, "1e-99", prec);
  arb_set_str(y, "3e-100", prec);
  printf("get_long_div %ld\n", arb_get_long_div(x, y));

  arb_clear(x);
  arb_clear(y);
}

void test_get_screen_ord() {
  arb_t d, c, wld;

  arb_init(d);
  arb_init(c);
  arb_init(wld);

  arb_set_str(d, "3e-100", prec);
  arb_set_str(wld, "1e-100", prec);
  printf("get_screen_ord %ld\n", arb_get_screen_ord(d, c, wld, 1000));

  arb_clear(d);
  arb_clear(c);
  arb_clear(wld);
}

void test_get_coord() {
  struct ViewData viewData;
  struct Coord coord, pdelta;
  long ppx, ppy;
  arb_t xc, yc, pxc, pyc, size, psize;

  viewData.xres = 1000;
  viewData.yres = 2000;

  arb_init(coord.x);
  arb_init(coord.y);
  arb_init(pdelta.x);
  arb_init(pdelta.y);
  arb_init(xc);
  arb_init(yc);
  arb_init(size);
  arb_init(pxc);
  arb_init(pyc);
  arb_init(psize);

  arb_set_str(size, "2e-100", prec);
  arb_set_str(psize, "4e-100", prec);
  arb_get_coord(&coord, viewData, xc, yc, size, 0, 0);
  printf("wld coord %s %s\n", arb_get_str(coord.x, 32, 0), arb_get_str(coord.y, 32, 0));
  arb_get_delta(&pdelta, viewData, psize);
  ppx = arb_get_screen_ord(pdelta.x, pxc, coord.x, viewData.xres);
  ppy = arb_get_screen_ord(pdelta.y, pyc, coord.x, viewData.xres);
  printf("scr coord %ld %ld\n", ppx, ppy);

  arb_clear(coord.x);
  arb_clear(coord.y);
  arb_clear(pdelta.x);
  arb_clear(pdelta.y);
  arb_clear(xc);
  arb_clear(yc);
  arb_clear(size);
  arb_clear(pxc);
  arb_clear(pyc);
  arb_clear(psize);
}

void test_iterate() {
  acb_t z, c;
  arb_t x, y;
  float iter;

  acb_init(z);
  acb_init(c);
  arb_init(x);
  arb_init(y);

  acb_zero(z);
  // 2.5216245651245128e-01 -1.6241073608368366e-04
  // 4.4382780058805177e-01 -3.7195966921546991e-01
  arb_set_str(x, "4.5288250103411448e-01", prec);
  arb_set_str(y, "-3.9614699614068416e-01", prec);
  acb_set_arb_arb(c, x, y);
  arb_setup();
  iter = arb_iterate(z, c);
  printf("iterate %f\n", iter);
  arb_teardown();

  acb_clear(z);
  acb_clear(c);
  arb_clear(x);
  arb_clear(y);
}

int main(int argc, char **argv) {
  test_get_long_div();
  test_get_screen_ord();
  test_get_coord();
  test_iterate();
}
