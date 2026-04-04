
#include "../include/memory/paging.h"
#include "../include/utils/stdlib.h"


#define ENTRIES_PER_TABLE   512ULL
#define PTE_ADDR_MASK       0x000FFFFFFFFFF000ULL
#define PAGE_ALIGN_MASK     (PAGE_SIZE - 1ULL)

#define EFI_LOADER_CODE         1U
#define EFI_LOADER_DATA         2U
#define EFI_BOOT_SERVICES_CODE  3U
#define EFI_BOOT_SERVICES_DATA  4U
#define EFI_CONVENTIONAL_MEMORY 7U


typedef struct __attribute__((packed)) {
    uint32_t type;
    uint32_t pad;
    uint64_t phys_start;
    uint64_t virt_start;
    uint64_t num_pages;
    uint64_t attr;
} efi_memory_desc_t;

static uint8_t  *g_pmm_bitmap = (uint8_t *)0;
static uint64_t g_pmm_bitmap_bytes = 0;
static uint64_t g_pmm_total_pages = 0;
static uint64_t g_pmm_free_pages = 0;
static uint64_t g_pmm_alloc_hint = 1;
static pml4_t   *g_kernel_root = (pml4_t *)0;


static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("hlt");
}

static uint64_t align_up(uint64_t value, uint64_t align) {
    uint64_t mask;
    if (align == 0)
        return value;

    mask = align - 1ULL;
    if (value > (~0ULL - mask))
        return ~0ULL;

    return (value + mask) & ~mask;
}

static uint64_t align_down(uint64_t value, uint64_t align) {
    if (align == 0)
        return value;

    return value & ~(align - 1ULL);
}

static uint64_t range_end(uint64_t start, uint64_t size) {
    if (size == 0)
        return start;

    if (start > (~0ULL - size))
        return ~0ULL;

    return start + size;
}

static uint64_t desc_num_bytes(const efi_memory_desc_t *d) {
    if (!d)
        return 0;

    if (d->num_pages > (~0ULL >> 12))
        return ~0ULL;

    return d->num_pages << 12;
}

static uint64_t mmap_desc_count(const memory_map_info_t *mmap_info) {
    if (!mmap_info || mmap_info->desc_size == 0)
        return 0;

    return mmap_info->size / mmap_info->desc_size;
}

static efi_memory_desc_t *mmap_desc_at(const memory_map_info_t *mmap_info, uint64_t index) {
    uint64_t off = index * mmap_info->desc_size;
    return (efi_memory_desc_t *)((uint8_t *)(uint64_t)mmap_info->ptr + off);
}

static uint8_t is_bootstrap_usable_type(uint32_t type) {
    return (type == EFI_CONVENTIONAL_MEMORY);
}

static uint8_t is_extra_reclaimable_after_switch(uint32_t type) {
    if (type == EFI_LOADER_CODE || type == EFI_LOADER_DATA)
        return 1;

    if (type == EFI_BOOT_SERVICES_CODE || type == EFI_BOOT_SERVICES_DATA)
        return 1;

    return 0;
}

static uint8_t ranges_overlap(uint64_t a_start, uint64_t a_end, uint64_t b_start, uint64_t b_end) {
    return (a_start < b_end) && (b_start < a_end);
}

static uint8_t overlaps_known_ranges(uint64_t start, uint64_t end, const boot_info_t *bi) {
    uint64_t r0;
    uint64_t r1;

    r0 = bi->kernel_base;
    r1 = range_end(r0, bi->kernel_size);
    if (ranges_overlap(start, end, r0, r1))
        return 1;

    r0 = (uint64_t)(uint64_t *)bi;
    r1 = range_end(r0, (uint64_t)sizeof(*bi));
    if (ranges_overlap(start, end, r0, r1))
        return 1;

    r0 = bi->mmap.ptr;
    r1 = range_end(r0, bi->mmap.size);
    if (ranges_overlap(start, end, r0, r1))
        return 1;

    r0 = (uint64_t)(uint64_t *)bi->fb.base;
    r1 = range_end(r0, bi->fb.size);
    if (ranges_overlap(start, end, r0, r1))
        return 1;

    return 0;
}

