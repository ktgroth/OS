
#include "../include/driver/vga.h"
#include "../include/cpu/ports.h"
#include "../include/libc/printf.h"
#include "../include/libc/string.h"


uint8_t fb_ready = 0;

void putc(char c) {
    if (fb_ready)
        serial_putc(c);
    else
        putchar(c, COLOR_WHT, COLOR_BLK);
}

void puts(const char *s) {
    if (fb_ready)
        serial_puts(s);
    else
        putstr(s, COLOR_WHT, COLOR_BLK);
}

static void krepeat(char c, int32_t n) {
    while (n-- > 0)
        putc(c);
}

static int utoa_base(uint64_t v, char *out, int32_t base, int32_t upper) {
    const char *dl = "0123456789abcdef";
    const char *du = "0123456789ABCDEF";
    const char *d  = upper ? du : dl;

    char tmp[65];
    uint32_t i = 0;

    if (v == 0) tmp[i++] = '0';
    while (v) {
        tmp[i++] = d[v % (uint64_t)base];
        v /= (uint64_t)base;
    }

    for (uint64_t j = 0; j < i; j++)
        out[j] = tmp[i - 1 - j];
    
    return i;
}

static void print_u(uint64_t v, int32_t base, int32_t upper, int32_t width, int32_t zero_pad, uint64_t *written) {
    char buf[65];
    int32_t len = utoa_base(v, buf, base, upper);

    char padc = zero_pad ? '0' : ' ';
    int32_t pad = (width > len) ? (width - len) : 0;
    *written += len + pad;

    krepeat(padc, pad);
    for (uint64_t i = 0; i < len; i++)
        putc(buf[i]);
}

static void print_f(float64_t v, int32_t width, int32_t precision, int32_t zero_pad, uint64_t *written) {
    if (v != v) {
        puts("NaN");
        *written += 3;
        return;
    }

    if (v > 18446744073709551615.0f) {
        puts("Inf");
        *written += 3;
        return;
    }

    if (precision < 0)
        precision = 6;
    if (precision > 9)
        precision = 9;

    int32_t neg = 0;
    if (v < 0.0f) {
        neg = 1;
        v = -v;
    }

    uint64_t scale = 1;
    for (int32_t i = 0; i < precision; ++i)
        scale *= 10ULL;

    uint64_t ip = (uint64_t)v;
    float64_t frac = v - (float64_t)ip;
    uint64_t fp = (uint64_t)(frac * (float64_t)scale + 0.5f);

    if (fp >= scale) {
        ip += 1;
        fp = 0;
    }

    char ibuf[65];
    int32_t ilen = utoa_base(ip, ibuf, 10, 0);
    int32_t total = ilen + neg + (precision > 0 ? (1 + precision) : 0);
    int32_t pad = (width > total) ? (width - total) : 0;
    *written += (uint64_t)(total + pad);
    if (!zero_pad) {
        krepeat(' ', pad);
        if (neg)
            putc('-');
    } else {
        if (neg)
            putc('-');
        krepeat('0', pad);
    }

    for (int32_t i = 0; i < ilen; ++i)
        putc(ibuf[i]);

    if (precision > 0) {
        putc('.');

        char fbuf[32];
        int32_t flen = utoa_base(fp, fbuf, 10, 0);

        for (int32_t i = 0; i < (precision - flen); ++i)
            putc('0');

        for (int32_t i = 0; i < flen; ++i)
            putc(fbuf[i]);
    }
}

static void print_s(int64_t v, int32_t width, int32_t zero_pad, uint64_t *written) {
    uint64_t mag;
    int32_t neg = 0;

    if (v < 0) {
        neg = 1;
        mag = (uint64_t)(-(v + 1)) + 1;
    } else
        mag = (uint64_t)v;
    
    char buf[65];
    int32_t len = utoa_base(mag, buf, 10, 1);

    int32_t total = len + neg;
    int32_t pad = (width > total) ? (width - total) : 0;
    *written += total + pad;

    if (!zero_pad) {
        krepeat(' ', pad);
        if (neg)
            putc('-');
    } else {
        if (neg)
            putc('-');
        krepeat('0', pad);
    }

    for (uint64_t i = 0; i < len; i++)
        putc(buf[i]);
}

uint64_t kvprintf(const char *fmt, va_list ap) {
    int64_t written = 0;

    for (uint64_t i = 0; fmt && fmt[i]; i++) {
        if (fmt[i] != '%') {
            putc(fmt[i]);
            written++;
            continue;
        }

        if (fmt[i + 1] == '%') {
            putc('%');
            written++;
            i++;
            continue;
        }

        int32_t zero_pad = 0;
        int32_t width = 0;
        int32_t precision = -1;
        int32_t long_mod = 0;

        uint64_t j = i + 1;

        if (fmt[j] == '0') {
            zero_pad = 1;
            j++;
        }
        
        while (fmt[j] >= '0' && fmt[j] <= '9') {
            width = width * 10 + (fmt[j] - '0');
            j++;
        }
        
        if (fmt[j] == '.') {
            j++;
            precision = 0;
            while (fmt[j] >= '0' && fmt[j] <= '9') {
                precision = precision * 10 + (fmt[j] - '0');
                j++;
            }
        }

        if (fmt[j] == 'l') {
            long_mod = 1;
            j++;
        }

        char spec = fmt[j];
        if (!spec)
            break;

        switch (spec) {
            case 'c': {
                char c = (char)va_arg(ap, int);
                putc(c);
                written++;
            } break;

            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                int32_t len = strlen(s);
                int32_t pad = (width > len) ? (width - len) : 0;
                krepeat(' ', pad);
                puts(s);
                written += pad + len;
            } break;

            case 'd':
            case 'i': {
                int64_t v = long_mod ? (int64_t)va_arg(ap, long) : (int64_t)va_arg(ap, int);
                print_s(v, width, zero_pad, &written);
            } break;

            case 'u': {
                uint64_t v = long_mod ? (uint64_t)va_arg(ap, unsigned long) : (uint64_t)va_arg(ap, unsigned int);
                print_u(v, 10, 0, width, zero_pad, &written);
            } break;

            case 'x': {
                uint64_t v = long_mod ? (uint64_t)va_arg(ap, unsigned long) : (uint64_t)va_arg(ap, unsigned int);
                puts("0x");
                print_u(v, 16, 1, width, zero_pad, &written);
            } break;

            case 'X': {
                uint64_t v = long_mod ? (uint64_t)va_arg(ap, unsigned long) : (uint64_t)va_arg(ap, unsigned int);
                puts("0x");
                print_u(v, 16, 1, width, zero_pad, &written);
            } break;

            case 'p': {
                void *p = (void *)va_arg(ap, void *);
                puts("0x");
                int32_t ptr_width = (int32_t)(sizeof(void *) * 2);
                print_u((uint64_t)p, 16, 1, ptr_width, 1, &written);
            } break;

            case 'f': {
                double v = va_arg(ap, double);
                print_f(v, width, precision, zero_pad, &written);
            } break;

            default: {
                putc('%');
                putchar(spec, COLOR_WHT, COLOR_BLK);
                written += 2;
            } break;
        }

        i = j;
    }

    return written;
}

uint64_t printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    uint64_t r = kvprintf(fmt, ap);
    va_end(ap);
    return r;
}
