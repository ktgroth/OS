
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

#define PAGE_PRESENT    0x01
#define PAGE_RW         0x02
#define PAGE_USER       0x04
#define PAGE_PWT        0x08
#define PAGE_PCD        0x10
#define PAGE_ACCESSED   0x20
#define PAGE_FLAGS_MASK 0xFFF

#define KMALLOC_ALIGN   0x08

typedef struct {
    uint64_t offset     : 12;
    uint64_t pt         : 9;
    uint64_t pdt        : 9;
    uint64_t pdpt       : 9;
    uint64_t pml4       : 9;
    uint64_t sign       : 16;
} vaddr_t;

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
        char length_str[32] = "";
        char type_str[2] = "";

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
    page_table = &PAGE_TABLE;
    memset(page_bitmap, 0, BITMAP_SIZE);
}

pt_t *alloc_physical_page() {
    for (uint64_t i = 0; i < MAX_PHYS_PAGES; ++i) {
        if (!BIT_TEST(page_bitmap, i)) {
            BIT_SET(page_bitmap, i);
            uint64_t *paddr = (uint64_t *)(heap_current + (i * PAGE_SIZE));
            return (void *)paddr;
        }
    }

    return NULL;
}

void free_physical_page(void *page) {
    uint64_t addr = (uint64_t)page;
    if (addr < heap_current)
        return;

    uint64_t index = ((uint64_t)addr - (uint64_t)heap_start) / PAGE_SIZE;
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
    vaddr_t addr = *(vaddr_t *)&vaddr;

    pml4_t *pml4 = page_table;
    if (!pml4[addr.pml4].present) {
        putstr("PML4E not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pdpt_t *pdpt = (pdpt_t *)(pml4[addr.pml4].addr << 12);
    if (!pdpt[addr.pdpt].present) {
        putstr("PDPTE not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pdt_t *pdt = (pdt_t *)(pdpt[addr.pdpt].addr << 12);
    if (!pdt[addr.pdt].present) {
        putstr("PDTE not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    pt_t *pt = (pt_t *)(pdt[addr.pdt].addr << 12);
    if (!pt[addr.pt].present) {
        putstr("Page not present\n", COLOR_WHT, COLOR_BLK);
        return NULL;
    }

    uint64_t pbase = (uint64_t)(pt[addr.pt].addr << 12);
    return (void *)(pbase + addr.offset);
}

void map_page(void *paddr, void *vaddr, uint8_t flags) {
    vaddr_t addr = *(vaddr_t *)&vaddr;

    pml4_t *pml4 = page_table;
    if (!pml4[addr.pml4].present) {
        pml4_t *new_pdpt = alloc_page_table();
        pml4[addr.pml4] = (pdt_t){
            .present = flags & PAGE_PRESENT,
            .rw = flags & PAGE_RW,
            .user = flags & PAGE_USER,
            .pwt = flags & PAGE_PWT,
            .pcd = flags & PAGE_PCD,
            .accessed = flags & PAGE_ACCESSED,
            .ignored1 = 0,
            .page_size = 0,
            .ignored2 = 0,
            .addr = (uint64_t)new_pdpt >> 12,
            .zeros = 0,
            .nx = 0
        };
    }

    pdpt_t *pdpt = (pdpt_t *)((uint64_t)pml4[addr.pdpt].addr << 12);
    if (!pdpt[addr.pdpt].present) {
        pdpt_t *new_pdt = alloc_page_table();
        pdpt[addr.pdpt] = (pdt_t){
            .present = flags & PAGE_PRESENT,
            .rw = flags & PAGE_RW,
            .user = flags & PAGE_USER,
            .pwt = flags & PAGE_PWT,
            .pcd = flags & PAGE_PCD,
            .accessed = flags & PAGE_ACCESSED,
            .ignored1 = 0,
            .page_size = 0,
            .ignored2 = 0,
            .addr = (uint64_t)new_pdt >> 12,
            .zeros = 0,
            .nx = 0
        };
    }

    pdt_t *pdt = (pdt_t *)((uint64_t)pdpt[addr.pdt].addr << 12);
    if (!pdt[addr.pdt].present) {
        pdt_t *new_pt = alloc_page_table();
        pdt[addr.pdt] = (pdt_t){
            .present = flags & PAGE_PRESENT,
            .rw = flags & PAGE_RW,
            .user = flags & PAGE_USER,
            .pwt = flags & PAGE_PWT,
            .pcd = flags & PAGE_PCD,
            .accessed = flags & PAGE_ACCESSED,
            .ignored1 = 0,
            .page_size = 0,
            .ignored2 = 0,
            .addr = (uint64_t)new_pt >> 12,
            .zeros = 0,
            .nx = 0
        };
    }

    pt_t *pt = (pt_t *)((uint64_t)pdt[addr.pdt].addr << 12);
    pt[addr.pt] = (pt_t){
        .present = flags & PAGE_PRESENT,
        .rw = flags & PAGE_RW,
        .user = flags & PAGE_USER,
        .pwt = flags & PAGE_PWT,
        .pcd = flags & PAGE_PCD,
        .accessed = flags & PAGE_ACCESSED,
        .ignored1 = 0,
        .page_size = 0,
        .ignored2 = 0,
        .addr = (uint64_t)paddr >> 12,
        .zeros = 0,
        .nx = 0
    };
}

void unmap_page(void *vaddr) {
    vaddr_t addr = *(vaddr_t *)&vaddr;

    pml4_t *pml4 = page_table;
    if (!pml4[addr.pml4].present)
        return;

    pdpt_t *pdpt = (pdpt_t *)(pml4[addr.pml4].addr << 12);
    if (!pdpt[addr.pdpt].present)
        return;

    pdt_t *pdt = (pdt_t *)(pdpt[addr.pdpt].addr << 12);
    if (!pdt[addr.pdt].present)
        return;

    pt_t * pt = (pt_t *)(pdt[addr.pdt].addr << 12);
    pt[addr.pt] = (pt_t){
        .present=0,
        .rw=0,
        .user=0,
        .pwt=0,
        .pcd=0,
        .accessed=0,
        .ignored1=0,
        .page_size=0,
        .ignored2=0,
        .addr=0,
        .zeros=0,
        .nx=0
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
    void *addr = kmalloc(n);
    memcpy(src, addr, n);
    kfree(src);
    return addr;
}

void kfree(void *src) {
    free_physical_page(src);
}

