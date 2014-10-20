
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

#.set EAX, 0x1C
.set EAX, 0x14

.global switch_to
.global syscall_entry

ISR_ENTRY(pgf_entry,   EXN_PF)
ISR_ENTRY(fpe_entry,   EXN_FPE)
ISR_ENTRY(ill_entry,   EXN_ILL)
ISR_ENTRY(timer_entry, INTR_TIMER)
ISR_ENTRY(kbd_entry,   INTR_KBD)
syscall_entry:
	SAVE_ALL
common_isr:
	SET_SEGS(SEG_KDATA, %bx)
	movl %esp,          %ecx
	movl current,       %ebx
	movl PCB_KSP(%ebx), %esp
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
	movl PCB_IFP(%eax), %ebx
	movl PCB_ESP(%eax), %ecx
	movl PCB_RC(%eax),  %edx
	movl PCB_FLG(%eax), %eax
	and  $PFLAG_SUPER,  %eax

	movl %ecx,          %esp
	movl %edx,          EAX(%esp)
	movl $(tss + 4),    %ecx
	movl %ebx,          (%ecx)

	cmp  $0,            %eax
	jne  skip_seg_set
	SET_SEGS(SEG_UDATA | 3, %ax)
skip_seg_set:
	RESTORE_ALL
	iret

# for C
switch_to:
	movl 0x4(%esp), %eax
	jmp  _switch_to