
#define __ASM__
#include <kernel/i386.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

#define ISR_ENTRY(entry, handler)			\
	.global entry;					\
	entry:	SAVE_ALL;				\
		movl current,       %ebx;		\
		movl %eax,          PCB_RC(%ebx);	\
		movl %esp,          PCB_ESP(%ebx);	\
		movl PCB_PID(%ebx), %edi;		\
		call handler;				\
		jmp  return_from_interrupt

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


# entry point for the schedule() interrupt
.global schedule_entry
schedule_entry:
	SAVE_ALL
	movl current, %ebx
	movl %esp,    PCB_ESP(%ebx)
	call _schedule
	jmp  _switch_to

ISR_ENTRY(pgf_entry,   exn_page_fault)
ISR_ENTRY(fpe_entry,   exn_fpe)
ISR_ENTRY(ill_entry,   exn_ill_instr)
ISR_ENTRY(timer_entry, tick)
ISR_ENTRY(kbd_entry,   int_keyboard)

.global syscall_entry
syscall_entry:
	SAVE_ALL
	movl current,       %ebx
	movl PCB_PID(%ebx), %edi
	movl $-1,           PCB_RC(%ebx)
	cmpl NR_SYSCALLS,   %eax
	jae  return_from_interrupt
	movl %esp,          PCB_ESP(%ebx)
	call *systab(,%eax,4)
	movl %eax,          PCB_RC(%ebx)

return_from_interrupt:
	movl current,       %eax
	movl PCB_PID(%eax), %esi
	cmp  %edi,          %esi
	je   __switch_to
_switch_to:
	movl PCB_PGD(%eax), %ebx
	movl %ebx,          %cr3
__switch_to:
	movl PCB_ESP(%eax), %esp
	call handle_signal
	movl current,       %eax
	movl PCB_ESP(%eax), %esp
	movl PCB_IFP(%eax), %ebx
	movl PCB_RC(%eax),  %edx

	movl %edx,          EAX(%esp)
	movl $(tss + 4),    %ecx
	movl %ebx,          (%ecx)
skip_seg_set:
	RESTORE_ALL
	iret

# for C
.global switch_to
switch_to:
	movl 0x4(%esp), %eax
	jmp  _switch_to

