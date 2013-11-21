/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <kernel/common.h>
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/interrupt.h>
#include <kernel/process.h>

/* ISR entry points */
void fpe_entry_point(void);
void ill_entry_point(void);
void pgf_entry_point(void);
void timer_entry_point(void);
void kbd_entry_point(void);
void syscall_entry_point(void);

void isr_init(void)
{
	set_gate(DBZ_EXN,	(ulong) fpe_entry_point,	SEG_KCODE);
	set_gate(ILLOP_EXN,	(ulong) ill_entry_point,	SEG_KCODE);
	set_gate(PF_EXN,	(ulong) pgf_entry_point,	SEG_KCODE);
	set_gate(TIMER_INTR,	(ulong) timer_entry_point,	SEG_KCODE);
	set_gate(KBD_INTR,	(ulong) kbd_entry_point,	SEG_KCODE);
	set_gate(SYSCALL_INTR,	(ulong) syscall_entry_point,	SEG_KCODE);
}

#define __STR(x) #x
#define STR(x) __STR(x)
#define ISR_ENTRY(entry,ident)				\
	#entry ": "					\
		"pusha				\n"	\
		"mov   %%eax,   %%edx		\n"	\
		"movl  $"STR(ident)", %%eax	\n"	\
		"jmp   common_isr		\n"

extern unsigned long _kernel_pgd;
extern unsigned long KERNEL_PAGE_OFFSET;

unsigned int context_switch(struct pcb *p)
{
	static void *ksp;	// kernel stack pointer
	void *psp;		// process stack pointer
	int  rc;		// syscall return code
	unsigned int iid;	// syscall/interrupt ID

	/* make sure TSS points to the right part of the stack */
	tss.esp0 = (unsigned long) p->ifp;

	asm volatile(
	".set EAX, 0x1C			\n"
	".set ECX, 0x18			\n"
	".set EDX, 0x14			\n"

	"pushf				\n" // save kernel context
	"pusha				\n"
	"movl  %%esp,   %[KSP]		\n" // switch stacks
	"movl  %[PSP],  %%esp		\n"
	"mov   %[PGD],  %%cr3		\n" // switch page directories
	"movl  %[RET],  EAX(%%esp)	\n" // syscall return code in %eax
	"cmp   $0x0,    %[SPR]		\n"
	"jne   skip_seg_set		\n"
	"movw  %[UDS],  %%ax		\n" // switch to user data segment
	"movw  %%ax,    %%ds		\n"
	"movw  %%ax,    %%es		\n"
	"movw  %%ax,    %%fs		\n"
	"movw  %%ax,    %%gs		\n"
"skip_seg_set: "
	"popa				\n" // restore user context
	"iret				\n" // return to user process

	ISR_ENTRY(pgf_entry_point,	PF_EXN)
	ISR_ENTRY(fpe_entry_point,	FPE_EXN)
	ISR_ENTRY(ill_entry_point,	ILL_EXN)
	ISR_ENTRY(timer_entry_point,	TIMER_INTR)
	ISR_ENTRY(kbd_entry_point,	KBD_INTR)
"syscall_entry_point: "
	"pusha				\n"
"common_isr: "
	"movw  %[KDS],  %%cx		\n" // switch to kernel data segment
	"movw  %%cx,    %%ds		\n"
	"movw  %%cx,    %%es		\n"
	"movw  %%cx,    %%fs		\n"
	"movw  %%cx,    %%gs		\n"
	"movl  %%esp,   %%ecx		\n" // switch stacks
	"movl  %[KSP],  %%esp		\n"
	"movl  %[KPD],  %%ebx		\n"
	"movl  %%ebx,   %%cr3		\n"
	"movl  %%eax,   EAX(%%esp)	\n" // save interrupt ID in %eax
	"movl  %%edx,   ECX(%%esp)	\n" // save old %eax value in %ecx
	"movl  %%ecx,   EDX(%%esp)	\n" // save user %esp in %edx
	"popa				\n" // restore kernel context
	"popf				\n"
	: "=a" (iid), "=c" (rc), "=d" (psp),
	/* read-write operands must be in memory */
	  [KSP] "+m" (ksp)
	/* values used only in the "top half" may be put in registers */
	: [PGD] "b" (p->pgdir), [SPR] "d" (p->flags & PFLAG_SUPER),
	  [RET] "a" (p->rc), [PSP] "c" (p->esp),
	/* "bottom half" values must be either immediate or in memory */
	  [UDS] "i" (SEG_UDATA | 3), [KDS] "i" (SEG_KDATA),
	  [KPD] "i" ((ulong) &_kernel_pgd - 0xC0000000)
	:);
	p->esp = psp;
	p->rc  = rc;

	return iid;
}
