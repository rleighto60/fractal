#include "arbfunc.h"
#include "fractal.h"
#include <flint/acb.h>
#include <flint/acb_types.h>
#include <flint/arb.h>
#include <flint/arb_types.h>
#include <flint/arf.h>
#include <flint/arf_types.h>
#include <flint/flint.h>

slong prec = 1024;
arb_t arb_escape, arb_logtwo;

extern int maxIter, nthread;
extern float *buf;
extern int sample_pbuf(long dx, long pos);
extern struct ViewData viewData;
extern struct FractalData data, pdata;

void arb_setup() {
  arb_init(arb_escape);
  arb_init(arb_logtwo);

  arb_set_str(arb_escape, "2.0", prec);
  arb_const_log2(arb_logtwo, prec);
}

void arb_teardown() {
  arb_clear(arb_escape);
  arb_clear(arb_logtwo);
}

long arb_get_long_div(arb_t x, arb_t y) {
  long cl;
  arb_t c;

  arb_init(c);
  arb_set(c, x);
  arb_div(c, c, y, prec);
  arb_ceil(c, c, prec);
  sscanf(arb_get_str(c, 32, ARB_STR_NO_RADIUS), "%ld", &cl);

  arb_clear(c);
  return cl;
}

/**
 * @brief Get the world ordinate value - translate screen ordinate value to world ordinate value - ord = c + d * ((scr / wd) - 0.5);
 * 
 * @param ord World ordinate value to set translated value to
 * @param d World distance
 * @param c World center
 * @param scr Screen ordinate value
 * @param wd Screen distance
 * @param prec Arbitrary value precision in bits
 */
void arb_get_world_ord(arb_t ord, arb_t d, arb_t c, long scr, long wd) {
  arf_t d2;

  arf_init(d2);

  // ord = c + d * ((scr / wd) - 0.5);
  arf_set_d(d2, 0.5);
  arb_set_ui(ord, scr);
  arb_div_si(ord, ord, wd, prec);
  arb_sub_arf(ord, ord, d2, prec);
  arb_mul(ord, ord, d, prec);
  arb_add(ord, ord, c, prec);

  arf_clear(d2);
}

/**
 * @brief Get the screen ordinate value - translate world ordinate value to screen ordinate value - ord = wd * (((wld - c) / d) + 0.5);
 * 
 * @param d World distance
 * @param c World center
 * @param wld World ordinate value
 * @param wd Screen distance
 * @param prec Arbitrary value precision in bits
 * @return long Screen ordinate value
 */
long arb_get_screen_ord(arb_t d, arb_t c, arb_t wld, long wd) {
  long scr;
  arb_t ord;
  arf_t d2;

  arb_init(ord);
  arf_init(d2);

  // ord = wd * (((wld - c) / d) + 0.5);
  arf_set_d(d2, 0.5);
  arb_set(ord, wld);
  arb_sub(ord, ord, c, prec);
  arb_div(ord, ord, d, prec);
  arb_add_arf(ord, ord, d2, prec);
  arb_mul_si(ord, ord, wd, prec);
  sscanf(arb_get_str(ord, 32, ARB_STR_NO_RADIUS), "%ld", &scr);

  arb_clear(ord);
  arf_clear(d2);
  return scr;
}

void arb_get_delta(struct Coord *delta, struct ViewData viewData, arb_t size) {
  arb_t aspect;

  arb_init(aspect);

  arb_set_si(aspect, viewData.xres);
  arb_div_si(aspect, aspect, viewData.yres, prec);

  if (viewData.xres > viewData.yres) {
    // dx = size * viewData.xres / viewData.yres;
    // dy = size;
    arb_mul(delta->x, size, aspect, prec);
    arb_set(delta->y, size);
  } else {
    // dx = size;
    // dy = size * viewData.yres / viewData.xres;
    arb_set(delta->x, size);
    arb_div(delta->y, size, aspect, prec);
  }

  arb_clear(aspect);
}

void arb_get_coord(struct Coord *coord, struct ViewData viewData, arb_t xc, arb_t yc, arb_t size, long ix, long iy) {
  struct Coord delta;

  arb_init(delta.x);
  arb_init(delta.y);

  arb_get_delta(&delta, viewData, size);

  // x = xc + delta.x * ((ix / viewData.xres) - 0.5);
  arb_get_world_ord(coord->x, delta.x, xc, ix, viewData.xres);

  // y = yc + delta.y * ((iy / viewData.yres) - 0.5);
  arb_get_world_ord(coord->y, delta.y, yc, iy, viewData.yres);

  arb_clear(delta.x);
  arb_clear(delta.y);
}