static uint64_t find_bitmap_phys(const boot_info_t *bi, uint64_t bitmap_bytes) {
    const memory_map_info_t *mmap_info = &bi->mmap;
    uint64_t n = mmap_desc_count(mmap_info);
    uint64_t best = 0;
    uint64_t i;

    for (i = 0; i < n; ++i) {
        efi_memory_desc_t *d = mmap_desc_at(mmap_info, i);
        uint64_t d_bytes;
        uint64_t d_start;
        uint64_t d_end;
        uint64_t candidate;
        uint64_t candidate_end;

        if (!is_bootstrap_usable_type(d->type))
            continue;

        d_bytes = desc_num_bytes(d);
        if (d_bytes < bitmap_bytes)
            continue;

        d_start = align_up(d->phys_start, PAGE_SIZE);
        d_end = align_down(range_end(d->phys_start, d_bytes), PAGE_SIZE);
        if (d_end <= d_start)
            continue;

        if ((d_end - d_start) < bitmap_bytes)
            continue;

        candidate = align_down(d_end - bitmap_bytes, PAGE_SIZE);
        candidate_end = range_end(candidate, bitmap_bytes);
        if (overlaps_known_ranges(candidate, candidate_end, bi))
            continue;

        if (candidate > best)
            best = candidate;
    }

    return best;
}

static uint8_t bitmap_test(uint64_t page_index) {
    return (g_pmm_bitmap[page_index >> 3] >> (page_index & 7ULL)) & 1U;
}
 
static void bitmap_set(uint64_t page_index) {
    g_pmm_bitmap[page_index >> 3] |= (uint8_t)(1U << (page_index & 7ULL));
}

static void bitmap_clear(uint64_t page_index) {
    g_pmm_bitmap[page_index >> 3] &= (uint8_t)~(1U << (page_index & 7ULL));
}

static void pmm_set_page_used(uint64_t page_index) {
    if (page_index >= g_pmm_total_pages)
        return;

    if (!bitmap_test(page_index)) {
        bitmap_set(page_index);
        if (g_pmm_free_pages > 0)
            --g_pmm_free_pages;
    }
}

static void pmm_set_page_free(uint64_t page_index) {
    if (page_index >= g_pmm_total_pages)
        return;

    if (bitmap_test(page_index)) {
        bitmap_clear(page_index);
        ++g_pmm_free_pages;
    }
}

static void pmm_mark_used_range(uint64_t base, uint64_t size) {
    uint64_t first;
    uint64_t last;
    uint64_t i;

    if (size == 0)
        return;

    first = base / PAGE_SIZE;
    last = align_up(range_end(base, size), PAGE_SIZE) / PAGE_SIZE;
    if (last > g_pmm_total_pages)
        last = g_pmm_total_pages;

    for (i = first; i < last; ++i)
        pmm_set_page_used(i);
}

static void pmm_mark_free_range(uint64_t base, uint64_t size) {
    uint64_t first;
    uint64_t last;
    uint64_t i;

    if (size == 0)
        return;

    first = base / PAGE_SIZE;
    last = align_up(range_end(base, size), PAGE_SIZE) / PAGE_SIZE;
    if (last > g_pmm_total_pages)
        last = g_pmm_total_pages;

    for (i = first; i < last; ++i)
        pmm_set_page_free(i);
}

static void reserve_known_ranges(const boot_info_t *bi, uint64_t bitmap_phys, uint64_t bitmap_bytes) {
    pmm_mark_used_range(0, PAGE_SIZE);
    pmm_mark_used_range(bi->kernel_base, bi->kernel_size);
    pmm_mark_used_range((uint64_t)(uint64_t *)bi, (uint64_t)sizeof(*bi));
    pmm_mark_used_range(bi->mmap.ptr, bi->mmap.size);
    pmm_mark_used_range((uint64_t)(uint64_t *)bi->fb.base, bi->fb.size);
    pmm_mark_used_range(bitmap_phys, bitmap_bytes);
}

