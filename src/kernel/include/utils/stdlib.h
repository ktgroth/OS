#ifndef __UTILS_STDLIB
#define __UTILS_STDLIB

#include "../../../types.h"


#define NULL (void *)0x00


typedef uint64_t size_t;


size_t strlen(const uint8_t *s);
void strcpy(uint8_t *dst, const uint8_t *src);
int8_t strcmp(const uint8_t *s1, const uint8_t *s2);
void strncpy(uint8_t *dst, const uint8_t *src, size_t n);
int8_t strncmp(const uint8_t *s1, const uint8_t *s2, size_t n);

void swap(uint64_t *s1, uint64_t *s2);
void memcpy(void *dst, const void *src, size_t n);
int8_t memcmp(const void *s1, const void *s2, size_t n);
uint8_t memeq(const void *s1, const void *s2, size_t n);
void memset(void *dst, uint8_t value, size_t n);

#endif

