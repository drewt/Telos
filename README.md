Telos
=====

Copyright © 2013-2015 Drew Thoreson

Some code is adapted from or inspired by Mach, Linux, tutorials on the net,
etc..  Details are in the code.

About
-----

Telos is a simple x86 kernel, supporting:

* preemptive multitasking
* processes running in kernel- and user-mode
* copy-on-write and demand paging
* a virtual filesystem layer
* memory-mapped files
* POSIX signals
* POSIX timers
* ...and more

Building
--------

You'll need the i386-telos toolchain.  It can be obtained by checking out the
[telos-build](https://github.com/drewt/telos-build "telos-build") repository
and following the instructions in the README.

Once the toolchain is installed, the kernel may be built by running `make` in
this directory.

    $ make

Running
-------

Telos requires an initial RAM disk to be loaded by the bootloader.  Building
an initrd is outside the scope of this repository.  See the
[telos-initrd](https://github.com/drewt/telos-initrd "telos-initrd") repository
for an initrd suitable for use with Telos.  If you checked out telos-build
earlier, you should already have this.

Telos is a multiboot kernel, so it should be possible to load it in any
bootloader implementing the multiboot standard.  QEMU is known to work, and
a script to run the kernel in QEMU is provided by telos-build.

Git Repository
--------------

https://github.com/drewt/Telos

    $ git clone https://github.com/drewt/Telos.git

