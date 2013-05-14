INC = ../include
include ../config.mk

OBJECTS = ctsw.o devinit.o gdt.o inthandlers.o intr.o kernel.o mem.o pic.o \
	  sighandlers.o sysproc.o loader.o
TARGET  = kernel.a

include ../common.mk

loader.o: loader.s
	$(AS) -o $@ $<
