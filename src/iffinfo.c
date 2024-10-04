#include "fractal.h"
#include "iff.h"
#include <stdio.h>

extern struct FractalData data;
extern struct ViewData viewData;
extern struct ColorData colorData;

extern int read_iff(char *file);

void print_info() {
  int i;
  printf("size %ld %ld\n", viewData.xres, viewData.yres);
  printf("view %s %s %s\n", data.xc, data.yc, data.size);
  printf("spectrum %d ", colorData.shift);
  for (i = 0; i < colorData.nindex; ++i) {
    printf("%d %02x%02x%02x ", colorData.indices[i], colorData.comps[0][i], colorData.comps[1][i], colorData.comps[2][i]);
  }
  printf("\n");
}

int main(int argc, char **argv) {
  char *input;

  if (argc > 1)
    input = argv[1];
  else
    input = 0;

  if (!read_iff(input)) {
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }
  print_info();
}
