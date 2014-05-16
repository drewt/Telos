UCFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	  -Wno-attributes -fno-builtin -ffreestanding -std=gnu99 -I $(incdir)

submakes = multi
objects = consoletest.o echo.o echoclient.o echoserver.o eventtest.o \
	  kbdtest.o memtest.o msgtest.o proctest.o sigtest.o strtest.o \
	  exntest.o jmptest.o tsh.o date.o \
	  $(foreach var,$(submakes),$(var)/telos_obj.o )
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
#quiet_cmd_ucc = UCC     $@
#      cmd_ucc = $(CC) $(UCFLAGS) $(CPPFLAGS) -c $< -o tmp.$@ && \
#		$(LD) -r tmp.$@ ../lib/klib.a -o $@ && \
#		$(OBJCOPY) $(foreach var,$(1),-G $(var)) $@ && \
#		rm tmp.$@

quiet_cmd_ucc = UCC     $@
      cmd_ucc = $(CC) $(UCFLAGS) $(CPPFLAGS) -c $< -o tmp.$@ && \
		$(LD) -r tmp.$@ ../lib/klib.a -o $@ && \
		$(OBJCOPY) -G main $@ && \
		$(OBJCOPY) --redefine-sym main=$(1) $@ && \
		rm tmp.$@

quiet_cmd_umake = MAKE    $(1)
      cmd_umake = cd $(1) && $(MAKE)

#
# Submakes should produce a file telos_obj.o in their directory.  This is an
# incrementally linked object containing only the 'main' symbol, as produced
# by the 'uld' command.
#
%/telos_obj.o: %
	+$(call cmd,umake,$<)

# default: main function is basename of source/object file
%.o: %.c
	$(call cmd,ucc,$(basename $@))
