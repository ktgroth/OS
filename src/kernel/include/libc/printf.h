#ifndef __LIBC_PRINTF
#define __LIBC_PRINTF

#include "types.h"
#include "stdarg.h"

void putc(char c);
void puts(const char *s);
uint64_t kvprintf(const char *fmt, va_list ap);
uint64_t printf(const char *fmt, ...);

#endif

