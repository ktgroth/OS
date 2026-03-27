#!/bin/bash

set -euo pipefail

kernel_location=0xC800
detected_memory=0x1F000
page_table=0x300000

UEFI=uefi
SRC=src
KERNEL=$SRC
OBJ=obj
BIN=bin
BUILD=build

ESP="$BUILD/esp"
BOOT="$ESP/EFI/BOOT"

CC="clang"
LD="lld-link"

UEFI_APP="$BOOT/BOOTX64.EFI"
VARS_RUNTIME="$BUILD/OVMF_VARS.4m.fd"

CFLAGS="--target=x86_64-pc-win32 -ffreestanding -fshort-wchar -fno-stack-protector -mno-red-zone -Wall -Wextra -Werror"
LFLAGS="/subsystem:efi_application /entry:efi_main /nodefaultlib /out:$UEFI_APP"

function all {
    mkdir -p $OBJ $BIN $BUILD $BOOT $BUILD/tmp

    $CC $CFLAGS -c $KERNEL/main.c -o $OBJ/main.o
    $LD $LFLAGS $OBJ/main.o
}

function run {
    all
    cp "$UEFI/OVMF_VARS.4m.fd" "$VARS_RUNTIME"

    TMPDIR="$BUILD/tmp" qemu-system-x86_64 \
        -machine q35 \
        -accel tcg \
        -cpu max \
        -drive if=pflash,format=raw,readonly=on,unit=0,file=$UEFI/OVMF_CODE.4m.fd \
        -drive if=pflash,format=raw,unit=1,file=$VARS_RUNTIME \
        -drive format=raw,file=fat:rw:$ESP \
        -monitor stdio \
        -m 16G
}

function build {
    $CC $CFLAGS -c $1 -o $2
}

function compile_program {
    $CC $PCFLAGS -c $1 -o $2
}

function link_program {
    echo "$LD $PLFLAGS ${PSYSCALLS_OBJS[@]} $1 -o $2"
    $LD $PLFLAGS ${PSYSCALLS_OBJS[@]} $1 -o $2
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
