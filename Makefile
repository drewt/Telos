CCPREFIX = i586-elf-
CC       = $(CCPREFIX)gcc #-m32 -march=i386
CFLAGS   = -Wall -Wextra -Wno-unused-parameter -fno-builtin -ffreestanding \
	   -std=gnu99 -I include
LD       = $(CCPREFIX)ld #-m elf_i386
AS       = $(CCPREFIX)as #--32

LIB      = lib

ROOTFILES = bin/kernel.o bin/mem.o bin/gdt.o bin/intr.o \
	    bin/ctsw.o bin/syscall.o bin/pic.o bin/sysproc.o \
	    bin/inthandlers.o bin/procqueue.o bin/devinit.o

DISPFILES = bin/dispatch/dispatch.o bin/dispatch/io.o bin/dispatch/signal.o \
	    bin/dispatch/process.o bin/dispatch/time.o bin/dispatch/sysprint.o \
	    bin/dispatch/msg.o bin/dispatch/mem.o

DRVRFILES = bin/drivers/kbd.o bin/drivers/console.o

USERFILES = bin/usr/strtest.o bin/usr/proctest.o bin/usr/sigtest.o \
	    bin/usr/kbdtest.o bin/usr/eventtest.o bin/usr/msgtest.o \
	    bin/usr/tsh.o bin/usr/printserver.o bin/usr/memtest.o

OBJFILES = $(ROOTFILES) $(DISPFILES) $(DRVRFILES) $(USERFILES)

# directories
KERNEL = include/kernel
DRIVERS = $(KERNEL)/drivers
TELOS = include/telos

COMMON = $(KERNEL)/common.h
ARCH_H = $(KERNEL)/i386.h
DISP_H = $(KERNEL)/dispatch.h $(KERNEL)/process.h
LIB_H = include/klib.h

DEVICE_H = $(KERNEL)/device.h $(TELOS)/devices.h
CONSOLE_H = $(DRIVERS)/console.h $(TELOS)/console.h
KEYBOARD_H = $(DRIVERS)/kbd.h $(TELOS)/kbd.h

USER_H = include/signal.h include/string.h $(TELOS)/print.h $(TELOS)/io.h \
	 $(TELOS)/devices.h $(TELOS)/process.h $(TELOS)/mem.h

all: bin/kernel.img

$(OBJFILES):
	$(CC) $(CFLAGS) -c $(patsubst bin/%.o,src/%.c,$@) -o $@

$(USERFILES): $(USER_H)
$(ROOTFILES): $(COMMON)
$(DISPFILES): $(COMMON) $(DISP_H)
$(DRVRFILES): $(COMMON) $(DISP_H) $(ARCH_H) $(DEVICE_H)

# make a multiboot-compliant ELF kernel
bin/kernel.bin: bin/loader.o $(OBJFILES) $(LIB)/klib.a
	$(LD) -T bin/linker.ld bin/loader.o $(OBJFILES) $(LIB)/klib.a -o \
	    bin/kernel.bin

# make a bootable floppy image with grub legacy
bin/kernel.img: bin/kernel.bin
	dd if=/dev/zero of=bin/pad bs=1 count=750
	cat boot/stage1 boot/stage2 bin/pad bin/kernel.bin > bin/kernel.img

# assembly files
bin/loader.o: boot/loader.s
	$(AS) -o bin/loader.o boot/loader.s

# ROOTFILES: core kernel files
bin/kernel.o: src/kernel.c $(ARCH_H) $(DISP_H) $(KERNEL)/multiboot.h \
    $(KERNEL)/kernel.h $(CONSOLE_H)
bin/mem.o: src/mem.c include/mem.h
bin/gdt.o: src/gdt.c $(ARCH_H) $(LIB_H)
bin/intr.o: src/intr.c $(LIB_H) $(ARCH_H)
bin/inthandlers.o: src/inthandlers.c $(ARCH_H)
bin/ctsw.o: src/ctsw.c $(ARCH_H) $(DISP_H) $(KERNEL)/interrupt.h  \
    $(KERNEL)/process.h
bin/syscall.o: src/syscall.c include/telos/process.h
bin/pic.o: src/pic.c $(ARCH_H)
bin/sysproc.o: src/sysproc.c $(USER_H)
bin/procqueue.o: src/procqueue.c $(KERNEL)/process.h
bin/devinit.o: src/devinit.c $(DEVICE_H) $(KEYBOARD_H) $(CONSOLE_H)

# DISPFILES: system call and interrupt service code
bin/dispatch/dispatch.o: src/dispatch/dispatch.c include/syscall.h \
    $(DEVICE_H) $(KERNEL)/interrupt.h
bin/dispatch/process.o: src/dispatch/process.c $(ARCH_H) $(KERNEL)/time.h \
    include/mem.h
bin/dispatch/time.o: src/dispatch/time.c $(ARCH_H)
bin/dispatch/io.o: src/dispatch/io.c $(DEVICE_H)
bin/dispatch/sysprint.o: src/dispatch/sysprint.c
bin/dispatch/signal.o: src/dispatch/signal.c $(ARCH_H) include/syscall.h \
    include/signal.h
bin/dispatch/msg.o: src/dispatch/msg.c
bin/dispatch/mem.o: src/dispatch/mem.c include/mem.h

# DRVRFILES: drivers
bin/drivers/kbd.o: src/drivers/kbd.c $(KEYBOARD_H)
bin/drivers/console.o: src/drivers/console.c $(CONSOLE_H)

$(LIB)/klib.a: $(LIB)/klib/string.c
	(cd $(LIB)/klib; make install)

clean:
	rm $(OBJFILES) bin/kernel.bin bin/kernel.img

