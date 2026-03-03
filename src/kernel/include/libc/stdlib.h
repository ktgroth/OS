
#ifndef __LIBC_STDLIB
#define __LIBC_STDLIB

#include "types.h"

#define MIN(x, y) x > y ? y : x
#define MAX(x, y) x > y ? x : y

void memcpy(uint8_t *src, uint8_t *dest, uint64_t n);
void memset(uint8_t *dest, uint8_t val, uint64_t n);

#endif
