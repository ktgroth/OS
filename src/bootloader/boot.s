[ORG 0x7C00]

jmp begin_real

PAGE_LEVEL_4 equ 0x1000
PAGE_LEVEL_3 equ PAGE_LEVEL_4 + 0x1000
PAGE_LEVEL_2 equ PAGE_LEVEL_3 + 0x1000
FIRST_PAGE_TABLE equ PAGE_LEVEL_2 + 0x1000
E820_BUFFER equ DETECTED_MEMORY
E820_ENTRY_SIZE equ 0x18
MAX_ENTRIES equ 0xC8

begin_real:
    [bits 16]
    
    mov bp, 0x0500
    mov sp, bp
    
    mov byte [boot_drive], dl

    call detect_mem

    mov bx, 0x02
    mov cx, KERNEL_SIZE
    add cx, 0x02

    mov dx, 0x7E00
    call load_bios

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

times 512-($ - bootloader_extended) db 0x00

bootloader_long:
begin_long_mode:
    [bits 64]

    mov rdi, STYLE_BLUE
    call clear_long

    mov rdi, STYLE_BLUE
    mov rsi, long_mode_note
    call print_long

    call KERNEL_START
    jmp $

%include "src/bootloader/long_mode/clear.s"
%include "src/bootloader/long_mode/print.s"

KERNEL_START equ 0x8200
STYLE_BLUE equ 0x1F

long_mode_note:
    db 'Now running in fully-enabled, 64-bit long mode!', 0x00

times 512-($ - bootloader_long) db 0x00
