#include "fractal.h"
#include <string.h>

int is = 0;
pthread_t thr[MAX_THREAD];

extern int nthread;
extern void generate_fractal(int x1);
extern void arb_generate_fractal(int x1);
extern void complex_setup();
extern void arb_setup();
extern void arb_teardown();

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
    complex_setup();
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
  if (arb) arb_teardown();
  return 1;
}
