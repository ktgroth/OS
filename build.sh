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
PCFLAGS="-target x86_64-elf -Wl,-u,-header -ffreestanding -fPIE -fno-stack-protector -mno-red-zone -nostdlib -nostartfiles -nodefaultlibs"
LFLAGS="-Ttext $kernel_location --image-base=$kernel_location --defsym=DETECTED_MEMORY=$detected_memory --defsym=PAGE_TABLE=$page_table --defsym=BPB=0x7C00"
PLFLAGS="-Tsrc/programs/program.ld -pie -nostdlib"

SRC=src
BOOT=$SRC/bootloader
PROGRAMS=$SRC/programs
KERNEL=$SRC/kernel
OBJ=obj
BIN=bin
BUILD=build

SRCSK=($(find "$KERNEL" -name "*.c"))
OBJSK=()

SRCSP=($(find "$PROGRAMS" -maxdepth 1 -name "*.c"))
PSYSCALLS_SRCS=($(find "$PROGRAMS/syscalls" -name "*.c"))
PSYSCALLS_OBJS=()
OBJSP=()
BINSP=()

for src in "${SRCSK[@]}"; do
    obj="${OBJ}/kernel/$(basename "$src" .c).o"
    OBJSK+=("$obj")
done

for src in "${SRCSP[@]}"; do
    obj="${OBJ}/programs/$(basename "$src" .c).o"
    bin="${BIN}/$(basename "$src" .c).bin"
    OBJSP+=("$obj")
    BINSP+=("$bin")
done

for src in "${PSYSCALLS_SRCS[@]}"; do
    obj="$OBJ/programs/$(basename "$src" .c).o"
    PSYSCALLS_OBJS+=("$obj")
done


OUTPUT=$BUILD/OS

function all {
    clean

    mkdir -p $OBJ
    mkdir -p $OBJ/kernel
    mkdir -p $OBJ/programs
    mkdir -p $BIN
    mkdir -p $BUILD

    for i in "${!SRCSK[@]}"; do
        src="${SRCSK[$i]}"
        obj="${OBJSK[$i]}"
        echo $src $obj
        build $src $obj
    done

    $AS $AOFLAGS $KERNEL/entry.s -o $OBJ/kernel/entry.o
    $AS $AOFLAGS $KERNEL/cpu/interrupts.s -o $OBJ/kernel/interrupts.o
    $AS $AOFLAGS $KERNEL/cpu/speed.s -o $OBJ/kernel/speed.o
    $LD $LFLAGS --oformat binary $OBJ/kernel/entry.o $OBJ/kernel/interrupts.o $OBJ/kernel/speed.o "${OBJSK[@]}" -o $BIN/kernel.bin
    kernel_size=$(wc -c <$BIN/kernel.bin)
    kernel_sectors=$((($kernel_size + 511) / 512))

    for i in "${!PSYSCALLS_SRCS[@]}"; do
        src="${PSYSCALLS_SRCS[$i]}"
        obj="${PSYSCALLS_OBJS[$i]}"
        echo $src $obj
        compile_program $src $obj
    done

    for i in "${!SRCSP[@]}"; do
        src="${SRCSP[$i]}"
        obj="${OBJSP[$i]}"
        bin="${BINSP[$i]}"
        echo $src $obj $bin
        compile_program $src $obj
        link_program $obj $bin
    done

    $AS $ABFLAGS $BOOT/boot.s \
        -D PAGE_TABLE=$page_table \
        -D KERNEL_LOCATION=$kernel_location \
        -D KERNEL_SIZE=$kernel_sectors \
        -D DETECTED_MEMORY=$detected_memory \
        -D BOOT_SECTORS=0 \
        -o $BIN/boot.bin
    bootloader_size=$(wc -c <$BIN/boot.bin)
    bootloader_sectors=$((($bootloader_size + 511) / 512))

    $AS $ABFLAGS $BOOT/boot.s \
        -D PAGE_TABLE=$page_table \
        -D KERNEL_LOCATION=$kernel_location \
        -D KERNEL_SIZE=$kernel_sectors \
        -D DETECTED_MEMORY=$detected_memory \
        -D BOOT_SECTORS=$bootloader_sectors \
        -o $BIN/boot.bin


    dd if=/dev/zero of=$OUTPUT bs=1M count=64
    dd if=$BIN/boot.bin of=$OUTPUT conv=notrunc
    dd if=$BIN/kernel.bin of=$OUTPUT bs=512 seek=$bootloader_sectors conv=notrunc
}

function run {
    all
    qemu-system-x86_64 \
        -enable-kvm \
        -cpu host,+apic,-x2apic \
        -smp 8,sockets=1,cores=4,threads=2,maxcpus=8 \
        -hda $OUTPUT \
        -monitor stdio \
        -no-reboot \
        -d in_asm,cpu_reset \
        -D qemu.log \
        -m 16G
}

function debug {
    all
    qemu-system-x86_64 \
        -enable-kvm \
        -cpu host,+apic,-x2apic \
        -smp 8,sockets=1,cores=4,threads=2,maxcpus=8 \
        -hda $OUTPUT \
        -monitor stdio \
        -no-reboot \
        -d in_asm,cpu_reset \
        -D qemu.log \
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

    rm -rf $OBJ
    rm -rf $BIN
    rm -rf $BUILD
    rm qemu.log 2>/dev/null
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
