#!/bin/bash
BIOS=rom/bios2.bin
MEM="2G"
HDD=disk.img

qemu-system-i386 -m ${MEM}    \
    -cpu 486                    \
    -M pc                       \
    -bios ${BIOS}
#    -drive file=${HDD}