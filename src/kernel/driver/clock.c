
#include "../include/cpu/ports.h"
#include "../include/driver/clock.h"

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

