
#define __ASM__
#include <kernel/i386.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

.macro set_segs val reg
	movw \val, \reg
	movw \reg, %ds
	movw \reg, %es
	movw \reg, %fs
	movw \reg, %gs
.endm

.macro save_all
	pushl %gs
	pushl %fs
	pushl %es
	pushl %ds
	pushl %ebp
	pushl %ebp
	pushl %eax
	pushl %esi
	pushl %edi
	pushl %edx
	pushl %ecx
	pushl %ebx
.endm

.macro restore_all
	popl %ebx
	popl %ecx
	popl %edx
	popl %edi
	popl %esi
	popl %eax
	popl %ebp
	popl %ebp
	popl %ds
	popl %es
	popl %fs
	popl %gs
.endm

.macro isr_entry entry handler
.global \entry
\entry:	save_all
	set_segs $SEG_KDATA, %bx
	movl current, %ebx
	movl %esp, PCB_ESP(%ebx)
	call \handler
	jmp return_to_user
.endm

.set EAX, 0x14

isr_entry pgf_entry,   exn_page_fault
isr_entry fpe_entry,   exn_fpe
isr_entry ill_entry,   exn_ill_instr
isr_entry timer_entry, tick
isr_entry kbd_entry,   int_keyboard

.global syscall_entry
syscall_entry:
	save_all
	set_segs $SEG_KDATA, %bx
	movl current,       %ebx
	movl $-1,           EAX(%esp)
	cmpl NR_SYSCALLS,   %eax
	jae  return_to_user
	movl %esp,          PCB_ESP(%ebx)
	call *systab(,%eax,4)
	movl %eax,          EAX(%esp)
return_to_user:
	call handle_signal
_return_to_user:
	movl PCB_ESP(%ebx), %esp
	movl PCB_IFP(%ebx), %ebx
	movl %ebx,          (tss + 4)
	restore_all
	iret

.global switch_to
switch_to:
	movl 0x4(%esp),     %ebx
_switch_to:
	movl PCB_PGD(%ebx), %eax
	movl %eax,          %cr3
	jmp _return_to_user

.global schedule
schedule:
	movl current, %eax
	movl PCB_ESP(%eax), %ecx
	pushfl
	pushl %cs
	pushl $schedule_return
	save_all
	movl %esp, PCB_ESP(%eax)
	call _schedule
	movl %eax, %ebx
	jmp _switch_to
schedule_return:
	movl current, %eax
	movl %ecx, PCB_ESP(%eax)
	movl PCB_RC(%eax), %eax
	ret

.global sys_exit
sys_exit:
	movl  0x4(%esp), %eax
	movl  current,   %ebx
	movl  $(_kstack + 0x4000), %esp
	subl  $0x8, %esp
	pushl %eax
	pushl %ebx
	call  do_exit
	call  schedule
