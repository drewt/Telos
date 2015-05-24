TELOS_CFLAGS = -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
	       -Wno-attributes -fno-builtin -ffreestanding -std=gnu11 \
	       -I $(incdir) -fno-pic -nostdlib

quiet_cmd_telos_cc = UCC     $@
      cmd_telos_cc = $(CC) $(TELOS_CFLAGS) $(CPPFLAGS) -c -o $@ $<

quiet_cmd_telos_ld = ULD     $@
      cmd_telos_ld = $(LD) $(LDFLAGS) -nostdlib $(1) -T telos.ld -o $@

%.o: %.c
	$(call cmd,telos_cc)
