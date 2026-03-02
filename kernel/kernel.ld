/*
 * kernel.ld  –  Linker script for TeenyKernel
 *
 * Produces a flat x86-64 binary loaded by the UEFI bootloader at 0x200000
 * (KERNEL_LOAD_ADDR).  The .text.entry section must appear first so that
 * kernel_entry() is at offset 0 of the output binary.
 */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kernel_entry)

SECTIONS
{
    . = 0x200000;   /* must match KERNEL_LOAD_ADDR in uefi/bootloader.c */

    .text : {
        *(.text.entry)   /* kernel_entry must be at the very start */
        *(.text)
        *(.text.*)
    }

    .rodata : { *(.rodata*) }

    .data : { *(.data*) }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}
