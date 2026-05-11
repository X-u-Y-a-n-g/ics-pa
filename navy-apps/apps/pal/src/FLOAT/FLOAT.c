#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  // assert(0);
  // return 0;
  return (FLOAT)(((int64_t)a * b) / FLOAT_SCALE);
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  // assert(0);
  // return 0;
  assert(b != 0);
  return (FLOAT)(((int64_t)a * FLOAT_SCALE) / b);
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */

  // assert(0);
  // return 0;

  uint32_t bits = *(uint32_t *)&a;
  uint32_t sign = bits >> 31;
  uint32_t exp = (bits >> 23) & 0xff;
  uint32_t frac = bits & 0x7fffff;

  assert(exp != 0xff);

  if (exp == 0) {
    return 0;
  }

  uint64_t mant = (1u << 23) | frac;
  int shift = (int)exp - 127 - 7;
  uint64_t val = (shift >= 0) ? (mant << shift) : (mant >> -shift);

  assert((!sign && val <= INT32_MAX) || (sign && val <= (uint64_t)INT32_MAX + 1));
  return sign ? -(FLOAT)val : (FLOAT)val;
}

FLOAT Fabs(FLOAT a) {
  // assert(0);
  // return 0;
  assert(a != INT32_MIN);
  return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