static uint64_t max_phys_needed_for_pmm(const boot_info_t *bi) {
    const memory_map_info_t *mmap_info = &bi->mmap;
    uint64_t n = mmap_desc_count(mmap_info);
    uint64_t max_end = 0;
    uint64_t i;

    for (i = 0; i < n; ++i) {
        efi_memory_desc_t *d = mmap_desc_at(mmap_info, i);
        uint64_t d_end;

        if (!is_bootstrap_usable_type(d->type) && !is_extra_reclaimable_after_switch(d->type))
            continue;

        d_end = range_end(d->phys_start, desc_num_bytes(d));
        if (d_end > max_end)
            max_end = d_end;
    }

    if (range_end(bi->kernel_base, bi->kernel_size) > max_end)
        max_end = range_end(bi->kernel_base, bi->kernel_size);

    if (range_end((uint64_t)(uint64_t *)bi, (uint64_t)sizeof(*bi)) > max_end)
        max_end = range_end((uint64_t)(uint64_t *)bi, (uint64_t)sizeof(*bi));

    if (range_end(bi->mmap.ptr, bi->mmap.size) > max_end)
        max_end = range_end(bi->mmap.ptr, bi->mmap.size);

    if (range_end((uint64_t)(uint64_t *)bi->fb.base, bi->fb.size) > max_end)
        max_end = range_end((uint64_t)(uint64_t *)bi->fb.base, bi->fb.size);

    return align_up(max_end, PAGE_SIZE);
}

static void reclaim_loader_and_boot_services(const boot_info_t *bi) {
    const memory_map_info_t *mmap_info = &bi->mmap;
    uint64_t n = mmap_desc_count(mmap_info);
    uint64_t i;

    for (i = 0; i < n; ++i) {
        efi_memory_desc_t *d = mmap_desc_at(mmap_info, i);
        if (!is_extra_reclaimable_after_switch(d->type))
            continue;

        pmm_mark_free_range(d->phys_start, desc_num_bytes(d));
    }

    reserve_known_ranges(bi, (uint64_t)(uint64_t *)g_pmm_bitmap, g_pmm_bitmap_bytes);
    pmm_mark_used_range((uint64_t)(uint64_t *)g_kernel_root, PAGE_SIZE);
}

uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ __volatile__("mov %%cr3, %0" : "=r"(v));
    return v;
}

void write_cr3(uint64_t cr3) {
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

pml4_t *get_active_page_table(void) {
    return (pml4_t *)(read_cr3() & PTE_ADDR_MASK);
}

void set_active_page_table(pml4_t *root) {
    write_cr3(((uint64_t)(uint64_t *)root) & PTE_ADDR_MASK);
}

uint64_t pmm_total_pages(void) {
    return g_pmm_total_pages;
}

uint64_t pmm_free_pages(void) {
    return g_pmm_free_pages;
}

void *alloc_physical_page(void) {
    uint64_t i;

    if (!g_pmm_bitmap || g_pmm_total_pages == 0)
        return (void *)0;

    if (g_pmm_alloc_hint < 1 || g_pmm_alloc_hint >= g_pmm_total_pages)
        g_pmm_alloc_hint = 1;

    for (i = g_pmm_alloc_hint; i < g_pmm_total_pages; ++i) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            --g_pmm_free_pages;
            g_pmm_alloc_hint = i + 1;
            return (void *)(i * PAGE_SIZE);
        }
    }

    for (i = 1; i < g_pmm_alloc_hint; ++i) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            --g_pmm_free_pages;
            g_pmm_alloc_hint = i + 1;
            return (void *)(i * PAGE_SIZE);
        }
    }

    return (void *)0;
}

void free_physical_page(void *page) {
    uint64_t phys;
    uint64_t index;

    if (!page)
        return;

    phys = (uint64_t)(uint64_t *)page;
    if ((phys & PAGE_ALIGN_MASK) != 0)
        return;

    index = phys / PAGE_SIZE;
    if (index == 0 || index >= g_pmm_total_pages)
        return;

    if (bitmap_test(index)) {
        bitmap_clear(index);
        ++g_pmm_free_pages;
        if (index < g_pmm_alloc_hint)
            g_pmm_alloc_hint = index;
    }
}

pt_t *alloc_page_table(void) {
    void *page = alloc_physical_page();
    if (!page)
        return (pt_t *)0;

    memset(page, 0, (size_t)PAGE_SIZE);
    return (pt_t *)page;
}

static inline void invlpg(void *vaddr) {
    __asm__ __volatile__("invlpg (%0)" : : "r"(vaddr) : "memory");
}

static uint16_t idx_pml4(uint64_t vaddr) {
    return (uint16_t)((vaddr >> 39) & 0x1FFULL);
}

static uint16_t idx_pdpt(uint64_t vaddr) {
    return (uint16_t)((vaddr >> 30) & 0x1FFULL);
}

static uint16_t idx_pdt(uint64_t vaddr) {
    return (uint16_t)((vaddr >> 21) & 0x1FFULL);
}

