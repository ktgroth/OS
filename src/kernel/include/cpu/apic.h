#ifndef __CPU_APIC
#define __CPU_APIC

#include "../libc/types.h"

#define APIC_MAX_CPUS 256

typedef struct {
    uint8_t acpi_cpu_id;
    uint8_t apic_id;
    uint32_t flags;
} apic_cpu_t;

typedef struct {
    uint64_t lapic_phys_addr;
    uint32_t madt_flags;
    uint32_t cpu_count;
    apic_cpu_t cpus[APIC_MAX_CPUS];
} apic_info_t;

void apic_enable_lapic(void);
void apic_eoi(void);
void ioapic_route_irq0_to_vector32(void);
void ioapic_route_irq1_to_vector33(void);

uint64_t apic_discover(apic_info_t *out);
uint64_t apic_get_lapic_phys_addr(void);
void apic_dump_info(const apic_info_t *info);

#endif

