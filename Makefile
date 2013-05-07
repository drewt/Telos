BIN = bin
SRC = src
LIB = lib
INC = include
DEP = make
BOOT = boot

SHELL     = /bin/sh
CCPREFIX  = i586-elf-
CC        = $(CCPREFIX)gcc #-m32 -march=i386
CFLAGS    = -Wall -Wextra -Wno-unused-parameter -std=gnu99
ALLCFLAGS = -I $(INC) -fno-builtin -ffreestanding $(CFLAGS)
LD        = $(CCPREFIX)ld #-m elf_i386
AS        = $(CCPREFIX)as #--32

FINDSRC = find src -name *.c | tr '\n' ' '

.SUFFIXES:
.PHONY: clean depclean maintainer-clean

all: $(BIN)/kernel.img

-include names.mk
-include $(DFILES)

# build binaries in $(BIN)
$(BIN)/%.o: $(SRC)/%.c
	$(CC) -c $(ALLCFLAGS) $(CPPFLAGS) $< -o $@

# automatically generate dependencies
$(DEP)/%.d: $(SRC)/%.c
	@echo "Generating dependencies for $<"
	@$(CC) -MM -MF $@ -MT "$@ $(subst $(DEP)/,$(BIN)/,$(basename $@)).o" \
	    -I $(INC) $(CPPFLAGS) $<

# make a multiboot-compliant ELF kernel
$(BIN)/kernel.bin: $(BIN)/loader.o $(OFILES) $(LIB)/klib.a
	$(LD) -T $(BIN)/linker.ld $(BIN)/loader.o $(OFILES) $(LIB)/klib.a -o \
	    $@

# make a bootable floppy image with grub legacy
$(BIN)/kernel.img: $(BIN)/kernel.bin $(BIN)/pad
	cat $(BOOT)/stage1 $(BOOT)/stage2 $(BIN)/pad $(BIN)/kernel.bin \
	    > $@

$(BIN)/pad:
	dd if=/dev/zero of=$@ bs=1 count=750

# assembly files
$(BIN)/loader.o: $(BOOT)/loader.s
	$(AS) -o $@ $<

$(LIB)/klib.a: $(LIB)/klib/string.c
	(cd $(LIB)/klib; make install)

clean:
	rm -f $(OFILES) $(BIN)/loader.o $(BIN)/kernel.bin $(BIN)/kernel.img

depclean:
	rm -f $(DFILES) 

maintainer-clean: clean depclean
	rm -f $(BIN)/pad
