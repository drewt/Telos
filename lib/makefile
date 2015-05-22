objects = itoa.o mem.o kbtoa.o string.o stdio.o unistd.o signal.o \
	  process.o vsprintf.o kstrtox.o time.o setjmp.o fs.o

all: klib.a

klib.a: $(objects)
	$(call cmd,ar)
