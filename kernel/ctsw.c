/*  Copyright 2013 Drew Thoreson
 *
 *  This file is part of Telos.
 *  
 *  Telos is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, version 2 of the License.
 *
 *  Telos is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Telos.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	set_gate(EXN_DBZ,	(ulong) fpe_entry_point,	SEG_KCODE);
	set_gate(EXN_ILLOP,	(ulong) ill_entry_point,	SEG_KCODE);
	set_gate(EXN_PF,	(ulong) pgf_entry_point,	SEG_KCODE);
	set_gate(INTR_TIMER,	(ulong) timer_entry_point,	SEG_KCODE);
	set_gate(INTR_KBD,	(ulong) kbd_entry_point,	SEG_KCODE);
	set_gate(INTR_SYSCALL,	(ulong) syscall_entry_point,	SEG_KCODE);
}

#define __STR(x) #x
#define STR(x) __STR(x)
#define ISR_ENTRY(entry,ident)				\
	#entry ": "					\
		"pusha				\n"	\
		"mov   %%eax,   %%edx		\n"	\
		"movl  $"STR(ident)", %%eax	\n"	\
		"jmp   common_isr		\n"

/* Sets the segment registers to the given value */
#define SET_SEGS(val, reg)		\
	"movw "val", "reg"	\n"	\
	"movw "reg", %%ds	\n"	\
	"movw "reg", %%es	\n"	\
	"movw "reg", %%fs	\n"	\
	"movw "reg", %%gs	\n"

_Noreturn void switch_to(struct pcb *p)
{
	tss.esp0 = (ulong) p->ifp;

	asm volatile(
	".set EAX, 0x1C			\n"
	".set ECX, 0x18			\n"
	".set EDX, 0x14			\n"

	"movl  %[PSP],  %%esp		\n"
	"mov   %[PGD],  %%cr3		\n" // switch page directories
	"movl  %[RET],  EAX(%%esp)	\n" // syscall return code in %eax
	"cmp   $0x0,    %[SPR]		\n"
	"jne   skip_seg_set		\n"
	SET_SEGS("%[UDS]", "%%ax")
"skip_seg_set: "
	"popa				\n" // restore user context
	"iret				\n" // return to user process
	:
	: [PSP] "g" (p->esp),
	  [PGD] "g" (p->mm.pgdir),
	  [RET] "g" (p->rc),
	  [SPR] "g" (p->flags & PFLAG_SUPER),
	  [UDS] "i" (SEG_UDATA | 3)
	:);
}

_Noreturn void __context_switch(void)
{
	asm volatile(
	".set RC,  0x8			\n"
	".set PSP, 0xC			\n"
	ISR_ENTRY(pgf_entry_point,	EXN_PF)
	ISR_ENTRY(fpe_entry_point,	EXN_FPE)
	ISR_ENTRY(ill_entry_point,	EXN_ILL)
	ISR_ENTRY(timer_entry_point,	INTR_TIMER)
	ISR_ENTRY(kbd_entry_point,	INTR_KBD)
"syscall_entry_point: "
	"pusha				\n"
"common_isr: "
	SET_SEGS("%[KDS]", "%%bx")
	"movl  %%esp,    %%ecx		\n" // switch stacks
	"movl  $_kstack, %%esp		\n"
	"addl  $0x4000,  %%esp		\n"
	"movl  current,  %%ebx		\n"
	"movl  %%edx,    RC(%%ebx)	\n"
	"movl  %%ecx,    PSP(%%ebx)	\n"
	"movl  %%eax,    (%%esp)	\n"
	"call  dispatch			\n"
	"movl  current,  %%ebx		\n"
	"movl  %%ebx, (%%esp)		\n"
	"call  switch_to		\n"
	:
	: [KDS] "i" (SEG_KDATA)
	:);
}
