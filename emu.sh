#!/bin/bash
BIOS=bios.rom
MEM="2G"
HDD=disk.img

qemu-system-i386 -m ${MEM}      \
    -cpu 486                    \
    -M pc                       \
    -bios ${BIOS}               \
    -nographic			\
    -d in_asm -D debug.log
#    -drive file=${HDD}
