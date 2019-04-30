#ifndef THREADS_FIXEDPOINT_H
#define THREADS_FIXEDPOINT_H

#include <stdbool.h>
#include <stdint.h>

/* define fixed-point type */
typedef int fixedpoint_t;

/* define P, Q of fractional bit */
#define P 17
#define Q 14
#define F 1<<(Q)

/* define useful fixed-point arithmetic operations
    n : integer | x, y: fixed-point number */
#define CONVERT_TO_FIX(n) (n) * (F)
#define CONVERT_TO_INT_ZERO(x) (x) / (F)
#define CONVERT_TO_INT_NEAR(x) ((x) >= 0 ? ((x) + (F) / 2) / (F) : ((x) - (F) / 2) / (F))

#define ADD_FIX(x, y) (x) + (y)
#define SUB_FIX(x, y) (x) - (y)

#define ADD_MIX(x, n) (x) + (n) * (F)
#define SUB_MIX(x, n) (x) - (n) * (F)

#define MUL_FIX(x, y) ((int64_t) (x)) * (y) / (F)
#define MUL_MIX(x, n) (x) * (n)

#define DIV_FIX(x, y) ((int64_t) (x)) * (F) / (y)
#define DIV_MIX(x, n) (x) / (n)

#endif /* threads/fixed-point.h */