MAKEFLAGS += -rR --no-print-directory

topdir = $(CURDIR)
incdir = $(topdir)/include
export topdir incdir

VERSION := $(shell git rev-parse --verify --short HEAD 2>/dev/null)-git

CCPREFIX  = i386-telos-
AR        = $(CCPREFIX)ar
ARFLAGS   = rcs
AS        = $(CCPREFIX)as
CC        = $(CCPREFIX)gcc
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	    -Wno-attributes
ALLCFLAGS = $(CFLAGS) -fno-builtin -ffreestanding -std=gnu11 \
	    -include 'kernel/common.h'
CPPFLAGS  = -I $(incdir) -DVERSION=\"$(VERSION)\"
LD        = $(CCPREFIX)ld
OBJCOPY   = $(CCPREFIX)objcopy
MAKEFILES = $(topdir)/rules.mk

export CCPREFIX AR ARFLAGS AS CC CFLAGS ALLCFLAGS CPPFLAGS LD OBJCOPY

# command to find all kernel object files
findobj = find kernel dispatch drivers fs -name '*.o' | tr '\n' ' '

submakes = dispatch drivers fs kernel lib
clean = kernel.bin boot/loader.o

all: kernel.bin

include rules.mk

# Generate the linker script to use when linking the kernel.
quiet_cmd_ldgen  = GEN     linker.ld
      cmd_ldgen  = printf "INPUT ( %s )\n" \
		   "boot/loader.o `$(findobj)` lib/klib.a" \
		   > linker.ld && cat sections.ld >> linker.ld

$(submakes) initrd:
	+$(call cmd,smake)

boot/loader.o: boot/loader.s
	$(call cmd,as)

linker.ld: $(submakes) boot/loader.o
	$(call cmd,ldgen)

kernel.bin: linker.ld
	$(call cmd,sld,linker.ld)
	@objdump -D $@ > dump

# Generate an initial RAM disk.
initrd.img: initrd
	./scripts/mkfs.sh initrd/initrd $@

initrd-clean:
	@$(MAKE) -C initrd clean
	rm -f initrd.img

.PHONY: $(submakes) linker.ld initrd initrdclean initrd.img