static uint16_t idx_pt(uint64_t vaddr) {
    return (uint16_t)((vaddr >> 12) & 0x1FFULL);
}

static uint64_t *table_from_entry(uint64_t entry) {
    return (uint64_t *)(entry & PTE_ADDR_MASK);
}

void *get_paddr_in(pml4_t *root, void *vaddr) {
    uint64_t va = (uint64_t)(uint64_t *)vaddr;
    uint64_t *pml4 = (uint64_t *)root;
    uint64_t *pdpt;
    uint64_t *pdt;
    uint64_t *pt;
    uint64_t entry;

    if (!pml4)
        return (void *)0;

    entry = pml4[idx_pml4(va)];
    if ((entry & PAGE_PRESENT) == 0)
        return (void *)0;

    pdpt = table_from_entry(entry);
    entry = pdpt[idx_pdpt(va)];
    if ((entry & PAGE_PRESENT) == 0)
        return (void *)0;

    pdt = table_from_entry(entry);
    entry = pdt[idx_pdt(va)];
    if ((entry & PAGE_PRESENT) == 0)
        return (void *)0;

    pt = table_from_entry(entry);
    entry = pt[idx_pt(va)];
    if ((entry & PAGE_PRESENT) == 0)
        return (void *)0;

    return (void *)((entry & PTE_ADDR_MASK) | (va & PAGE_ALIGN_MASK));
}

void *get_paddr(void *vaddr) {
    pml4_t *root = g_kernel_root ? g_kernel_root : get_active_page_table();
    return get_paddr_in(root, vaddr);
}

