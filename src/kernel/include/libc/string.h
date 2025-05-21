
#ifndef __LIBC_STRING
#define __LIBC_STRING

#include "types.h"

void int_to_ascii(uint32_t n, char str[]);
void hex_to_ascii(uint32_t n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);

#endif
