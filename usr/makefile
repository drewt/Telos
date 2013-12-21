UCFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	  -Wno-attributes -fno-builtin -ffreestanding -std=gnu99 -I $(incdir)

objects = consoletest.o echo.o eventtest.o kbdtest.o memtest.o msgtest.o \
	  printserver.o proctest.o sigtest.o strtest.o exntest.o tsh.o date.o
targets = usr.a
clean = $(objects) $(targets)

all: $(targets)

include $(topdir)/rules.mk

usr.a: $(objects)
	$(call cmd,ar)

#
# Userland programs are statically linked with their own copy of the C library.
# This is needed for crappy functions like strtok which have state.
#
# Argument: list of exported functions
#
quiet_cmd_ucc = CC      $@
      cmd_ucc = $(CC) $(UCFLAGS) $(CPPFLAGS) -c $< -o tmp.$@ && \
		$(LD) -r tmp.$@ ../lib/klib.a -o $@ && \
		$(OBJCOPY) $(foreach var,$(1),-G $(var)) $@ && \
		rm tmp.$@

printserver.o: printserver.c
	$(call cmd,ucc,printserver printclient)

# default: main function is basename of source/object file
%.o: %.c
	$(call cmd,ucc,$(basename $@))
