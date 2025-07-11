
#include "../include/libc/stdlib.h"
#include "../include/libc/string.h"
#include "../include/libc/memory.h"
#include "../include/driver/vga.h"

#define NULL ((void *)0)

#define PAGE_SIZE 0x1000
#define MAX_PHYS_PAGES 0x20000
#define BITMAP_SIZE (MAX_PHYS_PAGES / 8)

#define BIT_TEST(arr, bit)  ((arr[(bit) / 8] >> ((bit) % 8)) & 1)
#define BIT_SET(arr, bit)   (arr[(bit) / 8] |= (1 << ((bit) % 8)))
#define BIT_CLEAR(arr, bit) (arr[(bit) / 8] &= ~(1 << ((bit) % 8)))

#define VADDR_OFFSET(x) ((x) & 0xFFF)
#define VADDR_PT(x)     (((x) >> 12) & 0x1FF)
#define VADDR_PDT(x)    (((x) >> 21) & 0x1FF)
#define VADDR_PDPT(x)   (((x) >> 30) & 0x1FF)
#define VADDR_PML4(x)   (((x) >> 39) & 0x1FF)

#define PAGE_PRESENT    0x01
#define PAGE_RW         0x02
#define PAGE_USER       0x04
#define PAGE_PWT        0x08
#define PAGE_PCD        0x10
#define PAGE_ACCESSED   0x20
#define PAGE_FLAGS_MASK 0xFFF

#define KMALLOC_ALIGN   0x08

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t ACPI;
} mmap_t;

static uint8_t page_bitmap[BITMAP_SIZE];

static uint64_t heap_start;
static uint64_t heap_end;
static uint64_t heap_current;
static uint64_t phys_base_addr;

extern mmap_t DETECTED_MEMORY;


void init_memory() {
    mmap_t *buffer = (mmap_t *)&DETECTED_MEMORY;
    uint64_t i = 0;
    while (buffer[i].base + buffer[i].length) {
        uint64_t base = buffer[i].base;
        uint64_t length = buffer[i].length;
        uint32_t type = buffer[i].type;
        uint32_t ACPI = buffer[i++].ACPI;
 
        char base_str[66] = "";
        char length_str[66] = "";
        char type_str[66] = "";

        if (type == 1 && phys_base_addr == 0)
            phys_base_addr = base;

        hex_to_ascii(base, base_str);
        hex_to_ascii(length, length_str);
        int_to_ascii(type, type_str);

        putstr("BASE: ", COLOR_WHT, COLOR_BLK);
        putstr(base_str, COLOR_WHT, COLOR_BLK);
        putstr(", LENGTH: ", COLOR_WHT, COLOR_BLK);
        putstr(length_str, COLOR_WHT, COLOR_BLK);
        putstr(", TYPE: ", COLOR_WHT, COLOR_BLK);
        putstr(type_str, COLOR_WHT, COLOR_BLK);
        putstr("\n", COLOR_WHT, COLOR_BLK);
    }
}

void init_page_table() {
    page_table = PAGE_TABLE;
    memset(page_bitmap, 0, BITMAP_SIZE);
}

pt_t *alloc_physical_page() {
    for (uint64_t i = 0; i < MAX_PHYS_PAGES; ++i) {
        if (!BIT_TEST(page_bitmap, i)) {
            BIT_SET(page_bitmap, i);
            uint64_t *paddr = (uint64_t *)(phys_base_addr + (i * PAGE_SIZE));
            return (void *)paddr;
        }
    }

    return NULL;
}

void free_physical_page(void *page) {
    uint64_t addr = (uint64_t)page;
    if (addr < phys_base_addr)
        return;

    uint64_t index = (addr - phys_base_addr) / PAGE_SIZE;
    if (index >= MAX_PHYS_PAGES)
        return;

    BIT_CLEAR(page_bitmap, index);
}

pt_t *alloc_page_table() {
    void *page = alloc_physical_page();
    memset(page, 0, 0x1000);
    return page;
}

