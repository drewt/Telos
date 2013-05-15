.global loader

# setting up the Multiboot header
.set ALIGN,    1<<0                 # align loaded modules on page boundaries
.set MEMINFO,  1<<1                 # provide memory map
.set FLAGS,    ALIGN | MEMINFO      # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002           # Multiboot magic number
.set CHECKSUM, -(MAGIC + FLAGS)     # checksum required

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# reserve initial kernel stack space
.set STACKSIZE, 0x4000              # 16k
.global stack
.lcomm stack, STACKSIZE             # reserve 16k stack

loader:
    movl $(stack + STACKSIZE), %esp # set up the stack
    pushl %eax                      # Multiboot magic number
    pushl %ebx                      # Multiboot info structure
    call kmain                      # call kernel proper
    cli

hang:
    hlt                             # halt machine should kernel return
    jmp hang

