
#include "../include/memory/alloc.h"
#include "../include/memory/paging.h"
#include "../include/utils/stdlib.h"


#define KHEAP_DEFAULT_BASE  0x0000004000000000ULL
#define KHEAP_ALIGN         16ULL


typedef struct kalloc_header {
    uint64_t size;
    void *addr;

    struct kalloc_header *next;
    uint64_t capacity;
    uint8_t free;
} kalloc_header_t;


static kalloc_header_t *g_heap_head = (kalloc_header_t *)0;
static uint64_t g_heap_next_va = KHEAP_DEFAULT_BASE;


static uint64_t align_up_u64(uint64_t value, uint64_t align) {
    uint64_t mask;

    if (align == 0)
        return value;

    mask = align - 1ULL;
    if (value > (~0ULL - mask))
        return ~0ULL;

    return (value + mask) & ~mask;
}

static void append_block(kalloc_header_t *block) {
    kalloc_header_t *tail;

    if (!g_heap_head) {
        g_heap_head = block;
        return;
    }

    tail = g_heap_head;
    while (tail->next)
        tail = tail->next;

    tail->next = block;
}

static uint8_t map_heap_pages(uint64_t vbase, uint64_t pages) {
    uint64_t i;

    for (i = 0; i < pages; ++i) {
        uint64_t va = vbase + (i * PAGE_SIZE);
        void *pa = alloc_physical_page();
        if (!pa)
            break;

        map_page(pa, (void *)va, (uint8_t)PAGE_WRITABLE);
        if (!get_paddr((void *)va))
            break;
    }

    if (i == pages)
        return 1;

    while (i > 0) {
        uint64_t va;
        void *pa;

        --i;
        va = vbase + (i * PAGE_SIZE);
        pa = get_paddr((void *)va);
        if (pa) {
            unmap_page((void *)va);
            free_physical_page((void *)((uint64_t)(uint64_t *)pa & ~(PAGE_SIZE - 1ULL)));
        }
    }

    return 0;
}

static kalloc_header_t *find_free_block(uint64_t needed) {
    kalloc_header_t *it = g_heap_head;

    while (it) {
        if (it->free && it->capacity >= needed)
            return it;

        it = it->next;
    }

    return (kalloc_header_t *)0;
}

static kalloc_header_t *create_new_block(uint64_t needed) {
    uint64_t total = needed + (uint64_t)sizeof(kalloc_header_t);
    uint64_t pages = align_up_u64(total, PAGE_SIZE) / PAGE_SIZE;
    uint64_t base = g_heap_next_va;
    kalloc_header_t *hdr;

    if (pages == 0)
        return (kalloc_header_t *)0;

    if (!map_heap_pages(base, pages))
        return (kalloc_header_t *)0;

    g_heap_next_va += pages * PAGE_SIZE;

    hdr = (kalloc_header_t *)base;
    hdr->size = needed;
    hdr->addr = (void *)(base + (uint64_t)sizeof(kalloc_header_t));
    hdr->next = (kalloc_header_t *)0;
    hdr->capacity = (pages * PAGE_SIZE) - (uint64_t)sizeof(kalloc_header_t);
    hdr->free = 0;

    append_block(hdr);
    return hdr;
}

void init_kalloc(uint64_t heap_base, uint64_t pages) {
    uint64_t base;
    kalloc_header_t *hdr;

    g_heap_head = (kalloc_header_t *)0;
    g_heap_next_va = (heap_base != 0) ? align_up_u64(heap_base, PAGE_SIZE) : KHEAP_DEFAULT_BASE;

    if (pages == 0)
        return;

    base = g_heap_next_va;
    if (!map_heap_pages(base, pages))
        return;

    g_heap_next_va += pages * PAGE_SIZE;

    hdr = (kalloc_header_t *)base;
    hdr->size = 0;
    hdr->addr = (void *)(base + (uint64_t)sizeof(kalloc_header_t));
    hdr->next = (kalloc_header_t *)0;
    hdr->capacity = (pages * PAGE_SIZE) - (uint64_t)sizeof(kalloc_header_t);
    hdr->free = 1;

    g_heap_head = hdr;
}

mblock_t *kalloc(uint64_t size) {
    uint64_t needed;
    kalloc_header_t *hdr;

    if (size == 0)
        return (mblock_t *)0;

    needed = align_up_u64(size, KHEAP_ALIGN);

    hdr = find_free_block(needed);
    if (!hdr)
        hdr = create_new_block(needed);

    if (!hdr)
        return (mblock_t *)0;

    hdr->size = needed;
    hdr->free = 0;
    return (mblock_t *)hdr;
}

mblock_t *kcalloc(uint64_t n, uint64_t size) {
    uint64_t total;
    mblock_t *blk;

    if (n == 0 || size == 0)
        return (mblock_t *)0;

    if (size > (~0ULL / n))
        return (mblock_t *)0;

    total = n * size;
    blk = kalloc(total);
    if (!blk)
        return (mblock_t *)0;

    memset(blk->addr, 0, (size_t)total);
    return blk;
}

mblock_t *krealloc(mblock_t *src, uint64_t n) {
    kalloc_header_t *old_hdr;
    uint64_t needed;
    mblock_t *new_blk;
    uint64_t to_copy;

    if (!src)
        return kalloc(n);

    if (n == 0) {
        kfree(src);
        return (mblock_t *)0;
    }

    needed = align_up_u64(n, KHEAP_ALIGN);
    old_hdr = (kalloc_header_t *)src;
    if (needed <= old_hdr->capacity) {
        old_hdr->size = needed;
        old_hdr->free = 0;
        return src;
    }

    new_blk = kalloc(needed);
    if (!new_blk)
        return (mblock_t *)0;

    to_copy = old_hdr->size;
    if (to_copy > needed)
        to_copy = needed;

    memcpy(new_blk->addr, src->addr, to_copy);
    kfree(src);
    return new_blk;
}

void kfree(mblock_t *src) {
    kalloc_header_t *hdr;

    if (!src)
        return;

    hdr = (kalloc_header_t *)src;
    hdr->free = 1;
}

