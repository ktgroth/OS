
#include "../include/cpu/ports.h"
#include "../include/driver/fat32.h"
#include "../include/driver/storage.h"
#include "../include/libc/string.h"
#include "../include/libc/memory.h"
#include "../include/libc/stdlib.h"

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
static char *fat1;
static char *fat2;
extern directory_t *root = (void *)0x0;
extern directory_t *cwd = (void *)0x0;

static uint16_t bps = 0;
static uint8_t spc = 0;
static uint32_t spf = 0;
static uint32_t fat_start_sector = 0;
static uint32_t data_start_sector = 0;


void init_bpb() {
    bpb = &BPB;

    bps = bpb->bytes_per_sector;
    spc = bpb->sectors_per_cluster;
    spf = bpb->sectors_per_fat32;

    fat_start_sector = bpb->reserved_sectors;
    data_start_sector = bpb->reserved_sectors + (bpb->fat_allocation_tables * spf);

    fss = (fs_info_t*)(0x7C00UL + bpb->fs_info_sector * bps);
    fat1 = (char *)(bpb->reserved_sectors * bps);
    fat2 = (char *)fat1 + (spf * bps);
}

void init_fats_root() {
    root = (directory_t *)((char *)fss + (14 * spf * 2) * bps);

    root->name[0] = '/';
    root->flags = DIRECTORY;

    uint64_t addr = ((uint64_t)root - 14 * bps); 
    root->fc_hi = (addr >> 16) & 0xFF;
    root->fc_lo = addr & 0xFF;
    root->bytes = 14 * bps;

    cwd = root;
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
    void *sectors = kmalloc(n * bps);
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

uint32_t cluster_to_lba(uint32_t cluster) {
    return data_start_sector + (cluster - 2) * spc;
}

uint32_t get_next_cluster(uint32_t current_cluster) {
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector = (uint64_t)fat1 + (fat_offset / bps);
    uint32_t ent_offset = fat_offset % bps;

    uint8_t sector[512];
    r_sectors(fat_sector, sector, 1);
    return *(uint32_t *)&sector[ent_offset] & 0x0FFFFFFF;
}

uint32_t find_free_cluster() {
    uint32_t fat_entries = spf * bps / 4;
    for (uint32_t i = 2; i < fat_entries; ++i) {
        uint32_t fat_sector = fat_start_sector + (i * 4) / bps;
        uint32_t ent_offset = (i * 4) % bps;

        uint8_t sector[512];
        r_sectors(fat_sector, sector, 1);
        uint32_t entry = *(uint32_t *)&sector[ent_offset] & 0x0FFFFFFF;

        if (entry == 0x00000000) {
            *(uint32_t *)&sector[ent_offset] = 0x0FFFFFF8;
            w_sectors(fat_sector, sector, 1);
            return i;
        }
    }

    return 0xFFFFFFFF;
}

void append_cluster(uint32_t last_cluster, uint32_t new_cluster) {
    uint32_t fat_offset = last_cluster * 4;
    uint32_t fat_sector = (uint32_t)fat1 + (fat_offset / bps);
    uint32_t ent_offset = fat_offset % bps;

    uint8_t sector[512];
    r_sectors(fat_sector, sector, 1);
    *(uint32_t *)&sector[ent_offset] = new_cluster;
    w_sectors(fat_sector, sector, 1);

    fat_offset = new_cluster * 4;
    fat_sector = fat_start_sector + (fat_offset / bps);
    ent_offset = fat_offset % bps;

    r_sectors(fat_sector, sector, 1);
    *(uint32_t *)&sector[ent_offset] = 0x0FFFFFF8;
    w_sectors(fat_sector, sector, 1);
}

void write_cluster(uint32_t cluster, uint16_t *buffer, uint32_t bytes) {
    uint32_t lba = cluster_to_lba(cluster);
    for (uint32_t i = 0; i < spc; ++i)
        w_sectors(lba + i, buffer + (i * bps), 1);
}

void create_file(char *filename, uint16_t *buffer, uint32_t size) {
    uint32_t needed_clusters = (size + (spc * bps - 1)) / (spc * bps);

    uint32_t first_cluster = find_free_cluster();
    uint32_t prev_cluster = first_cluster;

    uint32_t written = 0;
    for (uint32_t i = 1; i < needed_clusters; ++i) {
        uint32_t next_cluster = find_free_cluster();
        append_cluster(prev_cluster, next_cluster);
        prev_cluster = next_cluster;
    }
}

void insert_directory_entry(char *filename, uint32_t first_cluster, uint32_t size) {
    uint32_t cluster = data_start_sector;
    uint8_t sector[512];

    while (cluster < 0x0FFFFFF8) {
        for (uint32_t s = 0; s < spc; ++s) {
            uint32_t lba = cluster_to_lba(cluster) + s;
            r_sectors(lba, sector, 1);

            for (uint32_t i = 0; i < bps; i += 32) {
                if (sector[i] == 0x00 || sector[i] == 0xE5) {
                    directory_t *entry = (directory_t *)&sector[i];

                    memset(entry, 0, sizeof(directory_t));
                    memcpy(entry->name, filename, strlen(filename));
                    entry->flags = 0x20;
                    entry->fc_hi = (first_cluster >> 16) & 0xFFFF;
                    entry->fc_lo = first_cluster & 0xFFFF;
                    entry->bytes = size;

                    w_sectors(lba, sector, 1);
                    return;
                }
            }
        }

        cluster = get_next_cluster(cluster);
    }
}

void read_file(uint32_t start_cluster, uint8_t *buffer, uint32_t size) {
    uint32_t cluster = start_cluster;
    uint32_t read = 0;
    while (cluster < 0x0FFFFFF8 && read < size) {
        uint32_t lba = cluster_to_lba(cluster);
        for (int i = 0; i < spc; ++i) {
            r_sectors(lba + i, buffer + read, 1);
            read += bps;
        }
        cluster = get_next_cluster(cluster);
    }
}

