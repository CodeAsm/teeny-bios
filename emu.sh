#!/bin/bash
BIOS=rom/bios.bin
MEM="2G"
HDD=disk.img

qemu-system-x86_64 -m ${MEM}    \
    -M pc                       \
    -bios ${BIOS}
#    -drive file=${HDD}