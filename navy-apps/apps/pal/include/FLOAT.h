#ifndef __FLOAT_H__
#define __FLOAT_H__

#include "assert.h"
#include <stdint.h>

typedef int FLOAT;

#define FLOAT_FRAC_BITS 16
#define FLOAT_SCALE (1 << FLOAT_FRAC_BITS)

static inline int F2int(FLOAT a) {
  // assert(0);
  // return 0;
  return a / FLOAT_SCALE;
}

static inline FLOAT int2F(int a) {
  // assert(0);
  // return 0;
  return (FLOAT)((int64_t)a * FLOAT_SCALE);
}

static inline FLOAT F_mul_int(FLOAT a, int b) {
  // assert(0);
  // return 0;
  return (FLOAT)((int64_t)a * b);
}

static inline FLOAT F_div_int(FLOAT a, int b) {
  // assert(0);
  // return 0;
  assert(b != 0);
  return a / b;
}

FLOAT f2F(float);
FLOAT F_mul_F(FLOAT, FLOAT);
FLOAT F_div_F(FLOAT, FLOAT);
FLOAT Fabs(FLOAT);
FLOAT Fsqrt(FLOAT);
FLOAT Fpow(FLOAT, FLOAT);

#endif
