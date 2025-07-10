
#ifndef __LIBC_STDLIB
#define __LIBC_STDLIB

#include "types.h"

void memcpy(uint8_t *src, uint8_t *dest, uint64_t n);
void memset(uint8_t *dest, uint8_t val, uint64_t n);

#endif
