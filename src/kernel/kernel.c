
#include "../boot_info.h"

#include "include/cpu/gdt.h"
#include "include/cpu/idt.h"
#include "include/cpu/isr.h"
#include "include/cpu/irq.h"

#include "include/memory/paging.h"
#include "include/memory/alloc.h"

#include "include/display/framebuffer.h"
#include "include/display/gui.h"
#include "include/display/serial.h"

#include "include/drivers/keyboard.h"


static void halt_forever(void) {
    for (;;)
        __asm__ __volatile__("hlt");
}

static void test_assert(uint8_t ok, const char *name) {
    if (!ok) {
        serial_puts("FAIL: ");
        serial_puts(name);
        serial_puts("\r\n");
        halt_forever();
    }

    serial_puts("PASS: ");
    serial_puts(name);
    serial_puts("\r\n");
}

static void run_memory_tests(void) {
    uint64_t i, j;
    void *pages[128];
    uint64_t free_before, free_after;

    test_assert(pmm_total_pages() > 0, "PMM total pages > 0");
    test_assert(pmm_free_pages() > 0, "PMM free pages > 0");

    free_before = pmm_free_pages();
    for (i = 0; i < 128; ++i) {
        pages[i] = alloc_physical_page();
        test_assert(pages[i] != 0, "alloc_physical_page non-null");
        test_assert((((uint64_t)pages[i]) & 0xFFFULL) == 0, "page alignment");
    }

    for (i = 0; i < 128; ++i)
        for (j = i + 1; j < 128; ++j)
            test_assert(pages[i] != pages[j], "unique physical_page");

    for (i = 0; i < 128; ++i)
        free_physical_page(pages[i]);

    free_after = pmm_free_pages();
    test_assert(free_after == free_before, "PMM free count restored");

    {
        void *pa = alloc_physical_page();
        volatile uint64_t *va = (volatile uint64_t *)0x6000000000ULL;
        test_assert(pa != 0, "paging test alloc pa");

        map_page(pa, (void *)va, (uint8_t)PAGE_WRITABLE);
        test_assert((uint64_t)get_paddr((void *)va) == (uint64_t)pa, "map/get_paddr");

        *va = 0x1122334455667788ULL;
        test_assert(*va == 0x1122334455667788ULL, "mapped write/read");

        unmap_page((void *)va);
        test_assert(get_paddr((void *)va) == 0, "unmap/get_paddr==0");
        free_physical_page(pa);
    }

    {
        mblock_t *a = kalloc(64);
        mblock_t *b;
        mblock_t *c;
        uint8_t *p;
        test_assert(a && a->addr, "kalloc");

        p = (uint8_t *)a->addr;
        for (i = 0; i < 64; ++i)
            p[i] = (uint8_t)(i + 1);

        b = krealloc(a, 4096);
        test_assert(b && b->addr, "krealloc");
        p = (uint8_t *)b->addr;
        for (i = 0; i < 64; ++i)
            test_assert(p[i] == (uint8_t)(i + 1), "krealloc preserves data");

        c = kcalloc(64, 16);
        test_assert(c && c->addr, "kcalloc");
        p = (uint8_t *)c->addr;
        for (i = 0; i < (64 * 16); ++i)
            test_assert(p[i] == 0, "kcalloc zero");

        kfree(b);
        kfree(c);
    }

    serial_puts("ALL MEMORY TESTS PASSES\r\n");
}

int kmain(boot_info_t *bi) {
    if (bi == 0 || bi->fb.base == 0 || bi->fb.width == 0 || bi->fb.height == 0)
        halt_forever();

    serial_puts("INIT MEMORY: ");
    init_memory(bi);
    serial_puts("PASS\r\nINIT PAGING: ");
    init_paging(bi);
    serial_puts("PASS\r\nINIT KALLOC: ");
    init_kalloc(0, 32);
    serial_puts("PASS\r\n");

    serial_puts("INIT GDT: ");
    init_gdt();
    serial_puts("PASS\r\nINIT IDT: ");
    init_idt();
    serial_puts("PASS\r\nINIT ISR: ");
    init_isr();
    serial_puts("PASS\r\nINIT IRQ: ");
    init_irq();
    serial_puts("PASS\r\n");

    serial_puts("INIT FRAMEBUFFER: ");
    init_fb(bi->fb);
    serial_puts("PASS\r\nINIT GUI: ");
    init_gui();
    serial_puts("PASS\r\n");

    serial_puts("INIT KEYBOARD: ");
    irq_register_handler(1, keyboard_callback);
    __asm__ __volatile__("sti");
    serial_puts("PASS\r\n");


    run_memory_tests();

    halt_forever();
    return 0;
}

