
#include "../include/cpu/apic.h"
#include "../include/libc/memory.h"
#include "../include/libc/string.h"
#include "../include/libc/printf.h"


static const char SIG_RSDP[8] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };
static const char SIG_RSDT[4] = { 'R', 'S', 'D', 'T' };
static const char SIG_XSDT[4] = { 'X', 'S', 'D', 'T' };
static const char SIG_MADT[4] = { 'A', 'P', 'I', 'C' };

#define IDMAP_LIMIT         0x100000000ULL
#define ACPI_MAX_TABLE_LEN  0x100000ULL

#define IA32_APIC_BASE_MSR  0x1B
#define IA32_APIC_BASE_BSP  (1ULL << 8)
#define IA32_APIC_BASE_EN   (1ULL << 11)
#define IA32_APIC_BASE_MASK 0xFFFFFFFFFFFFF000ULL

#define MADT_TYPE_LOCAL_APIC                0
#define MADT_TYPE_IO_APIC                   1
#define MADT_TYPE_INTERRUPT_OVERRIDE        2
#define MADT_TYPE_LOCAL_APIC_NMI            4
#define MADT_TYPE_LOCAL_APIC_ADDR_OVERRIDE  5
#define MADT_TYPE_LOCAL_X2APIC              9

#define PAGE_PRESENT 0x01
#define PAGE_RW      0x02
#define PAGE_PCD     0x10

#define LAPIC_PHYS  0xFEE00000ULL
#define IOAPIC_PHYS 0xFEC00000ULL

#define LAPIC_ID_REG    0x20
#define LAPIC_EOI_REG   0xB0
#define LAPIC_SVR_REG   0xF0

#define IOREGSEL        0x00
#define IOWIN           0x10


typedef struct __attribute__((packed)) {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} rsdp_t;

typedef struct __attribute__((packed)) {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} acpi_sdt_header_t;

typedef struct __attribute__((packed)) {
    acpi_sdt_header_t h;
    uint32_t lapic_addr;
    uint32_t flags;
    uint8_t entries[];
} madt_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
} madt_entry_header_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} madt_local_apic_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
    uint16_t reserved;
    uint64_t lapic_phys_addr;
} madt_lapic_addr_override_t;

typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t length;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_uid;
} madt_local_x2apic_t;


static apic_info_t g_apic_info;

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ __volatile__(
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );
    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t v) {
    uint32_t lo = (uint32_t)v;
    uint32_t hi = (uint32_t)(v >> 32);
    __asm__ __volatile__(
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
    );
}

static inline uint32_t lapic_read(uint64_t base, uint32_t reg) {
    return *(__volatile__ uint32_t *)((uint8_t *)base + reg);
}

static inline void lapic_write(uint64_t base, uint32_t reg, uint32_t v) {
    *(__volatile__ uint32_t *)((uint8_t *)base + reg);
    (void)lapic_read(base, LAPIC_ID_REG);
}

static inline uint32_t ioapic_read(uint64_t base, uint8_t reg) {
    *(__volatile__ uint32_t *)((uint8_t *)base + IOREGSEL) = reg;
    return *(__volatile__ uint32_t *)((uint8_t *)base + IOWIN);
}

static inline void ioapic_write(uint64_t base, uint8_t reg, uint32_t v) {
    *(__volatile__ uint32_t *)((uint8_t *)base + IOREGSEL) = reg;
    *(__volatile__ uint32_t *)((uint8_t *)base + IOWIN) = v;
}

static uint64_t apic_base_from_msr(void) {
    uint64_t v = rdmsr(IA32_APIC_BASE_MSR);
    if ((v & IA32_APIC_BASE_EN) == 0)
        return 0;

    return v & IA32_APIC_BASE_MASK;
}

static uint64_t memeq(const void *a, const void *b, uint64_t n) {
    const uint8_t *x = (const uint8_t *)a;
    const uint8_t *y = (const uint8_t *)b;
    for (uint64_t i = 0; i < n; ++i)
        if (x[i] != y[i])
            return 0;

    return 1;
}

static uint8_t acpi_checksum8(const void *ptr, uint64_t len) {
    const uint8_t *p = (const uint8_t *)ptr;
    uint8_t sum = 0;
    for (uint64_t i = 0; i < len; ++i)
        sum = (uint8_t)(sum + p[i]);

    return sum;
}

static void map_phys_range_identity(uint64_t base, uint64_t len) {
    uint64_t start = base & ~0xFFFULL;
    uint64_t end = (base + len + 0xFFFULL) & ~0xFFFULL;

    for (uint64_t p = start; p < end; p += 0x1000)
        map_page((void *)p, (void *)p, PAGE_PRESENT | PAGE_RW | PAGE_PCD);
}

