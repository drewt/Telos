Telos
=====

Copyright © 2013 Drew Thoreson

Some code is adapted from or inspired by Mach, Linux, tutorials on the net,
etc..  Details are in the code.


About
-----

Telos is a simple x86 kernel, supporting:

* preemptive multitasking
* processes running in kernel- and user-mode
* POSIX signals (partial)
* message passing IPC
* rudimentary paging (WIP)


Building
--------

You'll need a recent version of gcc with support for the i386 target.

    $ make


Running
-------

To run the kernel in QEMU,

    $ ./qemu

or Bochs,

    $ bochs


TSH
---

Telos comes with a basic shell and a few programs (mostly tests).  Type 'help'
at the prompt for a list of commands.

Git Repository
--------------

https://github.com/drewt/Telos

    $ git clone https://github.com/drewt/Telos.git

