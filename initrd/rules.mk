TELOS_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	       -Wno-attributes -std=gnu11
TELOS_CC = i386-telos-gcc
TELOS_LD = $(TELOS_CC)

quiet_cmd_telos_cc = UCC     $@
      cmd_telos_cc = $(TELOS_CC) $(TELOS_CFLAGS) $(TELOS_CPPFLAGS) -c -o $@ $<

quiet_cmd_telos_ld = ULD     $@
      cmd_telos_ld = $(TELOS_LD) $(TELOS_LDFLAGS) $(1) -o $@

%.o: %.c
	$(call cmd,telos_cc)
