include config.mk

SUBMAKES = dispatch drivers kernel usr

.SUFFIXES:
.PHONY: $(SUBMAKES) clean

FINDOBJ = find kernel dispatch drivers -name '*.o' | tr '\n' ' '

all: kernel.img

kernel.bin: $(SUBMAKES)
	@echo 'Generating linker script'
	@printf 'INPUT ( %s )\n' "`$(FINDOBJ)` lib/klib.a usr/usr.a" > linker.ld
	@cat sections.ld >> linker.ld
	$(LD) -T linker.ld -o $@

# make a bootable floppy image with grub legacy
kernel.img: kernel.bin pad
	cat boot/stage1 boot/stage2 pad kernel.bin > $@

pad:
	dd if=/dev/zero of=$@ bs=1 count=750

dispatch:
	cd dispatch; $(MAKE)

drivers:
	cd drivers; $(MAKE)

kernel:
	cd kernel; $(MAKE)

usr:
	cd usr; $(MAKE)

clean:
	rm -f boot/loader.o kernel.bin kernel.img
	cd dispatch; $(MAKE) clean
	cd drivers; $(MAKE) clean
	cd kernel; $(MAKE) clean
	cd usr; $(MAKE) clean

depclean:
	cd dispatch; $(MAKE) depclean
	cd drivers; $(MAKE) depclean
	cd kernel; $(MAKE) depclean
	cd usr; $(MAKE) depclean
