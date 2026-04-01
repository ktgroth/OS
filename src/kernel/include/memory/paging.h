#ifndef __MEMORY_PAGING
#define __MEMORY_PAGING


typedef struct __attribute__((packed)) {
    uint64_t    flags   : 12;
    uint64_t    addr    : 48;
    uint64_t    zs      : 3;
    uint64_t    xd      : 1;
} pml4_t, pdpt_t, pdt_t, pt_t;


void init_memory(void);
void init_paging(memory_map_info_t mmap_info);

pml4_t *get_active_page_table(void);
void set_active_page_table(pml4_t *root);

uint64_t read_cr3(void);
void write_cr3(uint64_t cr3);

void *alloc_physical_page(void);
void free_physical_page(void *page);
pt_t *alloc_page_table(void);

void *get_paddr(void *vaddr);
void map_page(void *paddr, void *vaddr, uint8_t flags);
void unmap_page(void *vaddr);

void *get_paddr_in(pml4_t *root, void *vaddr);
void map_page_in(pml4_t *root, void *paddr, void *vaddr, uint8_t flags);
void unmap_page_in(pml4_t *root, void *vaddr);

#endif

