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
[telos-build repository](https://github.com/drewt/telos-build "telos-build")
and following the instructions in the README.

Once the toolchain is installed, the kernel may be built by running `make` in
this directory.

    $ make

Running
-------

Telos requires an initial RAM disk to be loaded by the bootloader.  To
generate an initrd, run as root:

    # make initrd.img

Then Telos may be run in QEMU or loaded by a multiboot-compliant bootloader.
To run Telos in QEMU, a script is provided:

    $ ./qemu

tsh
---

Telos comes with a basic shell and a few programs.  Most of these programs
are placeholders for testing until a more complete user space is available.

Git Repository
--------------

https://github.com/drewt/Telos

    $ git clone https://github.com/drewt/Telos.git

