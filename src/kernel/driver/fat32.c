
#include "../include/driver/vga.h"
#include "../include/libc/string.h"
#include "../include/driver/fat32.h"

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

char *rnsectors(uint64_t lba, uint64_t n) {

}

directory_t *read_directory(char *path) {

}
