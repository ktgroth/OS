
#include "../include/cpu/ports.h"
#include "../include/driver/fat32.h"
#include "../include/libc/string.h"
#include "../include/libc/memory.h"

#define ATAIO_DATA              0x01F0
#define ATAIO_ERROR             0x01F1
#define ATAIO_FEATURES          0x01F1
#define ATAIO_SECTOR_COUNT      0x01F2
#define ATAIO_LBA_LO            0x01F3
#define ATAIO_LBA_MID           0x01F4
#define ATAIO_LBA_HI            0x01F5
#define ATAIO_DRIVE             0x01F6
#define ATAIO_STATUS            0x01F7
#define ATAIO_COMMAND           0x01F7

#define ATAC_ALTERNATE          0x03F6
#define ATAC_DEVICE_CONTROL     0x03F6
#define ATAC_DRIVE_ADDRESS      0x03F7

#define AMNF_ERROR              0x0
#define TKZNF_ERROR             0x1
#define ABRT_ERROR              0x2
#define MCR_ERROR               0x3
#define IDNF_ERROR              0x4
#define MC_ERROR                0x5
#define UNC_ERROR               0x6
#define BBK_ERROR               0x7

extern bpb_t BPB;
static bpb_t *bpb;
static fs_info_t *fss;
static directory_t *root;
static directory_t *cwd;

void init_bpb() {
    bpb = &BPB;
    fss = (fs_info_t*)(0x7C00UL + bpb->fs_info_sector * 0x200);
}

void init_fats_root() {
    root = (directory_t *)((char *)fss + (14 + 18) * 0x200);

    root->name[0] = '/';
    root->flags = DIRECTORY;

    uint64_t addr = ((uint64_t)root - 14 * 0x200); 
    root->first_cluster_high = (addr >> 16) & 0xFF;
    root->first_cluster_low = addr & 0xFF;
    root->bytes = 14 * 0x200;

    cwd = root;
}

void getcwd(char *str) {
    strncpy(str, cwd->name, 11);
}

static inline void wait_bsy() {
    while (inb(ATAIO_COMMAND) & 0x80);
}

static inline void wait_drq() {
    while (!(inb(ATAIO_COMMAND) & 0x08));
}

void *r_sectors(uint64_t lba, uint16_t *buffer, uint8_t n) {
    outb(ATAIO_DRIVE, (lba >> 24) | 0xE0);
    outb(ATAIO_SECTOR_COUNT, n);
    outb(ATAIO_LBA_LO, lba & 0xFF);
    outb(ATAIO_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATAIO_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATAIO_COMMAND, 0x20);

    for (uint64_t i = 0; i < n; ++i) {
        wait_bsy();
        wait_drq();

        __asm__ __volatile__(
            "rep insw"
            : "+D"(buffer)
            : "d"(ATAIO_DATA), "c"(0x100)
            : "memory"
        );

        buffer += 0x100;
    }

    return buffer;
}

void w_sectors(uint64_t lba, uint16_t *buffer, uint8_t n) {
    outb(ATAIO_DRIVE, (lba >> 24) | 0xE0);
    outb(ATAIO_SECTOR_COUNT, n);
    outb(ATAIO_LBA_LO, lba & 0xFF);
    outb(ATAIO_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATAIO_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATAIO_COMMAND, 0x30);

    for (uint64_t i = 0; i < n; ++i) {
        wait_bsy();
        wait_drq();

        __asm__ __volatile__(
            "rep outsw"
            : "+S"(buffer)
            : "d"(ATAIO_DATA), "c"(0x100)
            : "memory"
        );

        buffer += 0x100;
    }
}

void *rnsectors(uint64_t lba, uint64_t n) {
    void *sectors = kmalloc(n * 0x200);
    uint16_t *buffer = (uint16_t *)sectors;

    while (n >= 0xFF) {
        r_sectors(lba, buffer, 0xFF);
        buffer += 0xFF * 0x100;
        lba += 0xFF;
        n -= 0xFF;
    }

    if (n)
        r_sectors(lba, buffer, n);

    return sectors;
}

void wnsectors(uint64_t lba, void *sectors, uint64_t n) {
    uint16_t *buffer = (uint16_t *)sectors;

    while (n >= 0xFF) {
        w_sectors(lba, buffer, 0xFF);
        buffer += 0xFF * 0x100;
        lba += 0xFF;
        n -= 0xFF;
    }

    if (n)
        w_sectors(lba, buffer, n);
}

directory_t *read_directory(char *path) {

}
