
#include "../include/utils/stdlib.h"

size_t strlen(const uint8_t *s) {
    size_t size;
    for (size = 0; s[size]; ++size);
    return size;
}

void strcpy(uint8_t *dst, const uint8_t *src) {
    size_t i;
    for (i = 0; src[i]; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

int8_t strcmp(const uint8_t *s1, const uint8_t *s2) {
    size_t i;
    for (i = 0; s1[i] == s2[i]; ++i)
        if (s1[i] == '\0')
            return 0;

    return s1[i] - s2[i];
}

void strncpy(uint8_t *dst, const uint8_t *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

int8_t strncmp(const uint8_t *s1, const uint8_t *s2, size_t n) {
    size_t i;
    for (i = 0; i < n && s1[i] == s2[i]; ++i)
        if (s1[i] == s2[i])
            return 0;

    return s1[i] - s2[i];
}

void memcpy(void *dst, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)s;

    size_t i;
    for (i = 0; i < n; ++i)
        d[i] = s[i];
}

void swap(uint64_t *s1, uint64_t *s2) {
    uint64_t t = *s1;
    *s1 = *s2;
    *s2 = t;
}

int8_t memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *d1 = (const uint8_t *)s1;
    const uint8_t *d2 = (const uint8_t *)s2;

    size_t i;
    for (i = 0; i < n; ++i)
        if (d1[i] != d2[i])
            return d1[i] - d2[i];

    return 0;
}

uint8_t memeq(const void *s1, const void *s2, size_t n) {
    const uint8_t *d1 = (const uint8_t *)s1;
    const uint8_t *d2 = (const uint8_t *)s2;

    size_t i;
    for (i = 0; i < n; ++i)
        if (d1[i] != d2[i])
            return 0;

    return 1;
}

void memset(void *dst, uint8_t value, size_t n) {
    uint8_t *d = (uint8_t *)dst;

    size_t i;
    for (i = 0; i < n; ++i)
        d[i] = value;
}

