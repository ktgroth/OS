
#include "../include/libc/string.h"

void int_to_ascii(uint64_t n, char str[]) {
    uint64_t i, sign;

    if ((sign = n) < 0)
        n = -n;

    i = 0;
    do {
        str[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0)
        str[i++] = '-';
        str[i] = '\0';

    reverse(str);
}

uint64_t ascii_to_int(char str[]) {
    uint64_t n = 0;
    uint64_t power = 1;
    for (uint64_t i = strlen(str); i > 0; --i, power *= 10)
        n += (str[i - 1] - '0') * power;

    return n;
}

void hex_to_ascii(uint64_t n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    uint64_t tmp;
    uint64_t i;
    for (i = 60; i > 0; i -= 4) {
        tmp = (n >> i) & 0x0F;
        if (tmp == 0 && zeros == 0)
            continue;
    
        zeros = 1;
        if (tmp >= 0x0A)
            append(str, tmp - 0x0A + 'A');
        else
            append(str, tmp + '0');
    }

    tmp = n & 0x0F;
    if (tmp >= 0x0A)
        append(str, tmp - 0x0A + 'A');
    else
        append(str, tmp + '0');
}

void reverse(char s[]) {
    uint64_t c, i, j;
    for (i = 0, j = strlen(s) - 1; i < j; ++i, --j) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

uint64_t strlen(char s[]) {
    uint64_t i = 0;
    for (; s[i]; ++i);
    return i;
}

void append(char s[], char n) {
    uint64_t len = strlen(s);
    s[len] = n;
    s[len + 1] = '\0';
}

void backspace(char s[]) {
    uint64_t len = strlen(s);
    s[len - 1] = '\0';
}

uint64_t strcmp(char s1[], char s2[]) {
    uint64_t i;
    for (i = 0; s1[i] == s2[i]; ++i)
        if (s1[i] == '\0')
            return 0;

    return s1[i] - s2[i];
}

uint64_t strncmp(char s1[], char s2[], uint64_t n) {
    while (n > 0 && *s1 != '\0' && *s1 == *s2) {
        ++s1;
        ++s2;
        --n;
    }

    if (n == 0 || (*s1 == '\0' && *s2 == '\0'))
        return 0;
    else
        return *s1 - *s2;
}

void *strcpy(char s1[], char s2[]) {
    uint64_t i;
    for (i = 0; s2[i]; ++i)
        s1[i] = s2[i];
    return s1;
}

void *strncpy(char s1[], char s2[], uint64_t n) {
    uint64_t i;
    for (i = 0; s2[i] && i < n; ++i)
        s1[i] = s2[i];
    return s1;
}

