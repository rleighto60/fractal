#include "fractal.h"
#include <stdio.h>

extern float *buf;
extern void close_buf();
extern struct ViewData viewData;
extern struct ColorData colorData;

extern UBYTE *get_color(float fiter, int nindex, int shift,
                        int indices[MAX_INDICES], UBYTE comps[3][MAX_INDICES]);
extern int read_fff(char *file);

void save_ppm(char *file) {
  FILE *fp;
  UBYTE *c;

  if (file == 0)
    fp = stdout;
  else if ((fp = fopen(file, "w")) == NULL) {
    fprintf(stderr, "save - could not open file!!!\n");
    return;
  }

  int nc;
  long i, j, pos = 0;
  float fiter;

  fprintf(fp, "P6\n%ld %ld\n255\n", viewData.xres, viewData.yres);
  for (j = 0; j < viewData.yres; j++) {
    for (i = 0; i < viewData.xres; i++) {
      fiter = buf[pos++];
      c = get_color(fiter, colorData.nindex, colorData.shift, colorData.indices,
                    colorData.comps);
      for (nc = 0; nc < 3; nc++)
        putc(c[nc], fp);
    }
  }
  fflush(fp);
  fclose(fp);
}

int main(int argc, char **argv) {
  char *input, *output;

  if (argc > 1)
    input = argv[1];
  else
    input = 0;

  if (argc > 2)
    output = argv[2];
  else
    output = 0;

  if (!read_fff(input)) {
    close_buf();
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }
  save_ppm(output);
}