static rsdp_t *scan_rsdp_range(uint64_t start, uint64_t end) {
    for (uint64_t p = start; p + sizeof(rsdp_t) <= end; p += 16) {
        rsdp_t *r = (rsdp_t *)p;
        if (!memeq(r->signature, SIG_RSDP, 8))
            continue;

        if (acpi_checksum8(r, 20) != 0)
            continue;

        if (r->revision >= 2) {
            if (r->length < sizeof(rsdp_t))
                continue;
            if (acpi_checksum8(r, r->length) != 0)
                continue;
        }

        return r;
    }

    return NULL;
}

static rsdp_t *find_rsdp(void) {
    uint16_t ebda_seg = *(__volatile__ uint16_t *)0x40E;
    uint64_t ebda_base = ((uint64_t)ebda_seg) << 4;

    if (ebda_base >= 0x80000 && ebda_base < 0xA0000) {
        rsdp_t *r = scan_rsdp_range(ebda_base, ebda_base + 1024);
        if (r)
            return r;
    }

    return scan_rsdp_range(0xE0000, 0x100000);
}

static uint64_t sdt_valid(acpi_sdt_header_t *h) {
    if (!h)
        return 0;

    map_phys_range_identity((uint64_t)h, sizeof(acpi_sdt_header_t));
    if (h->length < sizeof(acpi_sdt_header_t))
        return 0;

    map_phys_range_identity((uint64_t)h, h->length);
    return acpi_checksum8(h, h->length) == 0;
}

static acpi_sdt_header_t *find_table_in_rsdt(acpi_sdt_header_t *rsdt, const char sig[4]) {
    uint32_t n = (rsdt->length - sizeof(acpi_sdt_header_t)) / 4;
    uint32_t *entries = (uint32_t *)((uint8_t *)rsdt + sizeof(acpi_sdt_header_t));

    for (uint32_t i = 0; i < n; ++i) {
        acpi_sdt_header_t *h = (acpi_sdt_header_t *)(uint64_t)entries[i];
        if (!sdt_valid(h))
            continue;

        if (memeq(h->signature, sig, 4))
            return h;
    }

    return NULL;
}

static acpi_sdt_header_t *find_table_in_xsdt(acpi_sdt_header_t *xsdt, const char sig[4]) {
    uint32_t n = (xsdt->length - sizeof(acpi_sdt_header_t)) / 8;
    uint64_t *entries = (uint64_t *)((uint8_t *)xsdt + sizeof(acpi_sdt_header_t));

    for (uint32_t i = 0; i < n; ++i) {
        acpi_sdt_header_t *h = (acpi_sdt_header_t *)(uint64_t)entries[i];
        if (!sdt_valid(h))
            continue;

        if (memeq(h->signature, sig, 4))
            return h;
    }

    return NULL;
}

static madt_t *find_madt(rsdp_t *rsdp) {
    if (!rsdp)
        return NULL;

    if (rsdp->revision >= 2 && rsdp->xsdt_address) {
        acpi_sdt_header_t *xsdt = (acpi_sdt_header_t *)(uint64_t)rsdp->xsdt_address;
        if (sdt_valid(xsdt) && memeq(xsdt->signature, SIG_XSDT, 4))
            return (madt_t *)find_table_in_xsdt(xsdt, SIG_MADT);
    }

    if (rsdp->rsdt_address) {
        acpi_sdt_header_t *rsdt = (acpi_sdt_header_t *)(uint64_t)rsdp->rsdt_address;
        if (sdt_valid(rsdt) && memeq(rsdt->signature, SIG_RSDT, 4))
            return (madt_t *)find_table_in_rsdt(rsdt, SIG_MADT);
    }

    return NULL;
}

