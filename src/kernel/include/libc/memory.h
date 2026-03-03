
#ifndef LIBC_MEMORY
#define LIBC_MEMORY

typedef struct {
    uint64_t flags      : 12;
    uint64_t addr       : 40;
    uint64_t zeros      : 11;
    uint64_t xd         : 1;
} pml4_t, pdpt_t, pdt_t, pt_t;

typedef struct mblock {
    uint64_t size;
    void     *addr;
} mblock_t;

extern pml4_t PAGE_TABLE[];
static pml4_t *page_table;

void init_memory();
void init_page_table();

void *get_paddr(void *vaddr);
void map_page(void *paddr, void *vaddr, uint8_t flags);
void unmap_page(void *vaddr);

void init_kalloc(uint64_t heap_base, uint64_t pages);

mblock_t kmalloc(uint64_t n);
mblock_t kcalloc(uint64_t n, uint64_t size);
mblock_t krealloc(mblock_t src, uint64_t n);
void kfree(mblock_t src);

#endif

