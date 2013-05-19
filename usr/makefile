ROOT = ..
include ../config.mk

OBJECTS = consoletest.o echo.o eventtest.o kbdtest.o memtest.o msgtest.o \
	  printserver.o proctest.o sigtest.o strtest.o exntest.o tsh.o
TARGET  = usr.a

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) rcs $@ $^

printserver.o: printserver.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o tmp.$@
	$(LD) -r tmp.$@ ../lib/klib.a -o $@
	$(OBJCOPY) -G printserver -G printclient $@
	rm tmp.$@

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o tmp.$@
	$(LD) -r tmp.$@ ../lib/klib.a -o $@
	$(OBJCOPY) -G $(basename $<) $@
	rm tmp.$@

include ../common.mk
