#include "arbfunc.h"
#include "fractal.h"
#include <flint/acb.h>
#include <flint/acb_types.h>
#include <flint/arb.h>
#include <flint/arb_types.h>
#include <string.h>

unsigned int ctype = 0;
int julia, maxIter = 10000, nthread = 16;
struct FractalData data, pdata;

extern int save_fff(char *file);
extern struct ViewData viewData;
extern struct ColorData colorData;

extern int setup();
extern void teardown();
extern int fractal(int arb);

void init_color_data() {
  // spectrum 1 0 25 000764 50 206bdd 100 edffff 150 ffaa00 215 310230
  colorData.comps[0][0] = 0x00;
  colorData.comps[1][0] = 0x07;
  colorData.comps[2][0] = 0x64;
  colorData.comps[0][1] = 0x20;
  colorData.comps[1][1] = 0x6b;
  colorData.comps[2][1] = 0xdd;
  colorData.comps[0][2] = 0xed;
  colorData.comps[1][2] = 0xff;
  colorData.comps[2][2] = 0xff;
  colorData.comps[0][3] = 0xff;
  colorData.comps[1][3] = 0xaa;
  colorData.comps[2][3] = 0x00;
  colorData.comps[0][4] = 0x31;
  colorData.comps[1][4] = 0x02;
  colorData.comps[2][4] = 0x30;
  colorData.indices[0] = 25;
  colorData.indices[1] = 50;
  colorData.indices[2] = 100;
  colorData.indices[3] = 150;
  colorData.indices[4] = 215;
  colorData.nindex = 5;
  colorData.shift = 0;
}

void test_fractal() {
  // 4.4382780058805177e-01 -3.7195966921546991e-01 1.4210854715202003717422485351563E-13
  // strcpy(data.xc,  "4.438278005880517300000001e-01"); // shift right - more negative
  // strcpy(data.yc, "-3.719596692154698999993013e-01"); // shift down - more negative
  // strcpy(data.size, "1.0e-25");
  // 3.6695927415858831e-01 -6.4601356974145441e-01 1.4210854715202003717422485351563E-13
  strcpy(data.xc,  "3.669592741585804246087909870794e-01"); // shift right - more negative
  strcpy(data.yc, "-6.460135697414555146007659111105e-01"); // shift down - more negative
  strcpy(data.size, "1.0e-30");
  // 0.3750001200618655	-0.2166393884377127 0.000000000002
  // strcpy(data.xc, "0.3750001200618655");
  // strcpy(data.yc, "-0.2166393884377127");
  // strcpy(data.size, "2E-12");
  viewData.xres = 800L;
  viewData.yres = 600L;
  viewData.scale = 1;

  setup();
  fractal(1);
  init_color_data();
  save_fff("test.fff");
  teardown();
}

int main(int argc, char **argv) {
  test_fractal();
}
