CCPREFIX = #i586-elf-
CC       = $(CCPREFIX)gcc -m32 -march=i386
CFLAGS   = -Wall -Wextra -Wno-unused-parameter -fno-builtin -ffreestanding \
	   -std=gnu99 -I include
LD       = $(CCPREFIX)ld -m elf_i386
AS       = $(CCPREFIX)as --32

LIB      = lib

ROOTFILES = bin/kernel.o bin/mem.o bin/gdt.o bin/intr.o bin/ctsw.o \
	    bin/syscall.o bin/pic.o bin/sysproc.o bin/inthandlers.o \
	    bin/procqueue.o bin/devinit.o bin/sighandlers.o

DISPFILES = bin/dispatch/dispatch.o bin/dispatch/io.o bin/dispatch/signal.o \
	    bin/dispatch/process.o bin/dispatch/time.o bin/dispatch/sysprint.o \
	    bin/dispatch/msg.o bin/dispatch/mem.o

DRVRFILES = bin/drivers/kbd.o bin/drivers/console.o bin/drivers/serial.o

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
DISP_H = $(KERNEL)/dispatch.h $(KERNEL)/process.h include/errnodefs.h
LIB_H = include/klib.h

CONSOLE_H = $(DRIVERS)/console.h $(TELOS)/console.h
KEYBOARD_H = $(DRIVERS)/kbd.h $(TELOS)/kbd.h

USER_H = include/signal.h include/string.h $(TELOS)/print.h $(TELOS)/process.h \
	 include/unistd.h

all: bin/kernel.img

$(OBJFILES): bin/%.o: src/%.c
	$(CC) $(CFLAGS) -c $(patsubst bin/%.o,src/%.c,$@) -o $@

$(USERFILES): $(USER_H)
$(ROOTFILES): $(COMMON)
$(DISPFILES): $(COMMON) $(DISP_H)
$(DRVRFILES): bin/%.o: $(KERNEL)/%.h $(COMMON) $(DISP_H) $(ARCH_H) \
    $(KERNEL)/device.h

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
bin/kernel.o: $(ARCH_H) $(DISP_H) $(KERNEL)/multiboot.h $(KERNEL)/kernel.h \
    $(CONSOLE_H)
bin/mem.o: $(KERNEL)/mem.h
bin/gdt.o: $(ARCH_H) $(LIB_H)
bin/intr.o: $(LIB_H) $(ARCH_H)
bin/inthandlers.o: $(ARCH_H)
bin/ctsw.o: $(ARCH_H) $(DISP_H) $(KERNEL)/interrupt.h $(KERNEL)/process.h
bin/syscall.o: include/telos/process.h
bin/pic.o: $(ARCH_H)
bin/sysproc.o: $(USER_H)
bin/procqueue.o: $(KERNEL)/process.h
bin/devinit.o: $(KERNEL)/device.h $(KEYBOARD_H) $(CONSOLE_H)
bin/sighandlers.o: $(TELOS)/process.h

# DISPFILES: system call and interrupt service code
bin/dispatch/dispatch.o: include/syscall.h $(KERNEL)/device.h \
    $(KERNEL)/interrupt.h
bin/dispatch/process.o: $(ARCH_H) $(KERNEL)/time.h $(KERNEL)/mem.h
bin/dispatch/time.o: $(ARCH_H)
bin/dispatch/io.o: $(KERNEL)/device.h
bin/dispatch/signal.o: $(ARCH_H) include/syscall.h include/signal.h
bin/dispatch/mem.o: $(KERNEL)/mem.h

$(LIB)/klib.a: $(LIB)/klib/string.c
	(cd $(LIB)/klib; make install)

clean:
	rm $(OBJFILES) bin/kernel.bin bin/kernel.img

