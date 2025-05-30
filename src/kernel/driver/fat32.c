
#include "../include/driver/fat32.h"
#include "../include/libc/storage.h"

extern bpb_t BPB;
static bpb_t *bpb;
static fs_info_t *fss;
static directory_t *root;

void init_bpb() {
    bpb = &BPB;
    fss = (fs_info_t*)(0x7C00UL + bpb->fs_info_sector * 0x200);
    

}

void init_fats() {
    root = (directory_t *)0x8800;
}