int arb_bailout(arb_t r, acb_t z) {
    acb_abs(r, z, prec);

    if (arb_gt(r, arb_escape)) {
        return 1;
    }

    return 0;
}

float arb_iterate(acb_t z, acb_t c) {
  long iter = 0;
  float fiter = -1.0, f;
  arb_t r, fac;
  acb_t z2;

  arb_init(r);
  acb_init(z2);

  while (!arb_bailout(r, z) && iter < maxIter) {
    acb_mul(z2, z, z, prec);
    acb_add(z, z2, c, prec);
    iter++;
  }

  acb_clear(z2);

  if (iter < maxIter) {
    fiter = (float)iter;
    // fiter = 1 - (log(log(r)) / log(2))
    arb_log(r, r, prec);
    arb_log(r, r, prec);
    arb_div(r, r, arb_logtwo, prec);
    arb_init(fac);
    arb_set_d(fac, 1.0);
    arb_sub(fac, fac, r, prec);
    sscanf(arb_get_str(fac, 32, ARB_STR_NO_RADIUS), "%f", &f);
    arb_clear(fac);
    fiter += f;
  }

  arb_clear(r);
  return fiter;
}

long arb_get_pos(long x, long y, long d) {
  if ((x < d) || (x > viewData.xres - d))
    return -1;
  if ((y < d) || (y > viewData.yres - d))
    return -1;
  return viewData.xres * y + x;
}

void arb_generate_fractal(int x1) {
  arb_t x, y;
  acb_t z, c;
  arb_t xc, yc, pxc, pyc, size, psize;
  long pos, ppos, ppx, ppy, dp;
  struct Coord delta, pdelta;

  arb_init(x);
  arb_init(y);
  arb_init(delta.x);
  arb_init(delta.y);
  arb_init(pdelta.x);
  arb_init(pdelta.y);
  arb_init(xc);
  arb_init(yc);
  arb_init(size);
  arb_init(pxc);
  arb_init(pyc);
  arb_init(psize);
  acb_init(z);
  acb_init(c);

  arb_set_str(xc, data.xc, prec);
  arb_set_str(yc, data.yc, prec);
  arb_set_str(size, data.size, prec);
  arb_set_str(pxc, pdata.xc, prec);
  arb_set_str(pyc, pdata.yc, prec);
  arb_set_str(psize, pdata.size, prec);

  arb_get_delta(&delta, viewData, size);

  if (arb_ge(psize, size)) {
    dp = arb_get_long_div(psize, size);
    arb_get_delta(&pdelta, viewData, psize);
  } else {
    dp = 0;
    arb_set(pdelta.x, delta.x);
    arb_set(pdelta.y, delta.y);
  }

  for (long py = 0; py < viewData.yres; py++) {
    arb_get_world_ord(y, delta.y, yc, py, viewData.yres);
    ppy = arb_get_screen_ord(pdelta.y, pyc, y, viewData.yres);
    for (long px = x1; px < viewData.xres; px += nthread) {
      pos = viewData.xres * py + px;
      arb_get_world_ord(x, delta.x, xc, px, viewData.xres);
      ppx = arb_get_screen_ord(pdelta.x, pxc, x, viewData.xres);
      ppos = arb_get_pos(ppx, ppy, 0);
      if (sample_pbuf(dp, ppos)) {
        buf[pos] = -1.0;
        continue;
      }
      acb_zero(z);
      acb_set_arb_arb(c, x, y);
      buf[pos] = arb_iterate(z, c);
      // printf("%f\n", buf[pos]);
    }
    printf("%ld %%\r", 100 * py / viewData.yres);
    fflush(stdout);
  }

  arb_clear(x);
  arb_clear(y);
  arb_clear(delta.x);
  arb_clear(delta.y);
  arb_clear(pdelta.x);
  arb_clear(pdelta.y);
  arb_clear(xc);
  arb_clear(yc);
  arb_clear(size);
  arb_clear(pxc);
  arb_clear(pyc);
  arb_clear(psize);
  acb_clear(z);
  acb_clear(c);
}
