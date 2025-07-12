[ORG 0x7C00]


JUMP_BOOT_RECORD:       db 0xEB, 0x3C, 0x90
OEM_IDENTIFIER:         db 'FRDOS5.1'
BYTES_PER_SECTOR:       dw 0x1000
SECTORS_PER_CLUSTER:    db 0x04
RESERVED_SECTORS:       dw 0x0003
FILE_ALLOCATION_TABLES: db 0x02
ROOT_DIRECTORY_ENTIRES: dw 0x00E0
TOTAL_SECTORS:          dw 0x0000
MEDIA_DESCRIPTOR_TYPE:  db 0xF0
SECTORS_PER_FAT:        dw 0x0009
SECTORS_PER_TRACK:      dw 0x0012
HEADS:                  dw 0x0002
HIDDEN_SECTORS:         dd 0x00000000
LARGE_SECTOR_COUNT:     dd 0x00400000

SECTORS_PER_FAT32:      dd 0x00400000
EXT_FLAGS:              dw 0x0000
FS_VERSION:             dw 0x0000
ROOT_CLUSTER:           dd 0x00000002
FSINFO_SECTOR:          dw 0x0001
BACKUP_BOOT_SECTOR:     dw 0x0006
RESERVED1:              db 0x0C
DRIVE_NUMBER:           db 0x80
RESERVED2:              db 0x00
BOOT_SIGNATURE:         db 0x29
VOLUME_ID:              dd 0x12345678
VOLUME_LABEL:           db 'MY OS FAT32'
FS_TYPE:                db 'FAT32   '


PAGE_LEVEL_4            equ PAGE_TABLE
PAGE_LEVEL_3            equ PAGE_LEVEL_4 + 0x1000
PAGE_LEVEL_2            equ PAGE_LEVEL_3 + 0x1000
FIRST_PAGE_TABLE        equ PAGE_LEVEL_2 + 0x1000
E820_BUFFER             equ DETECTED_MEMORY
E820_ENTRY_SIZE         equ 0x18
MAX_ENTRIES             equ 0xC8

begin_real:
    [bits 16]

    mov bp, 0x0500
    mov sp, bp

    mov byte [boot_drive], dl

    mov bx, 0x02
    mov cx, 0x25

    mov dx, 0x7E00
    call load_bios

    call detect_mem
    call elevate_bios
    jmp $

%include "src/bootloader/real_mode/load.s"
%include "src/bootloader/real_mode/gdt.s"
%include "src/bootloader/real_mode/elevate.s"
%include "src/bootloader/real_mode/mem.s"

boot_drive:
    db 0x00

times 510-($-$$) db 0x00
dw 0xAA55


fsinfo_sector:
LEAD_SIGNATURE:         dd 0x41615252
RESERVED3:              times 480 db 0x00
MIDDLE_SIGNATURE:       dd 0x61417272
FREE_CLUSTER_COUNT:     dd 0xFFFFFFFF
FS_DRIVER_CLUSTER:      dd 0xFFFFFFFF
RESERVED4:              times 12 db 0x00
TRAIL_SIGNATURE:        dd 0xAA550000

times 512-($-fsinfo_sector) db 0x00


RESERVED_SECTOR1:
times 512-($-RESERVED_SECTOR1) db 0x00


FATS:
; #FATS * SPF * BPS
times (18 * 512)-($-FATS) db 0x00


ROOT_DIRECTORY:
; (#RDE * 32 - 1) / BPS

HOME_DIRECTORY:
NAME: db 'HOME'
times 11-($-NAME) db 0x00
FLAGS: db 0x10
times 8 db 0x00
FC_HIGH: db 0x3D
times 4 db 0x00
FC_LOW: db 0x00
BYTES: db 0x00000000

times (15 * 512)-($-ROOT_DIRECTORY) db 0x00


bootloader_extended:
begin_protected:
    [bits 32]   

    call detect_lm_protected
    call init_page_directory
    call elevate_protected

    jmp $

%include "src/bootloader/protected_mode/detect_lm.s"
%include "src/bootloader/protected_mode/paging.s"
%include "src/bootloader/protected_mode/gdt.s"
%include "src/bootloader/protected_mode/elevate.s"

VGA_START equ 0xB8000
VGA_EXTENT equ 80 * 25 * 2
STYLE_WB equ 0x0F

times 512-($-bootloader_extended) db 0x00


bootloader_long:
begin_long_mode:
    [bits 64]

    mov rdi, STYLE_BLUE
    call clear_long

    mov rdi, STYLE_BLUE
    mov rsi, long_mode_note
    call print_long

    mov rax, 0x26
    mov rsi, KERNEL_SIZE
    mov rdi, KERNEL_LOCATION

    cmp rsi, 0xFF
    jl .read_remaining_sectors

.read_ff_sectors:
    mov cl, 0xFF
    call ata_read
    add rax, 0xFF

    sub rsi, 0xFF
    cmp rsi, 0xFF
    jz .done_read
    jge .read_ff_sectors

.read_remaining_sectors:
    mov rcx, rsi
    call ata_read

.done_read:
    call KERNEL_LOCATION
    jmp $

%include "src/bootloader/long_mode/clear.s"
%include "src/bootloader/long_mode/print.s"
%include "src/bootloader/long_mode/load.s"

STYLE_BLUE equ 0x1F

long_mode_note:
    db 'Now running in fully-enabled, 64-bit long mode!', 0x00

times 512-($-bootloader_long) db 0x00

