
#ifndef LIBC_MEMORY
#define LIBC_MEMORY

typedef struct __attribute__((packed)) {
    uint8_t present     : 1;
    uint8_t rw          : 1;
    uint8_t user        : 1;
    uint8_t pwt         : 1;
    uint8_t pcd         : 1;
    uint8_t accessed    : 1;
    uint8_t ignored1    : 1;
    uint8_t page_size   : 1;
    uint64_t ignored2   : 4;
    uint64_t addr       : 48;
    uint64_t zeros      : 3;
    uint64_t nx         : 1;
} pml4_t, pdpt_t, pdt_t, pt_t, page_t;

typedef struct __attribute__((packed)) {
    
} block_t;


extern page_t PAGE_TABLE;
static page_t *page_table;

void init_memory();
void init_page_table();

void *get_paddr(void *vaddr);
void map_page(void *paddr, void *vaddr, uint8_t flags);
void unmap_page(void *vaddr);

void init_kalloc(uint64_t heap_base, uint64_t pages);

void *kmalloc(uint64_t n);
void *kcalloc(uint64_t n, uint64_t size);
void *krealloc(void *src, uint64_t n);
void kfree(void *src);

#endif

