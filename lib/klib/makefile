CCPREFIX = i586-elf-
CC       = $(CCPREFIX)gcc
CFLAGS   = -Wall -Wextra -fno-builtin -ffreestanding -std=c99 -I ../../include -c
AR       = $(CCPREFIX)ar
LIB      = ..

.c.o:
	$(CC) $(CFLAGS) $<

OBJFILES = itoa.o mem.o kbtoa.o string.o print.o read.o nfa.o

all: klib.a

klib.a: $(OBJFILES)
	rm -f klib.a
	$(AR) cr klib.a $(OBJFILES)

install: klib.a
	cp klib.a $(LIB)/klib.a

clean:
	rm -f $(OBJFILES) klib.a
