
#include "../include/cpu/ports.h"
#include "../include/driver/clock.h"
#include "../include/driver/vga.h"
#include "../include/libc/string.h"

#define NMI_DISABLE     0x800
#define CMOS_OUT        0x70
#define CMOS_IN         0x71
#define SECONDS         0x00    // 0-59
#define MINUTES         0x02    // 0-59
#define HOURS           0x04    // 0-23
                                // 1-12, highest bit set if pm
#define WEEKDAY         0x06    // 1-7, Sunday = 1
#define DAY             0x07    // 1-31
#define MONTH           0x08    // 1-12
#define YEAR            0x09    // 0-99
#define CENTURY         0x32    // 19-20
#define STATUS_A        0x0A
#define STATUS_B        0x0B


void wait_uip() {
    outb(CMOS_OUT, STATUS_A);
    while (inb(CMOS_IN) & 0x80);
}

uint8_t get_RTC_register(uint32_t reg) {
    wait_uip();
    outb(CMOS_OUT, reg);
    return inb(CMOS_IN);
}

uint8_t get_second() {
    return get_RTC_register(SECONDS);
}

uint8_t get_minute() {
    return get_RTC_register(MINUTES);
}

uint8_t get_hour() {
    return get_RTC_register(HOURS);
}

uint8_t get_weekday() {
    return get_RTC_register(WEEKDAY);
}

uint8_t get_day() {
    return get_RTC_register(DAY);
}

uint8_t get_month() {
    return get_RTC_register(MONTH);
}

uint8_t get_year() {
    return get_RTC_register(YEAR);
}

uint8_t get_century() {
    return get_RTC_register(CENTURY);
}

char *days_str[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
char *months_str[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "", "", "", "", "", "" "October", "November", "December" };
void print_current_time() {
    uint8_t hour = get_hour();
    uint8_t min = get_minute();
    uint8_t sec = get_second();
    char *weekday = days_str[get_weekday() - 1];
    uint8_t day = get_day();
    char *month = months_str[get_month() - 1];
    uint16_t year = 0x2000 + get_year();

    char str[32] = "";
    hex_to_ascii(hour, str);
    putstr(str + 2, COLOR_WHT, COLOR_BLK);
    str[0] = '\0';
    
    putchar(':', COLOR_WHT, COLOR_BLK);
    hex_to_ascii(min, str);
    if (strlen(str) <= 3)
        putchar('0', COLOR_WHT, COLOR_BLK);

    putstr(str + 2, COLOR_WHT, COLOR_BLK);
    str[0] = '\0';

    putchar(':', COLOR_WHT, COLOR_BLK);
    hex_to_ascii(sec, str);
    if (strlen(str) <= 3)
        putchar('0', COLOR_WHT, COLOR_BLK);

    putstr(str + 2, COLOR_WHT, COLOR_BLK);
    str[0] = '\0';

    putstr(" UTC\n", COLOR_WHT, COLOR_BLK);

    putstr(weekday, COLOR_WHT, COLOR_BLK);
    putstr(", ", COLOR_WHT, COLOR_BLK);
    
    putstr(month, COLOR_WHT, COLOR_BLK);
    putchar(' ', COLOR_WHT, COLOR_BLK);

    hex_to_ascii(day, str);
    putstr(str + 2, COLOR_WHT, COLOR_BLK);
    str[0] = '\0';

    putstr(", ", COLOR_WHT, COLOR_BLK);
    hex_to_ascii(year, str);
    putstr(str + 2, COLOR_WHT, COLOR_BLK);
    putchar('\n', COLOR_WHT, COLOR_BLK);
}

