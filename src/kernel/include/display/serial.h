#ifndef __DISPLAY_SERIAL
#define __DISPLAY_SERIAL

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char *s);

#endif

