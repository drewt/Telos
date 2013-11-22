Telos
=====

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

