#ifndef __MEMORY_PAGING
#define __MEMORY_PAGING


#include "../../../boot_info.h"
#include "../../../types.h"


#define PAGE_SIZE       0x1000ULL
#define PAGE_PRESENT    0x001ULL
#define PAGE_WRITABLE   0x002ULL
#define PAGE_USER       0x004ULL
#define PAGE_PWT        0x008ULL
#define PAGE_PCD        0x010ULL
#define PAGE_GLOBAL     0x100ULL


typedef uint64_t pml4_t;
typedef uint64_t pdpt_t;
typedef uint64_t pdt_t;
typedef uint64_t pt_t;


void init_memory(const boot_info_t *bi);
void init_paging(const boot_info_t *bi);

pml4_t *get_active_page_table(void);
void set_active_page_table(pml4_t *root);

uint64_t read_cr3(void);
void write_cr3(uint64_t cr3);

void *alloc_physical_page(void);
void free_physical_page(void *page);
pt_t *alloc_page_table(void);

uint64_t pmm_total_pages(void);
uint64_t pmm_free_pages(void);

void *get_paddr(void *vaddr);
void map_page(void *paddr, void *vaddr, uint8_t flags);
void unmap_page(void *vaddr);

void *get_paddr_in(pml4_t *root, void *vaddr);
void map_page_in(pml4_t *root, void *paddr, void *vaddr, uint8_t flags);
void unmap_page_in(pml4_t *root, void *vaddr);

#endif

