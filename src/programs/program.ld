
ENTRY(_start)

APP_BASE = 0x10000;
APP_HDR_SIZE = 0x20;
APP_PAYLOAD_BASE = APP_BASE + APP_HDR_SIZE;

SECTIONS
{
    . = APP_BASE;

    .hdr : ALIGN(1) {
        LONG(0x34365041)
        LONG(1)
        QUAD(_start - APP_PAYLOAD_BASE)
        QUAD(_file_end - APP_PAYLOAD_BASE)
        QUAD(1)
    }

    . = APP_PAYLOAD_BASE;

    .text.entry : ALIGN(16) {
        KEEP(*(.text.entry))
    }

    .text : ALIGN(16) {
        *(.text*)
    }

    .rodata : ALIGN(16) {
        *(.rodata*)
    }

    .data : ALIGN(16) {
        *(.data*)
    }

    .dynamic    : ALIGN(8)  { *(.dynamic) }
    .dynsym     : ALIGN(8)  { *(.dynsym) }
    .dynstr     : ALIGN(1)  { *(.dynstr) }
    .hash       : ALIGN(4)  { *(.hash) }
    .gnu.hash   : ALIGN(8)  { *(.gnu.hash) }
    .gnu.version :          { *(.gnu.version) }
    .gnu.version_r :        { *(.gnu.version_r) }
    .gnu.version_d :        { *(.gnu.version_d) }
    .rela.dyn   : ALIGN(8)  { *(.rela*) }
    .got        : ALIGN(8)  { *(.got*) }
    .plt        : ALIGN(16) { *(.plt*) }

    _file_end = .;

    .bss        : ALIGN(16) {
        *(.bss*)
        *(COMMON)
    }
}
