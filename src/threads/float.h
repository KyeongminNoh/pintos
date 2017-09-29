#ifndef FLOAT_H
#define FLOAT_H

#define P 17
#define Q 14
#define FRACTION 1 << (Q)

#define INT_TO_FP(n) (n) * (FRACTION)
#define FP_TO_INT_ZERO(x) (x) / (FRACTION)
#define FP_TO_INT_ROUND(x) ((x) >= 0 ? ((x) + (FRACTION) / 2)\
                                           / (FRACTION) : ((x) - (FRACTION) / 2)\
                                           / (FRACTION))
#define ADD_FP(x, y) (x) + (y)
#define SUB_FP(x, y) (x) - (y)
#define ADD_INT(x, n) (x) + (n) * (FRACTION)
#define SUB_INT(x, n) (x) - (n) * (FRACTION)
#define MULT_FP(x, y) ((int64_t)(x)) * (y) / (FRACTION)
#define MULT_INT(x, n) (x) * (n)
#define DIV_FP(x, y) ((int64_t)(x)) * (FRACTION) / (y)
#define DIV_INT(x, n) (x) / (n)

#endif

