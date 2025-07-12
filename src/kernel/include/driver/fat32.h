
#ifndef __DRIVER_FAT32
#define __DRIVER_FAT32

#include "../libc/types.h"

typedef struct __attribute__((packed)) {
    uint8_t jc[3];
    char oem_identifier[8];
    uint16_t bytes_per_sector;
    uint8_t secters_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_allocation_tables;
    uint16_t root_directory_entry;
    uint16_t total_sectors;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_fat;
    uint16_t sector_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    uint32_t sectors_per_fat32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_director_cluster;
    uint16_t fs_info_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved1[12];
    uint8_t drive_number;
    uint8_t reserved2;
    uint8_t signature;
    uint32_t volume_id;
    char volume_label[11];
    char system_identifier_string[8];
} bpb_t;

typedef struct __attribute__((packed)) {
    uint32_t lead_signature;
    uint8_t reserved1[480];
    uint32_t middle_signature;
    uint32_t lk_free_cluster;
    uint32_t cn_fs_driver;
    uint8_t reserved2[12];
    uint32_t trail_signature;
} fs_info_t;


#define READ_ONLY   0x01
#define HIDDEN      0x02
#define SYSTEM      0x04
#define VOLUME_ID   0x08
#define DIRECTORY   0x10
#define ARCHIVE     0x20
#define LFN         READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID


void init_bpb();
void init_fats_root();

void *rnsectors(uint64_t lba, uint64_t n);
void wnsectors(uint64_t lba, void *buffer, uint64_t n);

#endif
