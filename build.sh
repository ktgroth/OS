#!/bin/bash

kernel_location=0xC800
detected_memory=0x1F000
page_table=0x1000

AS="nasm"
CC="clang"
LD="ld.lld"

ABFLAGS="-f bin"
AOFLAGS="-f elf64"
CFLAGS="-g -ffreestanding -target x86_64-elf"
LFLAGS="-Ttext $kernel_location --defsym=DETECTED_MEMORY=$detected_memory --defsym=PAGE_TABLE=$page_table --defsym=BPB=0x7C00"

SRC=src
BOOT=$SRC/bootloader
KERNEL=$SRC/kernel
OBJ=obj
BIN=bin
BUILD=build

SRCS=($(find "$KERNEL" -name "*.c" -o -path "$SRC" -name "*.c"))
OBJSK=()

for src in "${SRCS[@]}"; do
    obj="${OBJ}/kernel/$(basename "$src" .c).o"
    OBJSK+=("$obj")
done

OUTPUT=$BUILD/OS

function all {
    clean

    mkdir -p $OBJ
    mkdir -p $OBJ/kernel
    mkdir -p $BIN
    mkdir -p $BUILD

    for i in "${!SRCS[@]}"; do
        src="${SRCS[$i]}"
        obj="${OBJSK[$i]}"
        echo $src $obj
        build $src $obj
    done

    $AS $AOFLAGS $KERNEL/entry.s -o $OBJ/kernel/entry.o
    $AS $AOFLAGS $KERNEL/cpu/interrupts.s -o $OBJ/kernel/interrupts.o
    $LD $LFLAGS $OBJ/kernel/entry.o $OBJ/kernel/interrupts.o "${OBJSK[@]}" -o $BIN/kernel.elf
    $LD $LFLAGS --oformat binary $OBJ/kernel/entry.o $OBJ/kernel/interrupts.o "${OBJSK[@]}" -o $BIN/kernel.bin
    kernel_size=$(wc -c <$BIN/kernel.bin)
    kernel_sectors=$((($kernel_size + 511) / 512))

    $AS $ABFLAGS $BOOT/boot.s -D PAGE_TABLE=$page_table -D KERNEL_LOCATION=$kernel_location -D KERNEL_SIZE=$kernel_sectors -D DETECTED_MEMORY=$detected_memory -o $BIN/boot.bin
    bootloader_size=$(wc -c <$BIN/boot.bin)
    bootloader_sectors=$((($bootloader_size + 511) / 512))

    dd if=/dev/zero of=$OUTPUT bs=1M count=64
    dd if=$BIN/boot.bin of=$OUTPUT conv=notrunc
    dd if=$BIN/kernel.bin of=$OUTPUT bs=512 seek=$bootloader_sectors conv=notrunc
}

function run {
    all
    bochs -f debug_bochs
}

function debug {
    all
    qemu-system-x86_64 -hda $OUTPUT -monitor stdio -no-reboot -d in_asm,cpu_reset -D qemu.log -m 2G
    #qemu-system-x86_64 -hda $OUTPUT -S -gdb tcp::1234 -no-reboot -d in_asm,cpu_reset -D qemu.log &
    #gdb -x debug_setup
}

function build {
    $CC $CFLAGS -c $1 -o $2
}

function clean {
    clear

    rm -rf $OBJ
    rm -rf $BIN
    rm -rf $BUILD
    rm qemu.log
}

if [ "$1" = "" ]; then
    all
elif [ "$1" = "build" ]; then
    all
elif [ "$1" = "run" ]; then
    run
elif [ "$1" = "debug" ]; then
    debug
elif [ "$1" = "clean" ]; then
    clean
fi
