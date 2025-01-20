#include "fff.h"
#include "fractal.h"
#include <math.h>
#include <stdio.h>

float *buf = NULL;
UBYTE color[3];
int pipe = FALSE;
struct FractalData data;
struct ViewData viewData;
struct ColorData colorData, ignoreColorData;

void close_buf() { 
  if (buf != NULL) free(buf);
  buf = NULL;
}

long open_buf() {
  long buflen;

  buflen = viewData.xres * viewData.yres;

  close_buf();
  if ((buf = calloc((long)buflen, sizeof(float))) == NULL) {
    fprintf(stderr, "main - insufficient memory!!!\n");
    return (0);
  }
  return buflen;
}

void clear_buf() {
  long buflen;

  buflen = viewData.xres * viewData.yres;

  for (int pos = 0; pos < buflen; pos++)
    buf[pos] = 0.0;
}

UBYTE *get_color(float fiter) {
  int nc;
  if (fiter < 0.0) {
    for (nc = 0; nc < 3; nc++)
      color[nc] = 0;
  } else {
    int nlast = colorData.nindex - 1;
    float riter = fmodf(fiter + (float)colorData.shift, (float)colorData.indices[nlast]);
    float rf;
    int ni;
    UBYTE comp1, comp2;

    for (ni = 0; ni <= nlast; ni++) {
      if (colorData.indices[ni] > riter)
        break;
    }
    // if in the first interval or after the last then interpolate between last
    // and first colors
    if (ni == 0 || ni > nlast) {
      if (colorData.indices[0] > 0) {
        rf = riter / (float)colorData.indices[0];
        for (nc = 0; nc < 3; nc++) {
          comp1 = colorData.comps[nc][nlast];
          comp2 = colorData.comps[nc][0];
          color[nc] = (UBYTE)((float)comp1 + (float)(comp2 - comp1) * rf);
        }
      } else {
        for (nc = 0; nc < 3; nc++)
          color[nc] = colorData.comps[nc][0];
      }
    }
    // otherwise interpolate between colors at the start and end of the interval
    else {
      if (colorData.indices[ni] > colorData.indices[ni - 1]) {
        rf = (riter - (float)colorData.indices[ni - 1]) /
             (float)(colorData.indices[ni] - colorData.indices[ni - 1]);
        for (nc = 0; nc < 3; nc++) {
          comp1 = colorData.comps[nc][ni - 1];
          comp2 = colorData.comps[nc][ni];
          color[nc] = (UBYTE)((float)comp1 + (float)(comp2 - comp1) * rf);
        }
      } else {
        for (nc = 0; nc < 3; nc++)
          color[nc] = colorData.comps[nc][ni];
      }
    }
  }
  return color;
}

FILE *open_file(char *fspec, char *mode) {
  if (*fspec == '|') {
    ++fspec;
    pipe = TRUE;
    return popen(fspec, mode);
  } else {
    pipe = FALSE;
    return fopen(fspec, mode);
  }
}

int close_file(FILE *stream) {
  int rc = 0;

  if (pipe)
    rc = pclose(stream);
  else
    rc = fclose(stream);
  pipe = FALSE;

  return rc;
}

int read_fff(char *file) {
  FILE *fp;
  struct Chunk header;
  long id, buflen;

  if (file == 0)
    fp = stdin;
  else if ((fp = open_file(file, "r")) == NULL)
    return (0);

  SafeRead(fp, &header, sizeof(header));
  if (header.ckID != ID_FORM) {
    fclose(fp);
    return (0);
  }

  SafeRead(fp, &id, sizeof(id));
  if (id != ID_FRCL) {
    fclose(fp);
    return (0);
  }

  for (;;) {
    SafeRead(fp, &header, sizeof(header));
    if (header.ckID == ID_ENDD)
      break;

    switch (header.ckID) {
    case ID_GLBL:
      SafeRead(fp, &data, sizeof(struct FractalData));
      SafeRead(fp, &viewData, sizeof(struct ViewData));
      SafeRead(fp, &colorData, sizeof(struct ColorData));
      break;
    case ID_DATA:
      if ((buflen = open_buf())) {
        SafeRead(fp, buf, buflen * sizeof(float));
      } else {
        return (0);
      }
      break;
    }
  }
  fclose(fp);
  return (1);
}

int save_fff(char *file) {
  FILE *fp;

  if (file == 0)
    fp = stdout;
  else if ((fp = open_file(file, "w")) == NULL)
    return (0);

  struct Chunk header;
  long formSize = sizeof(header) + sizeof(long) + sizeof(header) +
                  sizeof(struct FractalData) + sizeof(struct ViewData) +
                  sizeof(struct ColorData) + sizeof(header) +
                  (viewData.xres * viewData.yres * sizeof(float));

  header.ckID = ID_FORM;
  header.ckSize = formSize;

  SafeWrite(fp, &header, sizeof(header));
  long id = ID_FRCL;

  SafeWrite(fp, &id, sizeof(long));

  header.ckID = ID_GLBL;
  header.ckSize = sizeof(struct FractalData) + sizeof(struct ViewData) +
                  sizeof(struct ColorData);

  SafeWrite(fp, &header, sizeof(header));
  SafeWrite(fp, &data, sizeof(struct FractalData));
  SafeWrite(fp, &viewData, sizeof(struct ViewData));
  SafeWrite(fp, &colorData, sizeof(struct ColorData));

  header.ckID = ID_DATA;
  header.ckSize = viewData.xres * viewData.yres * sizeof(float);

  SafeWrite(fp, &header, sizeof(header));
  SafeWrite(fp, buf, (size_t)header.ckSize);

  header.ckID = ID_ENDD;
  header.ckSize = 0L;

  SafeWrite(fp, &header, sizeof(header));
  fflush(fp);
  fclose(fp);
  return (1);
}
