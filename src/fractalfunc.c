#include "fractal.h"
#include <string.h>

int is = 0;
float *pbuf;
pthread_t thr[MAX_THREAD];

extern int nthread;
extern float *buf;
extern struct ViewData viewData;
extern long open_buf();
extern void close_buf();
extern void generate_fractal(int x1);
extern void arb_generate_fractal(int x1);
extern void arb_setup();

int setup() {
  long buflen;

  buflen = open_buf();

  if (buflen == 0) {
    return (0);
  }
  if ((pbuf = calloc(buflen, sizeof(float))) == NULL) {
    fprintf(stderr, "main - insufficient memory!!!\n");
    return (0);
  }
  return (1);
}

void teardown() {
  close_buf();
  free(pbuf);
}

int sample_pbuf(long dx, long pos) {
  if (pos < 0)
    return FALSE;
  if (pbuf[pos] == 0.0)
    return FALSE;
  if (dx == 1)
    return (pbuf[pos] < 0.0);
  if (dx > 0) {
    int flag = TRUE;
    long dy = dx * viewData.xres;
    for (long py = 0; py <= dy; py += viewData.xres) {
      if (!(pbuf[pos + py] < 0.0))
        flag = FALSE;
      for (long px = 1; px <= dx; px++) {
        if (!(pbuf[pos + py + px] < 0.0))
          flag = FALSE;
        if (!(pbuf[pos + py - px] < 0.0))
          flag = FALSE;
        if (!(pbuf[pos - py + px] < 0.0))
          flag = FALSE;
        if (!(pbuf[pos - py - px] < 0.0))
          flag = FALSE;
      }
    }
    return flag;
  }
  return FALSE;
}

void wait() {
  if (is == 0)
    return;
  for (int i = 0; i < is; ++i) {
    pthread_join(thr[i], NULL);
  }
  is = 0;
}

void *_generate_fractal(void *arg) {
  int *px1 = (int *)arg;

  generate_fractal(*px1);
  pthread_exit(NULL);
}

void *_arb_generate_fractal(void *arg) {
  int *px1 = (int *)arg;

  arb_generate_fractal(*px1);
  pthread_exit(NULL);
}

int fractal(int arb) {
  int it, rc, xs[MAX_THREAD];
  pthread_attr_t attr;
  void *func;

  if (arb) {
    arb_setup();
    func = _arb_generate_fractal;
  } else {
    func = _generate_fractal;
  }

  /* Initialize and set thread detached attribute */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (it = 0; it < nthread; ++it) {
    xs[it] = it;
    if ((rc = pthread_create(&thr[it], NULL, func, &xs[it]))) {
      fprintf(stderr, "main - error: pthread_create, rc: %d\n", rc);
      return 0;
    }
  }

  for (it = 0; it < nthread; ++it) {
    pthread_join(thr[it], NULL);
  }
  memcpy(pbuf, buf, viewData.xres * viewData.yres * sizeof(float));
  return 1;
}
