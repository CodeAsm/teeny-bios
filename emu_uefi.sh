#!/bin/bash
# emu_uefi.sh  –  Launch QEMU with OVMF (UEFI) and the TeenyBIOS bootloader disk
#
# Usage:
#   make uefi-disk        # build uefi.img first
#   ./emu_uefi.sh         # then run this script
#
# Controls:
#   Ctrl-A X  – quit QEMU (nographic mode)
#   Ctrl-A C  – enter QEMU monitor

MEM="256M"
OVMF="/usr/share/qemu/OVMF.fd"
DISK="uefi.img"

if [ ! -f "$OVMF" ]; then
    echo "OVMF firmware not found at $OVMF"
    echo "Install it with:  sudo apt-get install ovmf"
    exit 1
fi

if [ ! -f "$DISK" ]; then
    echo "Disk image '$DISK' not found.  Run:  make uefi-disk"
    exit 1
fi

qemu-system-x86_64 \
    -m ${MEM}                        \
    -bios ${OVMF}                    \
    -drive format=raw,file=${DISK}   \
    -nographic
