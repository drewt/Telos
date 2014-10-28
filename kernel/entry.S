
#define __ASM__
#include <kernel/i386.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

#define ISR_ENTRY(entry, ident) 	\
	.global entry;			\
	entry:	 			\
		SAVE_ALL;		\
		mov   %eax,   %edx;	\
		movl  $ident, %eax;	\
		jmp   common_isr

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
	pushl %ebx;	\

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
	popl %gs;	\

#.set EAX, 0x1C
.set EAX, 0x14

.global switch_to
.global syscall_entry
.global schedule_entry

# entry point for the schedule() interrupt
schedule_entry:
	SAVE_ALL
	movl current, %ebx
	movl %esp,    PCB_ESP(%ebx)
	call _schedule
	jmp  _switch_to

ISR_ENTRY(pgf_entry,   EXN_PF)
ISR_ENTRY(fpe_entry,   EXN_FPE)
ISR_ENTRY(ill_entry,   EXN_ILL)
ISR_ENTRY(timer_entry, INTR_TIMER)
ISR_ENTRY(kbd_entry,   INTR_KBD)
syscall_entry:
	SAVE_ALL
common_isr:
	movl %esp,          %ecx
	movl current,       %ebx
	movl PCB_PID(%ebx), %edi
	movl %edx,          PCB_RC(%ebx)
	movl %ecx,          PCB_ESP(%ebx)
	subl $8,            %esp
	movl %ecx,          4(%esp)
	movl %eax,          (%esp)
	call dispatch
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
switch_to:
	movl 0x4(%esp), %eax
	jmp  _switch_to

