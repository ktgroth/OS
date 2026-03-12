#ifndef __LIBC_APP
#define __LIBC_APP

#include "../libc/types.h"

#define APP_MAGIC       0x34365041U
#define APP_VERSION     1U
#define APP_FLAG_PIE    0x1ULL

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t entry_off;
    uint64_t image_size;
    uint64_t flags;
} app_header_t;

#endif

