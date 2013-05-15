ROOT = .
include config.mk

SUBMAKES = dispatch drivers kernel usr
GENFILES = kernel.bin kernel.img linker.ld

.SUFFIXES:
.PHONY: $(SUBMAKES) clean depclean maintainer-clean

FINDOBJ = find kernel dispatch drivers -name '*.o' | tr '\n' ' '

all: kernel.img

kernel.bin: $(SUBMAKES) boot/loader.o
	@echo 'Generating linker script'
	@printf 'INPUT ( %s )\n' \
	    "boot/loader.o `$(FINDOBJ)` usr/usr.a lib/klib.a" \
	    > linker.ld
	@cat sections.ld >> linker.ld
	$(LD) -T linker.ld -o $@

# make a bootable floppy image with grub legacy
kernel.img: kernel.bin pad
	cat boot/stage1 boot/stage2 pad kernel.bin > $@

pad:
	dd if=/dev/zero of=$@ bs=1 count=750

boot/loader.o: boot/loader.s
	$(AS) -o $@ $<

dispatch:
	cd dispatch; $(MAKE)

drivers:
	cd drivers; $(MAKE)

kernel:
	cd kernel; $(MAKE)

usr:
	cd usr; $(MAKE)

clean:
	rm -f $(GENFILES)
	cd dispatch; $(MAKE) clean
	cd drivers; $(MAKE) clean
	cd kernel; $(MAKE) clean
	cd usr; $(MAKE) clean

depclean:
	cd dispatch; $(MAKE) depclean
	cd drivers; $(MAKE) depclean
	cd kernel; $(MAKE) depclean
	cd usr; $(MAKE) depclean

maintainer-clean:
	rm -f $(GENFILES) pad
	cd dispatch; $(MAKE) maintainer-clean
	cd drivers; $(MAKE) maintainer-clean
	cd kernel; $(MAKE) maintainer-clean
	cd usr; $(MAKE) maintainer-clean
