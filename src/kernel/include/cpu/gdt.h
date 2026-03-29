#ifndef __CPU_GDT
#define __CPU_GDT

#include "../../../types.h"

// typedef struct __attribute__((packed)) {
//     uint16_t    limit_lo;
//     uint16_t    base_lo;
//     uint8_t     base_mid;
//     uint8_t     a           : 1;
//     uint8_t     rw          : 1;
//     uint8_t     dc          : 1;
//     uint8_t     e           : 1;
//     uint8_t     s           : 1;
//     uint8_t     dpl         : 2;
//     uint8_t     p           : 1;
//     uint8_t     limit_hi    : 4;
//     uint8_t     _reserved   : 1;
//     uint8_t     l           : 1;
//     uint8_t     db          : 1;
//     uint8_t     g           : 1;
//     uint8_t     base_hi;
// } seg_desc_t;
//
// typedef struct __attribute__((packed)) {
//     uint16_t    limit_lo;
//     uint16_t    base_lo;
//     uint8_t     base_mid1;
//     uint8_t     type        : 4;
//     uint8_t     s           : 1;
//     uint8_t     dpl         : 2;
//     uint8_t     p           : 1;
//     uint8_t     limit_hi    : 4;
//     uint8_t     _reserved2  : 1;
//     uint8_t     l           : 1;
//     uint8_t     db          : 1;
//     uint8_t     g           : 1;
//     uint8_t     base_mid2;
//     uint32_t    base_hi;
//     uint32_t    _reserved1;
// } tss_desc_t, ldt_desc_t;
//
// struct GDT {
//     seg_desc_t  null;
//     seg_desc_t  kcode;
//     seg_desc_t  kdata;
//     seg_desc_t  ucode;
//     seg_desc_t  udata;
//     tss_desc_t  tss;
//     ldt_desc_t  ldt;
// };

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} gdtr_t;

typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0, rsp1, rsp2;
    uint64_t reserved1;
    uint64_t ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    uint64_t reserved2;
    uint64_t reserved3;
    uint16_t iopb_offset;
} tss64_t;

void init_gdt(void);
void set_tss_rsp0(uint64_t rsp0);

uint16_t get_kcode(void);
uint16_t get_kdata(void);
uint16_t get_u32code(void);
uint16_t get_udata(void);
uint16_t get_u64code(void);
uint16_t get_tss(void);

#endif

