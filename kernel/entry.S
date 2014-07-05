
#define __ASM__
#include <kernel/i386.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

#define ISR_ENTRY(entry, ident) 	\
	.global entry;			\
	entry:	 			\
		pusha;			\
		mov   %eax,   %edx;	\
		movl  $ident, %eax;	\
		jmp   common_isr

#define SET_SEGS(val, reg)	\
	movw $val, reg;		\
	movw reg, %ds;		\
	movw reg, %es;		\
	movw reg, %fs;		\
	movw reg, %gs

.set EAX, 0x1C

.global switch_to
.global syscall_entry

ISR_ENTRY(pgf_entry,   EXN_PF)
ISR_ENTRY(fpe_entry,   EXN_FPE)
ISR_ENTRY(ill_entry,   EXN_ILL)
ISR_ENTRY(timer_entry, INTR_TIMER)
ISR_ENTRY(kbd_entry,   INTR_KBD)
syscall_entry:
	pusha
common_isr:
	SET_SEGS(SEG_KDATA, %bx)
	movl %esp,     %ecx
	movl $_kstack, %esp
	addl $0x4000,  %esp
	movl current,  %ebx
	movl %edx,     PCB_RC(%ebx)
	movl %ecx,     PCB_ESP(%ebx)
	movl %eax,     (%esp)
	call dispatch
	movl current,  %ebx
	movl %ebx,     (%esp)
	call switch_to

# switch_to(struct pcb *p)
switch_to:
	# get pcb members from struct
	movl 0x4(%esp),     %eax
	movl PCB_IFP(%eax), %edx
	movl PCB_ESP(%eax), %ecx
	movl PCB_PGD(%eax), %ebx
	movl PCB_RC(%eax),  %esi
	movl PCB_FLG(%eax), %eax
	and  $PFLAG_SUPER,  %eax

	movl $(tss + 4),    %edi
	movl %edx,          (%edi)    # tss.esp0 = p->ifp
	movl %ecx,          %esp      # %esp     = p->esp
	movl %ebx,          %cr3      # %cr3     = p->mm.pgdir
	movl %esi,          EAX(%esp) # put p->rc in popa %eax

	cmp  $0,            %eax
	jne  skip_seg_set
	SET_SEGS(SEG_UDATA | 3, %ax)
skip_seg_set:
	popa
	iret
