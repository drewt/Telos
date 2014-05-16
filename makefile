MAKEFLAGS += -rR --no-print-directory

topdir = $(CURDIR)
incdir = $(topdir)/include
export topdir incdir

CCPREFIX  = #i586-elf-
AR        = $(CCPREFIX)ar
ARFLAGS   = rcs
AS        = $(CCPREFIX)as --32
CC        = $(CCPREFIX)gcc -m32 -march=i386
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	    -Wno-attributes -Wdeclaration-after-statement
ALLCFLAGS = $(CFLAGS) -I $(incdir) -fno-builtin -ffreestanding -std=gnu11 \
	    -include 'kernel/common.h'
LD        = $(CCPREFIX)ld -m elf_i386
OBJCOPY   = $(CCPREFIX)objcopy

export CCPREFIX AR ARFLAGS AS CC CFLAGS ALLCFLAGS LD OBJCOPY

findobj = find kernel dispatch drivers -name '*.o' | tr '\n' ' '

submakes = dispatch drivers kernel usr lib
clean = kernel.img kernel.bin boot/loader.o
distclean = pad

all: kernel.img

include rules.mk

quiet_cmd_ldgen  = GEN     linker.ld
      cmd_ldgen  = printf "INPUT ( %s )\n" \
		   "boot/loader.o `$(findobj)` usr/usr.a lib/klib.a" \
		   > linker.ld && cat sections.ld >> linker.ld

quiet_cmd_padgen = GEN     $@
      cmd_padgen = dd if=/dev/zero of=$@ bs=1 count=750 &> /dev/null

dispatch:
	+$(call cmd,smake)

drivers:
	+$(call cmd,smake)

kernel:
	+$(call cmd,smake)

usr:
	+$(call cmd,smake)

lib:
	+$(call cmd,smake)

boot/loader.o: boot/loader.s
	$(call cmd,as)

linker.ld: $(submakes) boot/loader.o
	$(call cmd,ldgen)

kernel.bin: linker.ld
	$(call cmd,sld,linker.ld)

pad:
	$(call cmd,padgen)

# make a bootable floppy image with grub legacy
kernel.img: boot/stage1 boot/stage2 pad kernel.bin
	$(call cmd,cat)

clean: topclean
topclean:
	@cd dispatch && $(MAKE) clean
	@cd drivers && $(MAKE) clean
	@cd kernel && $(MAKE) clean
	@cd usr && $(MAKE) clean
	@cd lib && $(MAKE) clean

distclean: topdistclean
topdistclean:
	@cd dispatch && $(MAKE) distclean
	@cd drivers && $(MAKE) distclean
	@cd kernel && $(MAKE) distclean
	@cd usr && $(MAKE) distclean
	@cd lib && $(MAKE) distclean

.PHONY: $(submakes) linker.ld topclean topdistclean
