
#include "../include/libc/stdlib.h"

void memcpy(uint8_t *src, uint8_t *dest, uint32_t n) {
    int i;
    for (i = 0; i < n; ++i)
        *(dest + i) = *(src + i);
}

void memset(uint8_t *dest, uint8_t val, uint32_t n) {
    uint8_t *temp = (uint8_t *)dest;
    for (; n != 0; --n)
        *temp++ = val;
}