void *get_paddr(void *vaddr) {
    uint64_t addr = (uint64_t)vaddr;
    uint64_t pt_idx = VADDR_PT(addr);
    uint64_t pdt_idx = VADDR_PDT(addr);
    uint64_t pdpt_idx = VADDR_PDPT(addr);
    uint64_t pml4_idx = VADDR_PML4(addr);

    pml4_t *pml4 = page_table;
    if (!(pml4[pml4_idx].flags & PAGE_PRESENT)) {
        putstr("PML4E not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pdpt_t *pdpt = (pdpt_t *)(pml4[pml4_idx].addr << 12);
    if (!(pdpt[pdpt_idx].flags & PAGE_PRESENT)) {
        putstr("PDPTE not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pdt_t *pdt = (pdt_t *)(pdpt[pdpt_idx].addr << 12);
    if (!(pdt[pdt_idx].flags & PAGE_PRESENT)) {
        putstr("PDTE not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pt_t *pt = (pt_t *)(pdt[pdt_idx].addr << 12);
    if (!(pt[pt_idx].flags & PAGE_PRESENT)) {
        putstr("Page not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    uint64_t pbase = (pt[pt_idx].addr << 12);
    return (void *)(pbase + VADDR_OFFSET(addr));
}

void map_page(void *paddr, void *vaddr, uint8_t flags) {
    uint64_t addr = (uint64_t)vaddr;
    uint64_t pt_idx = VADDR_PT(addr);
    uint64_t pdt_idx = VADDR_PDT(addr);
    uint64_t pdpt_idx = VADDR_PDPT(addr);
    uint64_t pml4_idx = VADDR_PML4(addr);

    pml4_t *pml4 = page_table;
    if (!(pml4[pml4_idx].flags & PAGE_PRESENT)) {
        pdpt_t *new_pdpt = alloc_page_table();
        pml4[pml4_idx] = (pdpt_t){
            .flags      =flags,
            .addr       =(uint64_t)new_pdpt >> 12,
            .zeros      =0,
            .xd         =0
        };
    }

    pdpt_t *pdpt = (pdpt_t *)(pml4[pml4_idx].addr << 12);
    if (!(pdpt[pdpt_idx].flags & PAGE_PRESENT)) {
        pt_t *new_pdt = alloc_page_table();
        pdpt[pdpt_idx] = (pdt_t){
            .flags      =flags,
            .addr       =(uint64_t)new_pdt >> 12,
            .zeros      =0,
            .xd         =0
        };
    }

    pdt_t *pdt = (pdt_t *)(pdpt[pdpt_idx].addr << 12);
    if (!(pdt[pdt_idx].flags & PAGE_PRESENT)) {
        pt_t *new_pt = alloc_page_table();
        pdt[pdt_idx] = (pt_t){
            .flags      =flags,
            .addr       =(uint64_t)new_pt >> 12,
            .zeros      =0,
            .xd         =0
        };
    }

    pt_t *pt = (pt_t *)(pdt[pdt_idx].addr << 12);
    pt[pt_idx] = (pdpt_t){
            .flags      =flags,
            .addr       =(uint64_t)paddr >> 12,
            .zeros      =0,
            .xd         =0
        };

    __asm__ __volatile__(
        "invlpg (%0)"
        :
        : "r"(addr)
        : "memory"
    );
}

void unmap_page(void *vaddr) {
    uint64_t addr = (uint64_t)vaddr;
    uint64_t pt_idx = VADDR_PT(addr);
    uint64_t pdt_idx = VADDR_PDT(addr);
    uint64_t pdpt_idx = VADDR_PDPT(addr);
    uint64_t pml4_idx = VADDR_PML4(addr);

    pml4_t *pml4 = page_table;
    if (!(pml4[pml4_idx].flags & PAGE_PRESENT))
        return;

    pdpt_t *pdpt = (pdpt_t *)(pml4[pml4_idx].addr << 12);
    if (!(pdpt[pdpt_idx].flags & PAGE_PRESENT))
        return;

    pdt_t *pdt = (pdt_t *)(pdpt[pdpt_idx].addr << 12);
    if (!(pdt[pdt_idx].flags & PAGE_PRESENT))
        return;

    pt_t * pt = (pt_t *)(pdt[pdt_idx].addr << 12);
    pt[pt_idx] = (pt_t){
        .flags  =0,
        .addr   =0,
        .zeros  =0,
        .xd     =0
    };

    __asm__ __volatile__("invlpg (%0)" ::"r"(vaddr) : "memory");
}

void init_kalloc(uint64_t heap_base, uint64_t heap_pages) {
    heap_start = heap_base;
    heap_end = heap_base + heap_pages * PAGE_SIZE;
    heap_current = heap_start;

    for (uint64_t addr = heap_start; addr < heap_end; addr += PAGE_SIZE) {
        void *page = alloc_physical_page();
        map_page(page, (void *)addr, PAGE_PRESENT | PAGE_RW);
    }
}

static inline uint64_t align_up(uint64_t addr, uint64_t align) {
    return (addr + align - 1) & ~(align - 1);
}

void *kmalloc(uint64_t n) {
    if (n == 0)
        return NULL;

    n = align_up(n, KMALLOC_ALIGN);
    uint64_t addr = align_up(heap_current, KMALLOC_ALIGN);

    if (addr + n > heap_end) {
        uint64_t extra_pages = ((addr + n - heap_end) + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint64_t i = 0; i < extra_pages; ++i) {
            void *page = alloc_physical_page();
            map_page(page, (void *)heap_end, PAGE_PRESENT | PAGE_RW);
            heap_end += PAGE_SIZE;
        }
    }

    heap_current = addr + n;
    return (void *)addr;
}

void *kcalloc(uint64_t n, uint64_t size) {
    void *addr = kmalloc(n * size);
    memset(addr, 0, n * size);
    return addr;
}

void *krealloc(void *src, uint64_t n) {
    if (!src)
        return kmalloc(n);

    void *addr = kmalloc(n);
    memcpy(addr, src, n);
    kfree(src);
    return addr;
}

void kfree(void *src) {
    free_physical_page(get_paddr(src));
}

