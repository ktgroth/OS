
#ifndef __LIBC_STRING
#define __LIBC_STRING

#include "types.h"

void int_to_ascii(uint64_t n, char str[]);
uint64_t ascii_to_int(char str[]);
void hex_to_ascii(uint64_t n, char str[]);
void reverse(char s[]);
uint64_t strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
uint64_t strcmp(char s1[], char s2[]);
uint64_t strncmp(char s1[], char s2[], uint64_t n);

void *strcpy(char s1[], char s2[]);
void *strncpy(char s1[], char s2[], uint64_t n);

#endif
