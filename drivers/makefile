objects = pc_console.o tty.o
submakes = block

all: $(objects) $(submakes)

$(submakes):
	+$(call cmd,smake)
.PHONY: $(submakes)
