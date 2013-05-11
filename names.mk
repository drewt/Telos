UFILES = bin/usr/strtest.o bin/usr/eventtest.o bin/usr/tsh.o bin/usr/memtest.o\
	 bin/usr/proctest.o bin/usr/kbdtest.o bin/usr/msgtest.o bin/usr/echo.o\
	 bin/usr/printserver.o bin/usr/sigtest.o bin/usr/consoletest.o

OFILES = bin/sighandlers.o\
	 bin/kernel.o bin/mem.o bin/intr.o bin/sysproc.o\
	 bin/ctsw.o bin/inthandlers.o bin/gdt.o\
	 bin/dispatch/process.o bin/dispatch/mem.o bin/dispatch/dispatch.o\
	 bin/dispatch/sysprint.o bin/dispatch/io.o bin/dispatch/msg.o\
	 bin/dispatch/signal.o bin/dispatch/time.o bin/pic.o bin/devinit.o\
	 bin/drivers/console.o bin/drivers/serial.o bin/drivers/kbd.o

DFILES = make/usr/strtest.d make/usr/eventtest.d make/usr/tsh.d make/usr/echo.d\
	 make/usr/memtest.d make/usr/proctest.d make/usr/kbdtest.d\
	 make/usr/msgtest.d make/usr/printserver.d make/usr/sigtest.d\
	 make/usr/consoletest.d make/sighandlers.d make/kernel.d make/mem.d\
	 make/intr.d make/sysproc.d make/ctsw.d\
	 make/inthandlers.d make/gdt.d make/dispatch/process.d\
	 make/dispatch/mem.d make/dispatch/dispatch.d make/dispatch/sysprint.d\
	 make/dispatch/io.d make/dispatch/msg.d make/dispatch/signal.d\
	 make/dispatch/time.d make/pic.d make/devinit.d make/drivers/console.d\
	 make/drivers/serial.d make/drivers/kbd.d
