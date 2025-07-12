
#ifndef DRIVER_STORAGE
#define DRIVER_STORAGE

#include "fat32.h"
#include "../libc/types.h"
#include "clock.h"

typedef struct {
    char        name[11];
    uint8_t     flags;
    uint8_t     reserved;
    uint8_t     ctime_s;
    uint16_t    ctime_hms;
    uint16_t    cdate;
    uint16_t    adate;
    uint16_t    fc_hi;
    uint16_t    mtime;
    uint16_t    mdate;
    uint16_t    fc_lo;
    uint32_t    bytes;
} directory_t;

typedef struct {
    uint8_t     ois;
    char        ffive[10];
    uint8_t     flags;
    uint8_t     zero1;
    uint8_t     check_sum;
    char        nsix[12];
    uint16_t    zero2;
    char        ltwo[4];
} long_file_name_t;

typedef struct {
    uint8_t     entry_type;
    uint8_t     nsecondary;
    uint16_t    check_sum;
    uint16_t    flags;
    uint16_t    reserved;
    uint16_t    ctime;
    uint16_t    cdate;
    uint16_t    mtime;
    uint16_t    mdate;
    uint16_t    atime;
    uint16_t    adate;
    uint8_t     ctime_s;
    uint8_t     mtime_s;
    uint8_t     utc_offset_c;
    uint8_t     utc_offset_m;
    uint8_t     utc_offset_a;
    char        zero[7];
} file_t;

typedef struct {
    uint8_t     entry_type;
    uint8_t     flags;
    uint8_t     reserved1;
    uint8_t     name_length;
    uint16_t    name_hash;
    uint16_t    reserved2;
    uint64_t    valid_data_length;
    uint32_t    reserved3;
    uint32_t    fc;
    uint64_t    data_length;
} stream_t;

typedef struct {
    uint8_t     entry_type;
    uint8_t     flags;
    char        name[30];
} file_name_t;


char *getcwd();


void wdirecotry(char *name);
directory_t *rdirectory(char *name);

#endif

