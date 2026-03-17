
#define APP_MAGIC       0x34365041U
#define APP_VERSION     1U
#define APP_FLAG_PIE    0x1ULL

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t entry_off;
    uint64_t image_size;
    uint64_t flags;
} app_header_t;

__attribute__((used, section(".hdr"), aligned(1)))
const app_header_t app_header = {
    .magic = APP_MAGIC,
    .version = APP_VERSION,
    .entry_off = 0,
    .image_size = 0x140,
    .flags = APP_FLAG_PIE
};

typedef unsigned long long uint64_t;

static inline uint64_t sys_write_str(long fd, const void *buf, uint64_t n) {
    uint64_t ret;
    __asm__ __volatile__ (
        "mov $1, %%rax\n\t"
        "int $0x80\n\t"
        : "=a"(ret)
        : "D"(fd), "S"(buf), "d"(n)
        : "memory"
    );
    return ret;
}

static inline __attribute__((noreturn)) void sys_exit(uint64_t code) {
    __asm__ __volatile__ (
        "mov $60, %%rax\n\t"
        "int $0x80\n\t"
        :
        : "D"(code)
        : "rax", "memory"
    );
    __builtin_unreachable();
}

uint64_t _start(void) {
    static const char msg[] = "HELLO WORLD\n";
    sys_write_str(1, msg, sizeof(msg) - 1);
    // sys_exit(0);
    return 0;
}

