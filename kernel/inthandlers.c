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

#include <kernel/common.h>
#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <signal.h>

/* names of some exceptions */
static char *exns[20] = {
	"Divide Error Exception",
	"Debug Exception",
	"NMI Interrupt",
	"Breakpoint Exception",
	"Overflow Exception",
	"BOUND Range Exceeded Exception",
	"Invalid Opcode Exception",
	"Device Not Available Exception",
	"Double Fault Exception",
	"Coprocessor Segment Overrun",
	"Invalid TSS Exception",
	"Segment Not Present",
	"Stack Fault Exception",
	"General Protection Exception",
	"Page Fault Exception",
	"",
	"x87 FPU Floating-Point Error",
	"Alignment Check Exception",
	"Machine-Check Exception",
	"SIMD Floating-Point Exception" //19
};

static void dump_registers(struct gp_regs *reg)
{
	kprintf("Dumping registers...\n");
	kprintf("\t%%eax=%lx\n", reg->eax);
	kprintf("\t%%ecx=%lx\n", reg->ecx);
	kprintf("\t%%edx=%lx\n", reg->edx);
	kprintf("\t%%ebx=%lx\n", reg->ebx);
	kprintf("\t%%esi=%lx\n", reg->esi);
	kprintf("\t%%edi=%lx\n", reg->edi);
	kprintf("\t%%esp=%lx\n", reg->esp);
	kprintf("\t%%ebp=%lx\n", reg->ebp);
}

static void print_cpu_context(struct ucontext *context)
{
	struct gp_regs *reg = (struct gp_regs*) &context->reg;
	kprintf("stack[0]=%lx\n", reg->stack[0]);
	kprintf("stack[1]=%lx\n", reg->stack[1]);
	kprintf("stack[2]=%lx\n", reg->stack[2]);
	kprintf("stack[3]=%lx\n", reg->stack[3]);
	kprintf("stack[4]=%lx\n", reg->stack[4]);
	kprintf("stack[5]=%lx\n", reg->stack[5]);
	kprintf("Current process is %d\n", current->pid);
	dump_registers(reg);
}

/*
 * XXX: grotesque hack to make up for dumb signal handling code
 *	which assumes no error code was pushed on the stack.
 */
static inline void exn_kill(struct ucontext *cx, int sig)
{
	cx->stack[0] = cx->stack[1];
	cx->stack[1] = cx->stack[2];
	__kill(current, sig);
}

void exn_page_fault(void)
{
	kprintf("page_fault! %d\n", current->pid);
	for(;;);
	ulong error, eip, addr;

	error = ((struct gp_regs*)current->esp)->stack[0];
	eip   = ((struct gp_regs*)current->esp)->stack[1];
	MOV("cr2", addr);

	kprintf("\nPage fault!\n");
	if (error & 1) {
	kprintf("\t%s-mode %s %lx\n",
			error & 4 ? "user" : "supervisor",
			error & 2 ? "write to" : "read from",
			addr);
	} else {
		kprintf("\tpage not present: %lx\n", addr);
	}
	kprintf("\teip=%lx\n\n", eip);

	dump_registers(current->esp);

	exn_kill(current->esp, SIGSEGV);
	ready(current);
	new_process();
}

void exn_fpe(void)
{
	kprintf("Arithmetic error\n");
	__kill(current, SIGFPE);
	ready(current);
	new_process();
}

void exn_ill_instr(void)
{
	kprintf("Illegal instruction\n");
	__kill(current, SIGILL);
	ready(current);
	new_process();
}

/* prints a nice message when something goes wrong */
void exn_err(unsigned int num, struct gp_regs reg)
{
	kprintf("\ntrap!\nException %d: %s\n", num,
			(num < 20) ? exns[num] : "Unknown");
	//print_cpu_context((struct ctxt*) &reg);
	print_cpu_context(current->esp);
}

#define MAKE_HANDLER(name,x)		\
static void name(void) {		\
	asm volatile(			\
	"pusha			\n"	\
	"pushl $"x"		\n"	\
	"call  exn_err		\n"	\
	"_hlt"x": "			\
		"hlt		\n"	\
		"jmp _hlt"x"	\n"	\
	);				\
}

MAKE_HANDLER(int0, "0")
MAKE_HANDLER(int1, "1")
MAKE_HANDLER(int2, "2")
MAKE_HANDLER(int3, "3")
MAKE_HANDLER(int4, "4")
MAKE_HANDLER(int5, "5")
MAKE_HANDLER(int6, "6")
MAKE_HANDLER(int7, "7")
MAKE_HANDLER(int8, "8")
MAKE_HANDLER(int9, "9")
MAKE_HANDLER(int10, "10")
MAKE_HANDLER(int11, "11")
MAKE_HANDLER(int12, "12")
MAKE_HANDLER(int13, "13")
MAKE_HANDLER(int14, "14")
MAKE_HANDLER(int15, "15")
MAKE_HANDLER(int16, "16")
MAKE_HANDLER(int17, "17")
MAKE_HANDLER(int18, "18")
MAKE_HANDLER(int19, "19")
MAKE_HANDLER(int20, "20")
MAKE_HANDLER(int21, "21")
MAKE_HANDLER(int22, "22")
MAKE_HANDLER(int23, "23")
MAKE_HANDLER(int24, "24")
MAKE_HANDLER(int25, "25")
MAKE_HANDLER(int26, "26")
MAKE_HANDLER(int27, "27")
MAKE_HANDLER(int28, "28")
MAKE_HANDLER(int29, "29")
MAKE_HANDLER(int30, "30")
MAKE_HANDLER(int31, "31")
MAKE_HANDLER(int32, "32")
MAKE_HANDLER(int33, "33")
MAKE_HANDLER(int34, "34")
MAKE_HANDLER(int35, "35")
MAKE_HANDLER(int36, "36")
MAKE_HANDLER(int37, "37")
MAKE_HANDLER(int38, "38")
MAKE_HANDLER(int39, "39")
MAKE_HANDLER(int40, "40")
MAKE_HANDLER(int41, "41")
MAKE_HANDLER(int42, "42")
MAKE_HANDLER(int43, "43")
MAKE_HANDLER(int44, "44")
MAKE_HANDLER(int45, "45")
MAKE_HANDLER(int46, "46")
MAKE_HANDLER(int47, "47")

typedef void(*isr_t)(void);

/* put them all in an array, why not */
void(*int_errs[48])(void) = {
	int0,  int1,  int2,  int3,  int4,  int5,  int6,  int7,  int8,  int9,
	int10, int11, int12, int13, int14, int15, int16, int17, int18, int19,
	int20, int21, int22, int23, int24, int25, int26, int27, int28, int29,
	int30, int31, int32, int33, int34, int35, int36, int37, int38, int39,
	int40, int41, int42, int43, int44, int45, int46, int47
};
