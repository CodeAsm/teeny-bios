/*******************************************************************************
 * bootinfo.h  –  Boot information passed from the UEFI bootloader to the kernel
 *
 * Both the bootloader (efi context) and the kernel (freestanding context) use
 * this header, so it intentionally avoids EFI-specific types.
 ******************************************************************************/
#pragma once

#define BOOTINFO_VERSION 1

/*
 * BootInfo is populated by the bootloader just before ExitBootServices().
 * The kernel receives a pointer to this structure as its sole argument.
 */
typedef struct {
    unsigned long long version;        /* always BOOTINFO_VERSION              */
    unsigned long long mmap_base;      /* physical address of UEFI memory map  */
    unsigned long long mmap_size;      /* total size of the memory map (bytes) */
    unsigned long long mmap_desc_size; /* size of one EFI_MEMORY_DESCRIPTOR    */
} BootInfo;
