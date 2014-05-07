objects = one.o two.o main.o
target  = telos_obj.o
clean   = $(objects) $(target)

all: $(target)

include $(topdir)/rules.mk

$(target): $(objects)
	$(call cmd,uld,multi)
