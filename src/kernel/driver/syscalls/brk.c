
#include "../../include/libc/memory.h"
#include "../../include/driver/syscalls/brk.h"
#include "../../include/user_mode/process.h"

#define PAGE_SIZE       0x1000ULL
#define PAGE_PRESENT    0x01
#define PAGE_RW         0x02
#define PAGE_USER       0x04

static inline uint64_t align_up(uint64_t addr, uint64_t align) {
    return (addr + align - 1) & ~(align - 1);
}

uint64_t sys_brk(uint64_t new_end) {
    // process_t *p = current_process();
    // if (!p)
    //     return -1;
    //
    // uint64_t old_end = (uint64_t)p->brk_end;
    //
    // if (new_end == 0)
    //     return old_end;
    //
    // if (new_end < (uint64_t)p->brk_start || new_end > p->brk_limit)
    //     return old_end;
    //
    // uint64_t old_map_end = align_up(old_end, PAGE_SIZE);
    // uint64_t new_map_end = align_up(new_end, PAGE_SIZE);
    //
    // if (new_map_end > old_map_end) {
    //     uint64_t mapped = 0;
    //     for (uint64_t va = old_map_end; va < new_map_end; va += PAGE_SIZE) {
    //         void *pa = alloc_physical_page();
    //         if (!pa) {
    //             for (uint64_t rva = old_map_end; mapped--; rva += PAGE_SIZE) {
    //                 void *rpa = get_paddr_in(p->image_base, (void *)rva);
    //                 if (rpa)
    //                     free_physical_page((void *)((uint64_t)rpa & ~0xFFFULL));
    //
    //                 unmap_page_in(p->image_base, (void *)rva);
    //             }
    //
    //             return old_end;
    //         }
    //
    //         map_page_in(p->image_base, pa, (void *)va, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    //         ++mapped;
    //     }
    // } else if (new_map_end < old_map_end) {
    //     for (uint64_t va = new_map_end; va < old_map_end; va += PAGE_SIZE) {
    //         void *pa = get_paddr_in(p->image_base, (void *)va);
    //         if (pa)
    //             free_physical_page((void *)((uint64_t)pa & ~0xFFFULL));
    //
    //         unmap_page_in(p->image_base, (void *)va);
    //     }
    // }
    //
    // p->brk_end = (uint64_t)new_end;
    // return new_end;
    //
    return 1;
}

