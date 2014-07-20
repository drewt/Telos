objects = itoa.o mem.o kbtoa.o string.o print.o read.o unistd.o signal.o \
	  msg.o process.o vsprintf.o kstrtox.o time.o setjmp.o fs.o
targets = klib.a
clean = $(objects)

all: $(targets)

include $(topdir)/rules.mk

klib.a: $(objects)
	$(call cmd,ar)
