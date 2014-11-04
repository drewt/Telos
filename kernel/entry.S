
#define __ASM__
#include <kernel/i386.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

#define SET_SEGS(val, reg)	\
	movw $val, reg;		\
	movw reg, %ds;		\
	movw reg, %es;		\
	movw reg, %fs;		\
	movw reg, %gs

#define SAVE_ALL	\
	pushl %gs;	\
	pushl %fs;	\
	pushl %es;	\
	pushl %ds;	\
	pushl %ebp;	\
	pushl %ebp;	\
	pushl %eax;	\
	pushl %esi;	\
	pushl %edi;	\
	pushl %edx;	\
	pushl %ecx;	\
	pushl %ebx

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %edi;	\
	popl %esi;	\
	popl %eax;	\
	popl %ebp;	\
	popl %ebp;	\
	popl %ds;	\
	popl %es;	\
	popl %fs;	\
	popl %gs

.set EAX, 0x14

#define isr_entry(entry, handler)		\
.global entry;					\
entry:	SAVE_ALL;				\
	SET_SEGS(SEG_KDATA, %bx);		\
	movl current,       %ebx;		\
	movl %esp,          PCB_ESP(%ebx);	\
	call handler;				\
	jmp  return_to_user

isr_entry(pgf_entry,   exn_page_fault)
isr_entry(fpe_entry,   exn_fpe)
isr_entry(ill_entry,   exn_ill_instr)
isr_entry(timer_entry, tick)
isr_entry(kbd_entry,   int_keyboard)

.global syscall_entry
syscall_entry:
	SAVE_ALL
	SET_SEGS(SEG_KDATA, %bx)
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
	RESTORE_ALL
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
	SAVE_ALL
	movl %esp, PCB_ESP(%eax)
	call _schedule
	movl %eax, %ebx
	jmp _switch_to
schedule_return:
	movl current, %eax
	movl %ecx, PCB_ESP(%eax)
	movl PCB_RC(%eax), %eax
	ret
