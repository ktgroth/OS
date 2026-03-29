
#include "../include/cpu/gdt.h"

#define GDT_ENTRY_COUNT 8

#define GDT_KCODE_SELECTOR      0x08
#define GDT_KDATA_SELECTOR      0x10
#define GDT_U32CODE_SELECTOR    0x18
#define GDT_UDATA_SELECTOR      0x20
#define GDT_U64CODE_SELECTOR    0x28
#define GDT_TSS_SELECTOR        0x30


static uint64_t g_gdt[GDT_ENTRY_COUNT];
static gdtr_t g_gdtr;
static tss64_t g_tss;

static uint8_t g_rsp0_boot_stack[16384] __attribute__((aligned(16)));
static uint8_t g_ist1_stack[16384] __attribute__((aligned(16)));


static uint64_t gdt_make_entry(uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    uint64_t entry = 0;

    entry |= (uint64_t)(limit & 0xFFFFU);
    entry |= (uint64_t)(base & 0xFFFFU);
    entry |= (uint64_t)((base >> 16) & 0xFFU) << 32;
    entry |= (uint64_t)access << 40;
    entry |= (uint64_t)((limit >> 16) & 0x0FU) << 48;
    entry |= (uint64_t)(flags & 0x0FU) << 52;
    entry |= (uint64_t)((base >> 24) & 0xFFU) << 56;

    return entry;
}

static void gdt_set_tss_desc(uint32_t index, uint64_t base, uint32_t limit) {
    uint64_t low = 0;
    uint64_t high = 0;

    low |= (uint64_t)(limit & 0xFFFFU);
    low |= (uint64_t)(base & 0xFFFFU) << 16;
    low |= (uint64_t)((base >> 16) & 0xFFU) << 32;
    low |= (uint64_t)0x89 << 40;
    low |= (uint64_t)((limit >> 16) & 0x0FU) << 48;
    low |= (uint64_t)((base >> 24) & 0xFFU) << 56;

    high |= (uint64_t)((base >> 32) & 0xFFFFFFFFU);

    g_gdt[index] = low;
    g_gdt[index+1] = high;
}

extern void gdt_load(const gdtr_t *gdtr);
extern void tss_load(uint16_t tss_sel);
void init_gdt(void) {

    g_gdt[0] = 0;
    g_gdt[1] = gdt_make_entry(0x00000000, 0xFFFFF, 0x9A, 0x0A);
    g_gdt[2] = gdt_make_entry(0x00000000, 0xFFFFF, 0x92, 0x0C);
    g_gdt[3] = gdt_make_entry(0x00000000, 0xFFFFF, 0xFA, 0x0C);
    g_gdt[4] = gdt_make_entry(0x00000000, 0xFFFFF, 0xF2, 0x0C);
    g_gdt[4] = gdt_make_entry(0x00000000, 0xFFFFF, 0xFA, 0x0A);

    g_tss = (tss64_t){ 0 };
    g_tss.rsp0 = (uint64_t)(g_rsp0_boot_stack + sizeof(g_rsp0_boot_stack));
    g_tss.ist1 = (uint64_t)(g_ist1_stack + sizeof(g_ist1_stack));
    g_tss.iopb_offset = sizeof(tss64_t);
    gdt_set_tss_desc(6, (uint64_t)&g_tss, (uint32_t)(sizeof(tss64_t) - 1));

    g_gdtr.limit = (uint16_t)(sizeof(g_gdt) - 1);
    g_gdtr.base = (uint64_t)g_gdt;

    gdt_load(&g_gdtr);
    tss_load(GDT_TSS_SELECTOR);
}

uint16_t get_kcode(void) {
    return GDT_KCODE_SELECTOR;
}

uint16_t get_kdata(void) {
    return GDT_KDATA_SELECTOR;
}

uint16_t get_u32code(void) {
    return GDT_U32CODE_SELECTOR;
}

uint16_t get_udata(void) {
    return GDT_UDATA_SELECTOR;
}

uint16_t get_u64code(void) {
    return GDT_U64CODE_SELECTOR;
}

uint16_t get_tss(void) {
    return GDT_TSS_SELECTOR;
}

