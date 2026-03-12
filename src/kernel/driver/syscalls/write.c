
#include "../../include/driver/vga.h"

#include "../../include/driver/syscalls/write.h"

int64_t sys_write(uint64_t fd, const char *buf, uint64_t count) {
    if (!buf)
        return -1;

    if (fd != 1 && fd != 2)
        return -1;

    for (uint64_t i = 0; i < count; ++i)
        putchar(buf[i], COLOR_WHT, COLOR_BLK);

    return (int64_t)count;
}

