devices = initrd/dev/cons0 initrd/dev/cons1 initrd/dev/mod0 initrd/dev/mod1

clean = $(devices)

all: directories $(devices)

directories:
	mkdir -p initrd/bin
	mkdir -p initrd/dev
	mkdir -p initrd/mod
	mkdir -p initrd/tmp

initrd/dev/cons0:
	mknod $@ c 4 0

initrd/dev/cons1:
	mknod $@ c 4 1

initrd/dev/mod0:
	mknod $@ b 5 0

initrd/dev/mod1:
	mknod $@ b 5 1

.PHONY: directories
