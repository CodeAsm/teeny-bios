ASM       = gcc
CC        = gcc
LD        = ld
OBJDUMP   = objdump
OBJCOPY   = objcopy
 
CFLAGS    = -m32 -nostartfiles #-g
LDFLAGS   = -mi386pe
 
OBJECTS   = bios.o
TARGET    = bios
MEMLAYOUT = xMemLayout.map

# Suppress warnings from objcopy
OBJCOPY_FLAGS = -Wno-warning-flag-here
 
###############################################################################
# UEFI Bootloader + TeenyKernel
###############################################################################

# gnu-efi headers and support files (installed by the gnu-efi package)
EFI_INC        = /usr/include/efi
EFI_CFLAGS     = -I$(EFI_INC) -I$(EFI_INC)/x86_64 \
                 -DEFI_FUNCTION_WRAPPER \
                 -fno-stack-protector -fpic -fshort-wchar \
                 -mno-red-zone -Wall -Wextra -O2 \
                 -ffreestanding -fno-strict-aliasing

EFI_LDFLAGS    = -nostdlib -znocombreloc \
                 -T /usr/lib/elf_x86_64_efi.lds \
                 -shared -Bsymbolic -L/usr/lib

EFI_STARTFILE  = /usr/lib/crt0-efi-x86_64.o
EFI_LIBS       = -lgnuefi -lefi

# Kernel (freestanding x86-64)
KERN_CC        = gcc
KERN_CFLAGS    = -m64 -ffreestanding -nostdlib -fno-stack-protector \
                 -mno-red-zone -Wall -Wextra -O2
KERN_LD        = ld
KERN_LDFLAGS   = -T kernel/kernel.ld -nostdlib

.PHONY: all clean uefi kernel uefi-disk run-uefi
 
all: $(TARGET).rom $(TARGET).sym
 
clean:
	@-rm -f -v *.o $(TARGET).out $(TARGET).rom $(TARGET).sym $(MEMLAYOUT)
	@-rm -f -v uefi/bootloader.o uefi/bootloader.so uefi/bootloader.efi
	@-rm -f -v kernel/kernel.o kernel/kernel.elf kernel/kernel.bin
	@-rm -f -v uefi.img

%.o: %.c Makefile
	@echo "[CC]  $@"
	@$(CC) -c -o $*.o $(CFLAGS) $<
 
%.o: %.S Makefile
	@echo "[AS]  $<"
	@$(ASM) -c -o $*.o $(CFLAGS) $<
 
# Produce a disassembly dump of the main section, for verification purposes
dis: $(TARGET).out
	@echo "[DIS] $<"
	@$(OBJCOPY) -O binary -j .main --set-section-flags .main=alloc,load,readonly,code $< main.bin
	@$(OBJDUMP) -D -bbinary -mi8086 -Mintel main.bin | less
	@-rm -f main.bin
 
$(TARGET).out: $(OBJECTS) $(TARGET).ld
	@echo "[LD]  $@"
	@$(LD) $(LDFLAGS) -T$(TARGET).ld -o $@ $(OBJECTS) -Map $(MEMLAYOUT)
 
$(TARGET).rom: $(TARGET).out
	@echo "[ROM] $@"
	@# Note: -j only works for sections that have the 'ALLOC' flag set
	@$(OBJCOPY) $(OBJCOPY_FLAGS) -O binary -j .begin -j .main -j .reset --gap-fill=0x0ff $< $@

# Symbol creation

$(TARGET).sym: $(TARGET).o
	@echo "[SYM] $@"
	@$(OBJCOPY) --only-keep-debug $< $@
	@$(OBJCOPY) --strip-debug $<
	@$(OBJCOPY) --add-gnu-debuglink=$@ $<

###############################################################################
# UEFI Bootloader targets
###############################################################################

uefi: uefi/bootloader.efi

uefi/bootloader.o: uefi/bootloader.c uefi/bootinfo.h Makefile
	@echo "[EFI-CC] $@"
	@$(CC) $(EFI_CFLAGS) -c -o $@ $<

uefi/bootloader.so: uefi/bootloader.o Makefile
	@echo "[EFI-LD] $@"
	@$(LD) $(EFI_LDFLAGS) $(EFI_STARTFILE) $< -o $@ $(EFI_LIBS)

uefi/bootloader.efi: uefi/bootloader.so
	@echo "[EFI]    $@"
	@$(OBJCOPY) \
	    -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
	    -j .rel -j .rela -j .reloc \
	    --target=efi-app-x86_64 --subsystem=10 $< $@

###############################################################################
# Minimal kernel targets
###############################################################################

kernel: kernel/kernel.bin

kernel/kernel.o: kernel/kernel.c uefi/bootinfo.h Makefile
	@echo "[KERN-CC] $@"
	@$(KERN_CC) $(KERN_CFLAGS) -c -o $@ $<

kernel/kernel.elf: kernel/kernel.o kernel/kernel.ld Makefile
	@echo "[KERN-LD] $@"
	@$(KERN_LD) $(KERN_LDFLAGS) -o $@ $<

kernel/kernel.bin: kernel/kernel.elf
	@echo "[KERN]    $@"
	@$(OBJCOPY) -O binary $< $@

###############################################################################
# Create a VFAT disk image and populate it for UEFI boot
# Requires: mtools (mmd, mcopy), mkdosfs
###############################################################################

uefi-disk: uefi/bootloader.efi kernel/kernel.bin
	@echo "[DISK]   uefi.img"
	@dd if=/dev/zero of=uefi.img bs=1k count=4096 status=none
	@mkdosfs -F 32 uefi.img
	@mmd   -i uefi.img ::/EFI
	@mmd   -i uefi.img ::/EFI/BOOT
	@mcopy -i uefi.img uefi/bootloader.efi ::/EFI/BOOT/BOOTX64.EFI
	@mcopy -i uefi.img kernel/kernel.bin   ::/kernel.bin
	@echo "[DISK]   uefi.img ready"

###############################################################################
# Launch QEMU with OVMF and the UEFI disk image
###############################################################################

run-uefi: uefi-disk
	@echo "[QEMU] Starting UEFI VM (Ctrl-A X to quit)..."
	qemu-system-x86_64 -m 256M \
	    -bios /usr/share/qemu/OVMF.fd \
	    -drive format=raw,file=uefi.img \
	    -nographic
