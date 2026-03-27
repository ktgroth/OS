#!/bin/bash

set -euo pipefail

kernel_location=0xC800
detected_memory=0x1F000
page_table=0x300000

AS=nasm
CC=gcc
LD=ld

UEFI=uefi
SRC=src
BOOT=$SRC/boot
KERNEL=$SRC/kernel
OBJ=obj
BIN=bin
BUILD=build

ESP="$BUILD/esp"
OBOOT="$ESP/EFI/BOOT"

UEFI_APP="$BOOT/BOOTX64.EFI"
VARS_RUNTIME="$BUILD/OVMF_VARS.4m.fd"

UEFI_CODE="$UEFI/OVMF_CODE.4m.fd"
UEFI_VARS_TEMPLATE="$UEFI/OVMF_VARS.4m.fd"
UEFI_VARS_RUNTIME="$BUILD/OVMF_VARS.4m.fd"

INC=gnu-efi/inc
EFI_LIBDIR=gnu-efi/x86_64/lib
EFI_GNUDIR=gnu-efi/x86_64/gnuefi
EFI_LDS=gnu-efi/gnuefi/elf_x86_64_efi.lds
EFI_CRT=$EFI_GNUDIR/crt0-efi-x86_64.o

AFLAGS="-f elf64"
CBFLAGS="-std=gnu11 -ffreestanding -fpic -fshort-wchar -fno-stack-protector -mno-red-zone -Wall -Wextra -Werror"
LBFLAGS="-shared -Bsymbolic -L$EFI_LIBDIR -L$EFI_GNUDIR -T$EFI_LDS $EFI_CRT"

CKFLAGS="-ffreestanding -fno-stack-protector -mno-red-zone -fno-pic -fno-pie -mcmodel=kernel -m64 -Wall -Wextra -Werror"
LKFLAGS="-nostdlib -T$KERNEL/kernel.ld"

SRCS=($(find "$KERNEL" -name "*.c"))
OBJS=()

for src in "${SRCS[@]}"; do
    obj="${OBJ}/$(basename "$src" "*.c").o"
    OBJS+=("$obj")
done

function all {
    mkdir -p $OBJ $BUILD $OBOOT $BUILD/tmp

    $CC $CBFLAGS \
        -I$INC \
        -c $BOOT/main.c -o $OBJ/main_efi.o
    $LD $LBFLAGS $OBJ/main_efi.o \
        --start-group -lgnuefi -lefi --end-group \
        -o $BUILD/BOOTX64.so
    objcopy \
        -I elf64-x86-64 -O efi-app-x86_64 \
        -j .text -j .sdata -j .data -j .rodata \
        -j .dynamic -j .dynsym -j .dynstr \
        -j .rel -j .rela -j .reloc \
        "$BUILD/BOOTX64.so" "$OBOOT/BOOTX64.EFI"

    for i in "${!SRCS[@]}"; do
        src="${SRCS[$i]}"
        obj="${OBJS[$i]}"
        echo $src $obj
        build $src $obj
    done

    $AS $AFLAGS $KERNEL/entry.s -o $OBJ/entry.s.o
    $LD $LKFLAGS $OBJ/entry.s.o ${OBJS[@]} -o "$BUILD/kernel.elf"
}

function run {
    all
    cp "$UEFI/OVMF_VARS.4m.fd" "$VARS_RUNTIME"

    TMPDIR="$BUILD/tmp" qemu-system-x86_64 \
        -machine q35 \
        -accel tcg \
        -cpu max \
        -drive if=pflash,format=raw,readonly=on,unit=0,file=$UEFI_CODE \
        -drive if=pflash,format=raw,unit=1,file=$VARS_RUNTIME \
        -drive format=raw,file=fat:rw:$ESP \
        -monitor stdio \
        -serial file:serial.log \
        -m 16G
}

function build {
    $CC $CKFLAGS -c $1 -o $2
}

function clean {
    clear

    rm -rf $OBJ $BIN $BUILD
    rm qemu.log 2>/dev/null
    rm serial.log 2>/dev/null
    rm debug.log 2>/dev/null
}

case "${1:-build}" in
    build) all ;;
    run) run ;;
    clean) clean ;;
    *) echo "Usage: $0 [build|run|clean]"; exit 1;;
esac
