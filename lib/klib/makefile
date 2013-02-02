CCPREFIX = #i586-elf-
CC       = $(CCPREFIX)gcc -m32 -march=i386
CFLAGS   = -Wall -Wextra -fno-builtin -ffreestanding -std=gnu99 -I ../../include -c
AR       = $(CCPREFIX)ar
LIB      = ..

.c.o:
	$(CC) $(CFLAGS) $<

OBJFILES = itoa.o mem.o kbtoa.o string.o print.o read.o unistd.o signal.o

all: klib.a

klib.a: $(OBJFILES)
	rm -f klib.a
	$(AR) cr klib.a $(OBJFILES)

install: klib.a
	cp klib.a $(LIB)/klib.a

clean:
	rm -f $(OBJFILES) klib.a
