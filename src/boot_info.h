#ifndef __BOOT_INFO
#define __BOOT_INFO

#include "types.h"

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
    uint32_t pixel_format;
} framebuffer_info_t;

typedef struct {
    uint64_t ptr;
    uint64_t size;
    uint64_t desc_size;
    uint64_t desc_version;
} memory_map_info_t;

typedef struct {
    uint64_t magic;
    uint32_t version;
    uint32_t reserved;

    framebuffer_info_t fb;
    memory_map_info_t  mmap;

    uint64_t rsdp;
    uint64_t kernel_entry;
    uint64_t kernel_base;
    uint64_t kernel_size;
} boot_info_t;

#endif

