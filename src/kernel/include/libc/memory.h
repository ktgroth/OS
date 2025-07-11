
#ifndef LIBC_MEMORY
#define LIBC_MEMORY

typedef struct {
    uint64_t flags      : 12;
    uint64_t addr       : 40;
    uint64_t zeros      : 11;
    uint64_t xd         : 1;
} pml4_t, pdpt_t, pdt_t, pt_t;

extern pml4_t PAGE_TABLE[];
static pml4_t *page_table;

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

