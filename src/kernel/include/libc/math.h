#ifndef __LIBC_MATH
#define __LIBC_MATH

#include "types.h"

void rational_approx(float64_t, uint64_t max_den, int64_t *num, int64_t *den);

int32_t signi(int32_t x);
int32_t signl(int64_t x);
int32_t signf(float32_t x);
int32_t signd(float64_t x);

uint32_t absi(int32_t x);
uint64_t absl(int64_t x);
float32_t absf(float32_t x);
float64_t absd(float64_t x);

uint32_t sqrti(uint32_t x);
uint64_t sqrtl(uint64_t x);
float32_t sqrtf(float32_t x);
float64_t sqrtd(float64_t x);

#define sign(X) _Generic((X),   \
        int32_t:    signi,      \
        int64_t:    signl,      \
        float32_t:  signf,      \
        float64_t:  signd       \
        )(X)

#define abs(X) _Generic((X),    \
        int32_t:    absi,       \
        int64_t:    absl,       \
        float32_t:  absf,       \
        float64_t:  absd        \
        )(X)

#define sqrt(X) _Generic((X),   \
        uint32_t:   sqrti,      \
        uint64_t:   sqrtl,      \
        float32_t:  sqrtf,      \
        float64_t:  sqrtd       \
        )(X)

#endif

