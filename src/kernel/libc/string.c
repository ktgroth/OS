
#include "../include/libc/string.h"

void int_to_ascii(uint32_t n, char str[]) {
    int i, sign;

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

void hex_to_ascii(uint32_t n, char str[]) {
    append(str, '0');
    append(str, 'x');
    char zeros = 0;

    int32_t tmp;
    int i;
    for (i = 28; i > 0; i -= 4) {
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
    int c, i, j;
    for (i = 0, j = strlen(s) - 1; i < j; ++i, --j) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

int strlen(char s[]) {
    int i = 0;
    for (; s[i]; ++i);
    return i;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len + 1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len - 1] = '\0';
}

int strcmp(char s1[], char s2[]) {
    int i;
    for (i = 0; s1[i] == s2[i]; ++i)
        if (s1[i] == '\0')
            return 0;

    return s1[i] - s2[i];
}
