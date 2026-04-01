#ifndef __MEMORY_ALLOC
#define __MEMORY_ALLOC


typedef struct {
    uint64_t size;
    void    *addr;
} mblock_t;


void init_kalloc(uint64_t heap_base, uint64_t pages);

mblock_t *kalloc(uint64_t);
mblock_t *kcalloc(uint64_t n, uint64_t size);
mblock_t *krealloc(mblock_t *src, uint64_t n);
void kfree(mblock_t *src);

#endif

