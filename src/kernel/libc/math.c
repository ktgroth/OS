
#include "../include/libc/math.h"

#include "../include/libc/string.h"
#include "../include/cpu/ports.h"

void rational_approx(float64_t x, uint64_t max_den, int64_t *num, int64_t *den) {
    int64_t a = (int64_t)x;
    int64_t h1 = 1, k1 = 0;
    int64_t h = a, k = 1;

    double frac = x - a;
    while (frac > 1e-12 && k < max_den) {
        frac = 1.0 / frac;
        a = (int64_t)frac;

        int64_t h2 = h1;
        h1 = h;
        int64_t k2 = k1;
        k1 = k;

        h = a * h1 + h2;
        k = a * k1 + k2;

        frac = frac - a;
    }

    *num = h;
    *den = k;
}

int32_t signi(int32_t x) {
    return x > 0 ? 1 : -1;
}

int32_t signl(int64_t x) {
    return x > 0 ? 1 : -1;
}

int32_t signf(float32_t x) {
    return x > 0.0f ? 1 : -1;
}

int32_t signd(float64_t x) {
    return x > 0.0f ? 1 : -1;
}

uint32_t absi(int32_t x) {
    x = x < 0 ? -x : x;
    return x;
}

uint64_t absl(int64_t x) {
    x = x < 0 ? -x : x;
    return x;
}

uint32_t sqrti(uint32_t x) {
    uint32_t l = 0, h = x, m;
    while (l <= h) {
        m = (h + l) / 2;
        uint32_t mm = m * m;
        if (mm == x)
            return m;
        else if (mm < x)
            l = m + 1;
        else
            h = m - 1;
    }

    return m;
}

uint64_t sqrtl(uint64_t x) {
    uint64_t l = 0, h = x, m;
    while (l < h) {
        m = (h + l) / 2;
        uint64_t mm = m * m;
        if (mm == x)
            return m;
        else if (mm < x)
            l = m + 1;
        else
            h = m - 1;
    }

    return 0;
}

float32_t sqrtf(float32_t x) {
    if (x <= 0.0f)
        return 0.0f;

    float32_t guess = x;

    for (uint32_t i = 0; i < 32; ++i)
        guess = 0.5f * (guess + x / guess);

    return guess;
}

float64_t sqrtd(float64_t x) {
    if (x <= 0.0f)
        return 0.0f;

    float64_t guess = x;

    for (uint32_t i = 0; i < 32; ++i)
        guess = 0.5f * (guess + x / guess);

    return guess;
}

