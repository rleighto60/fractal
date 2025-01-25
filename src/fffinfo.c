#include "fractal.h"
#include <stdio.h>

extern struct FractalData data;
extern struct ViewData viewData;
extern struct ColorData colorData;

extern int read_fff(char *file);
extern void print_info();

int main(int argc, char **argv) {
  char *input;

  if (argc > 1)
    input = argv[1];
  else
    input = 0;

  if (!read_fff(input)) {
    fprintf(stderr, "main - error reading image!!!\n");
    return 0;
  }
  print_info();
}
