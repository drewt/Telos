INC = ../include
include ../config.mk

OBJECTS = consoletest.o echo.o eventtest.o kbdtest.o memtest.o msgtest.o \
	  printserver.o proctest.o sigtest.o strtest.o tsh.o
TARGET = usr.a

include ../common.mk

printserver.o: printserver.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	$(LD) -r $@ ../lib/klib.a -o s.$@
	$(OBJCOPY) -G printserver -G printclient s.$@ $@
	rm s.$@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	$(LD) -r $@ ../lib/klib.a -o s.$@
	$(OBJCOPY) -G $(basename $<) s.$@ $@
	rm s.$@