void map_page_in(pml4_t *root, void *paddr, void *vaddr, uint8_t flags) {
    uint64_t va = (uint64_t)(uint64_t *)vaddr;
    uint64_t pa = ((uint64_t)(uint64_t *)paddr) & PTE_ADDR_MASK;
    uint64_t leaf_flags = ((uint64_t)flags & 0x0FFULL) | PAGE_PRESENT;

    uint64_t *pml4 = (uint64_t *)root;
    uint64_t *pdpt;
    uint64_t *pdt;
    uint64_t *pt;

    if (!pml4)
        return;

    if ((pml4[idx_pml4(va)] & PAGE_PRESENT) == 0) {
        uint64_t *new_pdpt = (uint64_t *)alloc_page_table();
        if (!new_pdpt)
            return;

        pml4[idx_pml4(va)] = ((uint64_t)new_pdpt & PTE_ADDR_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    pdpt = table_from_entry(pml4[idx_pml4(va)]);
    if ((pdpt[idx_pdpt(va)] & PAGE_PRESENT) == 0) {
        uint64_t *new_pdt = (uint64_t *)alloc_page_table();
        if (!new_pdt)
            return;

        pdpt[idx_pdpt(va)] = ((uint64_t)new_pdt & PTE_ADDR_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    pdt = table_from_entry(pdpt[idx_pdpt(va)]);
    if ((pdt[idx_pdt(va)] & PAGE_PRESENT) == 0) {
        uint64_t *new_pt = (uint64_t *)alloc_page_table();
        if (!new_pt)
            return;

        pdt[idx_pdt(va)] = ((uint64_t)new_pt & PTE_ADDR_MASK) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    pt = table_from_entry(pdt[idx_pdt(va)]);
    pt[idx_pt(va)] = pa | leaf_flags;

    if (((uint64_t)(uint64_t *)root & PTE_ADDR_MASK) == (read_cr3() & PTE_ADDR_MASK))
        invlpg(vaddr);
}

void map_page(void *paddr, void *vaddr, uint8_t flags) {
    pml4_t *root = g_kernel_root ? g_kernel_root : get_active_page_table();
    map_page_in(root, paddr, vaddr, flags);
}

void unmap_page_in(pml4_t *root, void *vaddr) {
    uint64_t va = (uint64_t)(uint64_t *)vaddr;
    uint64_t *pml4 = (uint64_t *)root;
    uint64_t *pdpt;
    uint64_t *pdt;
    uint64_t *pt;

    if (!pml4)
        return;

    if ((pml4[idx_pml4(va)] & PAGE_PRESENT) == 0)
        return;

    pdpt = table_from_entry(pml4[idx_pml4(va)]);
    if ((pdpt[idx_pdpt(va)] & PAGE_PRESENT) == 0)
        return;

    pdt = table_from_entry(pdpt[idx_pdpt(va)]);
    if ((pdt[idx_pdt(va)] & PAGE_PRESENT) == 0)
        return;

    pt = table_from_entry(pdt[idx_pdt(va)]);
    if ((pt[idx_pt(va)] & PAGE_PRESENT) == 0)
        return;

    pt[idx_pt(va)] = 0;
    if (((uint64_t)(uint64_t *)root & PTE_ADDR_MASK) == (read_cr3() & PTE_ADDR_MASK))
        invlpg(vaddr);
}

void unmap_page(void *vaddr) {
    pml4_t *root = g_kernel_root ? g_kernel_root : get_active_page_table();
    unmap_page_in(root, vaddr);
}

static void map_identity_page(pml4_t *root, uint64_t base, uint64_t size, uint8_t flags) {
    uint64_t start;
    uint64_t end;
    uint64_t addr;

    if (size == 0)
        return;

    start = align_down(base, PAGE_SIZE);
    end = align_up(range_end(base, size), PAGE_SIZE);

    for (addr = start; addr < end; addr += PAGE_SIZE) {
        map_page_in(root, (void *)addr, (void *)addr, flags);
        if (addr > (~0ULL - PAGE_SIZE))
            break;
    }
}

static void map_uefi_ranges_identity(pml4_t *root, const memory_map_info_t *mmap_info) {
    uint64_t n = mmap_desc_count(mmap_info);
    uint64_t i;

    for (i = 0; i < n; ++i) {
        efi_memory_desc_t *d = mmap_desc_at(mmap_info, i);
        uint64_t bytes = desc_num_bytes(d);
        if (bytes == 0)
            continue;

        map_identity_page(root, d->phys_start, bytes, (uint8_t)PAGE_WRITABLE);
    }
}

void init_memory(const boot_info_t *bi) {
    const memory_map_info_t *mmap_info;
    uint64_t max_phys;
    uint64_t raw_bitmap_bytes;
    uint64_t bitmap_bytes;
    uint64_t bitmap_phys;
    uint64_t n;
    uint64_t i;

    if (g_pmm_bitmap)
        return;

    if (!bi || bi->mmap.ptr == 0 || bi->mmap.desc_size == 0)
        halt_forever();

    mmap_info = &bi->mmap;
    max_phys = max_phys_needed_for_pmm(bi);
    if (max_phys < PAGE_SIZE)
        halt_forever();

    g_pmm_total_pages = max_phys / PAGE_SIZE;
    raw_bitmap_bytes = (g_pmm_total_pages + 7ULL) / 8ULL;
    bitmap_bytes = align_up(raw_bitmap_bytes, PAGE_SIZE);

    bitmap_phys = find_bitmap_phys(bi, bitmap_bytes);
    if (bitmap_phys == 0)
        halt_forever();

    g_pmm_bitmap = (uint8_t *)(uint64_t)bitmap_phys;
    g_pmm_bitmap_bytes = bitmap_bytes;
    g_pmm_free_pages = 0;
    g_pmm_alloc_hint = 1;

    memset(g_pmm_bitmap, 0xFF, (size_t)g_pmm_bitmap_bytes);

    n = mmap_desc_count(mmap_info);
    for (i = 0; i < n; ++i) {
        efi_memory_desc_t *d = mmap_desc_at(mmap_info, i);
        if (!is_bootstrap_usable_type(d->type))
            continue;

        pmm_mark_free_range(d->phys_start, desc_num_bytes(d));
    }

    reserve_known_ranges(bi, bitmap_phys, bitmap_bytes);
}

void init_paging(const boot_info_t *bi) {
    pml4_t *new_root;

    if (!bi)
        halt_forever();

    if (!g_pmm_bitmap)
        halt_forever();

    new_root = (pml4_t *)alloc_page_table();
    if (!new_root)
        halt_forever();

    map_uefi_ranges_identity(new_root, &bi->mmap);

    g_kernel_root = new_root;
    set_active_page_table(new_root);

    reclaim_loader_and_boot_services(bi);
    map_identity_page(new_root, (uint64_t)bi, sizeof(*bi),
                      (uint8_t)PAGE_WRITABLE);
    map_identity_page(new_root, (uint64_t)bi->mmap.ptr, bi->mmap.size,
                      (uint8_t)PAGE_WRITABLE);
    map_identity_page(new_root, (uint64_t)bi->fb.base, bi->fb.size,
                      (uint8_t)(PAGE_WRITABLE | PAGE_PCD));

}

