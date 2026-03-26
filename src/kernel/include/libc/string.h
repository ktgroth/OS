
#ifndef __LIBC_STRING
#define __LIBC_STRING

#include "types.h"

void int_to_ascii(int64_t n, char str[]);
uint64_t ascii_to_int(const char str[]);
void hex_to_ascii(uint64_t n, char str[]);
void reverse(char s[]);
uint64_t strlen(const char s[]);
void backspace(char s[]);
void append(char s[], char n);
uint64_t strcmp(const char s1[], const char s2[]);
uint64_t strncmp(const char s1[], const char s2[], uint64_t n);

char *strcpy(char s1[], const char s2[]);
char *strncpy(char s1[], const char s2[], uint64_t n);
char *strtok(char *str, char *delim);

#endif