static void parse_madt(madt_t *madt, apic_info_t *out) {
    out->lapic_phys_addr = madt->lapic_addr;
    out->madt_flags = madt->flags;
    out->cpu_count = 0;

    uint8_t *p = madt->entries;
    uint8_t *end = ((uint8_t *)madt) + madt->h.length;

    while (p + sizeof(madt_entry_header_t) <= end) {
        madt_entry_header_t *eh = (madt_entry_header_t *)p;
        if (eh->length < sizeof(madt_entry_header_t))
            break;

        if (p + eh->length > end)
            break;

        if (eh->type == MADT_TYPE_LOCAL_APIC && eh->length >= sizeof(madt_local_apic_t)) {
            madt_local_apic_t *e = (madt_local_apic_t *)p;
            if (out->cpu_count < APIC_MAX_CPUS) {
                out->cpus[out->cpu_count].acpi_cpu_id = e->acpi_processor_id;
                out->cpus[out->cpu_count].apic_id = e->apic_id;
                out->cpus[out->cpu_count].flags = e->flags;
                out->cpu_count++;
            }

            // printf("MADT LOCAL APIC\n");
        } else if (eh->type == MADT_TYPE_LOCAL_APIC_ADDR_OVERRIDE &&
                   eh->length >= sizeof(madt_lapic_addr_override_t)) {
            madt_lapic_addr_override_t *e = (madt_lapic_addr_override_t *)p;
            out->lapic_phys_addr = e->lapic_phys_addr;
            // printf("MADT LOCAL APIC ADDR OVERRIDE\n");
        } else if (eh->type == MADT_TYPE_LOCAL_X2APIC &&
                   eh->length >= sizeof(madt_local_x2apic_t)) {
            madt_local_x2apic_t *e = (madt_local_x2apic_t *)p;

            if (out->cpu_count < APIC_MAX_CPUS) {
                out->cpus[out->cpu_count].acpi_cpu_id = (uint8_t)e->acpi_uid;
                out->cpus[out->cpu_count].apic_id = (uint8_t)e->x2apic_id;
                out->cpus[out->cpu_count].flags = e->flags;
                out->cpu_count++;
            }
            
            // printf("MADT LOCAL X2APIC\n");
        }
        
        p += eh->length;
    }
}

void apic_enable_lapic(void) {
    uint64_t v = rdmsr(IA32_APIC_BASE_MSR);
    v |= IA32_APIC_BASE_EN;
    wrmsr(IA32_APIC_BASE_MSR, v);

    uint64_t lapic_base = v & IA32_APIC_BASE_MASK;
    if (!lapic_base)
        lapic_base = apic_get_lapic_phys_addr();

    map_phys_range_identity(lapic_base, 0x1000);
    map_phys_range_identity(IOAPIC_PHYS, 0x1000);

    lapic_write(lapic_base, LAPIC_SVR_REG, 0x100 | 0xFF);
}

void apic_eoi(void) {
    uint64_t lapic_base = apic_get_lapic_phys_addr();
    if (!lapic_base)
        lapic_base = apic_base_from_msr();

    lapic_write(lapic_base, LAPIC_EOI_REG, 0);
}

static void ioapic_route_isa_irq(uint8_t irq, uint8_t vector) {
    uint64_t lapic_base = apic_get_lapic_phys_addr();
    if (!lapic_base)
        lapic_base = apic_base_from_msr();
    
    uint32_t lapic_id = lapic_read(lapic_base, LAPIC_ID_REG) >> 24;
    uint8_t lo_reg = (uint8_t)(0x10 + irq * 2);
    uint8_t hi_reg = (uint8_t)(lo_reg + 1);

    uint32_t low = vector;
    low &= ~(1u << 16);
    low &= ~(1u << 15);
    low &= ~(1u << 13);
    low &= ~(7u << 8);
    low &= ~(1u << 11);

    uint32_t high = lapic_id << 24;

    ioapic_write(IOAPIC_PHYS, hi_reg, high);
    ioapic_write(IOAPIC_PHYS, lo_reg, low);
}

void ioapic_route_irq0_to_vector32(void) {
    ioapic_route_isa_irq(0, 32);
    ioapic_route_isa_irq(2, 32);
}

void ioapic_route_irq1_to_vector33(void) {
    ioapic_route_isa_irq(1, 33);
}

uint64_t apic_discover(apic_info_t *out) {
    if (!out)
        return 0;

    for (uint64_t i = 0; i < sizeof(apic_info_t); ++i)
        ((uint8_t *)out)[i] = 0;

    rsdp_t *rsdp = find_rsdp();
    if (!rsdp) {
        out->lapic_phys_addr = apic_base_from_msr();
        g_apic_info = *out;
        return out->lapic_phys_addr != 0;
    }

    madt_t *madt = find_madt(rsdp);
    if (!madt || !sdt_valid(&madt->h)) {
        out->lapic_phys_addr = apic_base_from_msr();
        g_apic_info = *out;
        return out->lapic_phys_addr != 0;
    }

    parse_madt(madt, out);
    if (out->lapic_phys_addr == 0)
        out->lapic_phys_addr = apic_base_from_msr();

    g_apic_info = *out;
    return out->lapic_phys_addr != 0;
}

uint64_t apic_get_lapic_phys_addr(void) {
    if (g_apic_info.lapic_phys_addr)
        return g_apic_info.lapic_phys_addr;

    return apic_base_from_msr();
}

void apic_dump_info(const apic_info_t *info) {
    if (!info)
        return;

    printf("APIC: LAPIC phys base = %x, CPUs = %d\n", info->lapic_phys_addr, info->cpu_count);
}

