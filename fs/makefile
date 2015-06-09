objects = buffer.o devices.o fcntl.o filesystems.o inode.o ioctl.o namei.o \
	  open.o pipe.o read_write.o stat.o super.o ramfs/ramfs.o modfs/modfs.o
submakes = ext2

all: $(objects) $(submakes)

$(submakes):
	+$(call cmd,smake)
.PHONY: $(submakes)
