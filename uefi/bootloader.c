/*******************************************************************************
 * bootloader.c  –  TeenyBIOS UEFI Bootloader
 *
 * A minimal UEFI application (built with gnu-efi) that:
 *   1. Opens the EFI System Partition root directory.
 *   2. Reads kernel.bin into memory at KERNEL_LOAD_ADDR.
 *   3. Obtains the UEFI memory map and exits boot services.
 *   4. Transfers control to the kernel, passing a BootInfo pointer.
 *
 * Build (see top-level Makefile target "uefi"):
 *   gcc <EFI_CFLAGS> -c bootloader.c -o bootloader.o
 *   ld  <EFI_LDFLAGS> bootloader.o -o bootloader.so
 *   objcopy ... bootloader.so bootloader.efi
 ******************************************************************************/
#include <efi.h>
#include <efilib.h>

#include "bootinfo.h"

/* Path to the flat-binary kernel on the ESP root */
#define KERNEL_PATH      L"\\kernel.bin"

/* Physical address where the kernel flat binary is loaded.
 * Must match the origin defined in kernel/kernel.ld.           */
#define KERNEL_LOAD_ADDR ((EFI_PHYSICAL_ADDRESS)0x200000ULL)

/* Kernel entry-point signature */
typedef void (*KernelEntry)(BootInfo *info) __attribute__((noreturn));

/* ── helpers ────────────────────────────────────────────────────────────── */

/*
 * open_root  –  open the root directory of the volume the bootloader was
 *               loaded from (usually the EFI System Partition).
 */
static EFI_STATUS open_root(EFI_HANDLE image, EFI_FILE_PROTOCOL **root)
{
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL      *li;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;

    status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               image, &LoadedImageProtocol, (void **)&li);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               li->DeviceHandle, &FileSystemProtocol,
                               (void **)&fs);
    if (EFI_ERROR(status))
        return status;

    return uefi_call_wrapper(fs->OpenVolume, 2, fs, root);
}

/*
 * get_file_size  –  query the size of an open file via EFI_FILE_INFO.
 */
static EFI_STATUS get_file_size(EFI_FILE_PROTOCOL *file, UINTN *out_size)
{
    EFI_STATUS    status;
    EFI_FILE_INFO *info;
    UINTN          buf_size = sizeof(EFI_FILE_INFO) + 256;

    status = uefi_call_wrapper(BS->AllocatePool, 3,
                               EfiLoaderData, buf_size, (void **)&info);
    if (EFI_ERROR(status))
        return status;

    status = uefi_call_wrapper(file->GetInfo, 4,
                               file, &GenericFileInfo, &buf_size, info);
    if (!EFI_ERROR(status))
        *out_size = (UINTN)info->FileSize;

    uefi_call_wrapper(BS->FreePool, 1, info);
    return status;
}

/* ── main entry point ───────────────────────────────────────────────────── */

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
    EFI_STATUS            status;
    EFI_FILE_PROTOCOL    *root, *kfile;
    UINTN                 ksize;
    EFI_PHYSICAL_ADDRESS  kaddr = KERNEL_LOAD_ADDR;
    UINTN                 pages;

    /* Memory-map state */
    EFI_MEMORY_DESCRIPTOR *mmap      = NULL;
    UINTN                  mmap_size = 0;
    UINTN                  mmap_buf_size;
    UINTN                  map_key, desc_size;
    UINT32                 desc_version;

    BootInfo    *binfo;
    KernelEntry  kernel;

    InitializeLib(image, systab);
    Print(L"TeenyBIOS UEFI Bootloader v0.1\r\n");

    /* ── 1. Open the ESP root directory ──────────────────────────────── */
    status = open_root(image, &root);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot open root volume (%r)\r\n", status);
        return status;
    }

    /* ── 2. Open kernel.bin ──────────────────────────────────────────── */
    status = uefi_call_wrapper(root->Open, 5, root, &kfile, KERNEL_PATH,
                               EFI_FILE_MODE_READ, 0ULL);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot open %s (%r)\r\n", KERNEL_PATH, status);
        return status;
    }

    status = get_file_size(kfile, &ksize);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot query kernel size (%r)\r\n", status);
        return status;
    }
    Print(L"Kernel: %lu bytes\r\n", (unsigned long)ksize);

    /* ── 3. Allocate pages at the fixed kernel address ───────────────── */
    pages  = (ksize + 0xFFFUL) / 0x1000UL;
    status = uefi_call_wrapper(BS->AllocatePages, 4,
                               AllocateAddress, EfiLoaderData, pages, &kaddr);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot allocate 0x%lx pages at 0x%lx (%r)\r\n",
              (unsigned long)pages, (unsigned long)kaddr, status);
        return status;
    }

    /* ── 4. Read kernel into allocated pages ─────────────────────────── */
    status = uefi_call_wrapper(kfile->Read, 3, kfile, &ksize, (void *)kaddr);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot read kernel (%r)\r\n", status);
        return status;
    }
    Print(L"Kernel loaded at 0x%lx\r\n", (unsigned long)kaddr);

    uefi_call_wrapper(kfile->Close, 1, kfile);
    uefi_call_wrapper(root->Close,  1, root);

    /* ── 5. Allocate BootInfo (before the final GetMemoryMap) ────────── */
    status = uefi_call_wrapper(BS->AllocatePool, 3,
                               EfiLoaderData, sizeof(BootInfo), (void **)&binfo);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot allocate BootInfo (%r)\r\n", status);
        return status;
    }

    /* ── 6. Allocate buffer for the UEFI memory map ──────────────────── */
    /* First call returns EFI_BUFFER_TOO_SMALL but gives us the needed size */
    uefi_call_wrapper(BS->GetMemoryMap, 5,
                      &mmap_size, mmap, &map_key, &desc_size, &desc_version);
    /* Add slack for the AllocatePool call below changing the map */
    mmap_buf_size = mmap_size + 4 * desc_size;
    status = uefi_call_wrapper(BS->AllocatePool, 3,
                               EfiLoaderData, mmap_buf_size, (void **)&mmap);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot allocate memory-map buffer (%r)\r\n", status);
        return status;
    }

    /* ── 7. Final GetMemoryMap – no UEFI allocations after this point ── */
    mmap_size = mmap_buf_size;
    status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                               &mmap_size, mmap,
                               &map_key, &desc_size, &desc_version);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: cannot get memory map (%r)\r\n", status);
        return status;
    }

    /* Populate BootInfo – no UEFI calls between here and ExitBootServices */
    binfo->version        = BOOTINFO_VERSION;
    binfo->mmap_base      = (unsigned long long)(UINTN)mmap;
    binfo->mmap_size      = (unsigned long long)mmap_size;
    binfo->mmap_desc_size = (unsigned long long)desc_size;

    Print(L"Exiting boot services and launching kernel...\r\n");

    /* ── 8. Exit boot services ───────────────────────────────────────── */
    status = uefi_call_wrapper(BS->ExitBootServices, 2, image, map_key);
    if (EFI_ERROR(status)) {
        /* Map key may have changed due to the Print() above; retry once */
        mmap_size = mmap_buf_size;
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
                                   &mmap_size, mmap,
                                   &map_key, &desc_size, &desc_version);
        if (!EFI_ERROR(status))
            status = uefi_call_wrapper(BS->ExitBootServices, 2, image, map_key);
        if (EFI_ERROR(status))
            return status;
    }

    /* ── 9. Hand control to the kernel ──────────────────────────────── */
    kernel = (KernelEntry)kaddr;
    kernel(binfo);   /* noreturn */

    /* unreachable */
    return EFI_SUCCESS;
}
