CCPREFIX = i586-elf-
CC = $(CCPREFIX)gcc #-m32 -march=i386
LD = $(CCPREFIX)ld #-m elf_i386
AR = $(CCPREFIX)ar
AS = $(CCPREFIX)as #--32
OBJCOPY = $(CCPREFIX)objcopy

CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=gnu99 -O \
                -I $(INC) -fno-builtin -ffreestanding

%d: %c
	@echo "Generating dependencies for $<"
	@$(CC) -MM -I $(INC) $(CPPFLAGS) $< > $@

FINDSRC = find src -name *.c | tr '\n' ' '
