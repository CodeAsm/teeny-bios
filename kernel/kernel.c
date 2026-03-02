/*******************************************************************************
 * kernel.c  –  TeenyKernel: minimal freestanding x86-64 kernel
 *
 * Loaded as a flat binary by the UEFI bootloader.  After ExitBootServices()
 * the CPU is still in 64-bit long mode with identity-mapped RAM; the VGA
 * text-mode buffer at 0xB8000 is readable and writable.
 *
 * Entry point: kernel_entry()
 *   - placed in .text.entry so the linker script guarantees it sits at the
 *     very start of the flat binary (= KERNEL_LOAD_ADDR in the bootloader).
 *   - receives a pointer to BootInfo as its sole argument (System V ABI: rdi).
 ******************************************************************************/
#include "../uefi/bootinfo.h"

/* VGA text-mode framebuffer: 80×25 cells, 2 bytes each (char | attribute) */
#define VGA_BASE  ((volatile unsigned short *)0xB8000UL)
#define VGA_COLS  80
#define VGA_ROWS  25
#define VGA_ATTR  0x0F00U  /* bright-white on black */

/* Clear the entire VGA screen */
static void vga_clear(void)
{
    volatile unsigned short *vga = VGA_BASE;
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        vga[i] = VGA_ATTR | ' ';
}

/* Print a NUL-terminated ASCII string starting at column 0 of the given row */
static void vga_print_row(int row, const char *s)
{
    volatile unsigned short *vga = VGA_BASE + row * VGA_COLS;
    for (int col = 0; *s && col < VGA_COLS; s++, col++)
        vga[col] = VGA_ATTR | (unsigned char)*s;
}

/* Simple hex formatter – writes exactly 16 hex digits into buf[16] */
static void hex64(char buf[17], unsigned long long v)
{
    const char *hex = "0123456789abcdef";
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--, v >>= 4)
        buf[i] = hex[v & 0xF];
}

/*
 * kernel_entry  –  first function executed after the bootloader hands over.
 *
 * __attribute__((section(".text.entry"))) ensures the linker script places
 * this function at offset 0 of the flat binary, i.e. at KERNEL_LOAD_ADDR.
 */
__attribute__((section(".text.entry"), noreturn))
void kernel_entry(BootInfo *binfo)
{
    char hexbuf[17];

    vga_clear();
    vga_print_row(0, "TeenyKernel booted via UEFI bootloader!");
    vga_print_row(1, "BootInfo fields:");

    /* Show mmap_base */
    vga_print_row(2, "  mmap_base      = 0x");
    hex64(hexbuf, binfo ? binfo->mmap_base : 0ULL);
    {
        volatile unsigned short *vga = VGA_BASE + 2 * VGA_COLS + 22;
        for (int i = 0; hexbuf[i]; i++)
            vga[i] = VGA_ATTR | (unsigned char)hexbuf[i];
    }

    /* Show mmap_size */
    hex64(hexbuf, binfo ? binfo->mmap_size : 0ULL);
    {
        volatile unsigned short *vga = VGA_BASE + 3 * VGA_COLS;
        const char *label = "  mmap_size      = 0x";
        for (int i = 0; label[i]; i++)
            vga[i] = VGA_ATTR | (unsigned char)label[i];
        vga += 22;
        for (int i = 0; hexbuf[i]; i++)
            vga[i] = VGA_ATTR | (unsigned char)hexbuf[i];
    }

    vga_print_row(5, "Kernel halted.  Press RESET to restart.");

    for (;;)
        __asm__ volatile ("hlt");
}
