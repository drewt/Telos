.global loader
.global _kernel_pgd
.global _kernel_high_pgt
.global _kernel_low_pgt
.global stack

.set STACKSIZE, 0x4000
.set NR_LOW_PGTS, 16

.section .bss

.align 0x1000
_kernel_pgd:      .space 0x1000
_kernel_low_pgt:  .space (0x1000 * NR_LOW_PGTS)
_kernel_high_pgt: .space 0x1000

.section .data
stack:            .space STACKSIZE

.section .text

.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

loader:
    mov  $(stack + STACKSIZE), %esp
    mov  %esp, %ebp

    # save multiboot data
    push %eax
    push %ebx

    call boot_init_paging
    call kmain
    cli

hang:
    hlt
    jmp hang

boot_init_paging:

    mov  $_kernel_pgd, %eax
    mov  $_kernel_low_pgt, %ebx
    or   $0x7, %ebx
    mov  $0x0, %edx

    # initialize page directory
    _set_pgt_loop:
        mov  %ebx, (%eax, %edx, 4)
        add  $0x1000, %ebx
        inc  %edx
        cmp  $NR_LOW_PGTS, %edx
        jl   _set_pgt_loop
    
    # initialize page tables
    mov  $_kernel_low_pgt, %eax
    mov  $0x0, %ebx
    mov  $(0x400000 * NR_LOW_PGTS), %ecx
    call boot_direct_map

    # map kernel in higher half
    mov  $_kernel_pgd, %eax
    mov  $_kernel_high_pgt, %ebx
    or   $0x7, %ebx
    mov  $KERNEL_PAGE_OFFSET, %ecx
    shr  $22, %ecx
    mov  %ebx, (%eax, %ecx, 4)

    mov  $_kernel_high_pgt, %eax
    mov  $0x0, %ebx
    mov  $0x400000, %ecx
    call boot_direct_map

    # enable paging
    mov  $_kernel_pgd, %eax
    mov  %eax, %cr3
    mov  %cr0, %eax
    or   $(1 << 31), %eax
    mov  %eax, %cr0

    ret

#
# void boot_direct_map (pgt, base, len)
#
#       %eax: page table base address
#       %ebx: base physical address
#       %ecx: amount of memory to map
boot_direct_map:

    add  %ebx, %ecx
    or   $0x7, %ebx

    _map_loop:
        mov  %ebx, (%eax)
        add  $0x4, %eax
        add  $0x1000, %ebx
        cmp  %ecx, %ebx
        jl   _map_loop
    ret
