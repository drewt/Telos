include config.mk

OBJECTS = dispatch/dispatch.a drivers/drivers.a kernel/kernel.a usr/usr.a

.SUFFIXES:
.PHONY: clean
.PHONY: $(OBJECTS)

all: kernel.img

kernel.bin: $(OBJECTS)
	$(LD) -T linker.ld $(OBJECTS) $(OBJECTS) \
	    lib/klib.a -o $@

# make a bootable floppy image with grub legacy
kernel.img: kernel.bin pad
	cat boot/stage1 boot/stage2 pad kernel.bin > $@

pad:
	dd if=/dev/zero of=$@ bs=1 count=750

dispatch/dispatch.a:
	cd dispatch; $(MAKE)

drivers/drivers.a:
	cd drivers; $(MAKE)

kernel/kernel.a:
	cd kernel; $(MAKE)

usr/usr.a:
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
