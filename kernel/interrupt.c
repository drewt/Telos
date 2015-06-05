/*  Copyright 2013-2015 Drew Thoreson
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

#include <stdarg.h>

#include <kernel/i386.h>
#include <kernel/dispatch.h>
#include <kernel/signal.h>
#include <string.h>

static void dump_registers(struct gp_regs *reg)
{
	kprintf("Dumping registers...\n");
	kprintf("\t[eax] = 0x%lx\n", reg->eax);
	kprintf("\t[ecx] = 0x%lx\n", reg->ecx);
	kprintf("\t[edx] = 0x%lx\n", reg->edx);
	kprintf("\t[ebx] = 0x%lx\n", reg->ebx);
	kprintf("\t[esi] = 0x%lx\n", reg->esi);
	kprintf("\t[edi] = 0x%lx\n", reg->edi);
	kprintf("\t[esp] = 0x%lx\n", reg->esp);
	kprintf("\t[ebp] = 0x%lx\n", reg->ebp);
}

static void dump_context(struct ucontext *cx)
{
	kprintf("iret_eip    = 0x%lx\n", cx->iret_eip);
	kprintf("iret_cs     = 0x%lx\n", cx->iret_cs);
	kprintf("iret_elfags = 0x%lx\n", cx->eflags);
	if (!is_kernel_context(cx)) {
		kprintf("iret_esp    = 0x%lx\n", cx->iret_esp);
		kprintf("iret_ss     = 0x%lx\n", cx->iret_ss);
	}
	dump_registers(&cx->reg);
}

_Noreturn void _panic(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kvprintf(fmt, ap);
	va_end(ap);
	halt();
}

#include <kernel/mm/vma.h>

#define PGF_PERM  1
#define PGF_WRITE 2
#define PGF_USER  4

static unsigned long get_error_code(struct ucontext *cx)
{
	unsigned long error = cx->reg.stack[0];
	cx->reg.stack[0] = cx->reg.stack[1];
	cx->reg.stack[1] = cx->reg.stack[2];
	cx->reg.stack[2] = cx->reg.stack[3];
	if (!is_kernel_context(cx)) {
		cx->reg.stack[3] = cx->reg.stack[4];
		cx->reg.stack[4] = cx->reg.stack[5];
	}
	return error;
}

static void segfault(int code, unsigned long error, void *addr)
{
	kprintf("Segmentation fault: %s mode %s at 0x%p%s\n",
			(error & PGF_USER) ? "user" : "kernel",
			(error & PGF_WRITE) ? "write" : "read",
			addr,
			(error & PGF_PERM) ? "" : " (page not present)");
	dump_context(current->esp);
	if (is_kernel_context(current->esp))
		panic("Segfault in kernel");
	__kill(current, SIGSEGV, code);
}

void exn_page_fault(void)
{
	void *addr;
	struct vma *vma;
	unsigned long error;

	error = get_error_code(current->esp);
	MOV("cr2", addr);

	// address not mapped in process address space
	if (!(vma = vma_get(&current->mm, addr))) {
		segfault(SEGV_MAPERR, error, addr);
		return;
	}
	// page not present (demand paging)
	if (!(error & PGF_PERM)) {
		if (vm_map_page(vma, addr) < 0)
			// FIXME: stall until memory available?
			segfault(SEGV_MAPERR, error, addr);
		return;
	}
	// write permission
	if (error & PGF_WRITE) {
		if (vm_write_perm(vma, addr) < 0)
			segfault(SEGV_ACCERR, error, addr);
		return;
	}
	// read permission
	if (vm_read_perm(vma, addr) < 0)
		segfault(SEGV_ACCERR, error, addr);
}

void exn_fpe(void)
{
	kprintf("Arithmetic error\n");
	__kill(current, SIGFPE, FPE_INTDIV); // TODO: more codes
}

void exn_ill_instr(void)
{
	kprintf("Illegal instruction\n");
	__kill(current, SIGILL, ILL_ILLOPC); // FIXME: determine correct code
}

extern void exn0(void);
extern void exn1(void);
extern void exn2(void);
extern void exn3(void);
extern void exn4(void);
extern void exn5(void);
extern void exn6(void);
extern void exn7(void);
extern void exn8(void);
extern void exn9(void);
extern void exn10(void);
extern void exn11(void);
extern void exn12(void);
extern void exn13(void);
extern void exn14(void);
extern void exn15(void);
extern void exn16(void);
extern void exn17(void);
extern void exn18(void);
extern void exn19(void);
extern void exn20(void);
extern void fpe_entry(void);
extern void ill_entry(void);
extern void pgf_entry(void);
extern void timer_entry(void);
extern void kbd_entry(void);
extern void syscall_entry(void);
extern void schedule_entry(void);

struct exn {
	const char *name;
	bool has_code;
	void (*const handler)(void);
};

static const struct exn exns[] = {
	{
		.name = "Divide Error Exception",
		.has_code = false,
		.handler = fpe_entry,
	},{
		.name = "Debug Exception",
		.has_code = false,
		.handler = exn1,
	},{
		.name = "NMI Interrupt",
		.has_code = false,
		.handler = exn2,
	},{
		.name = "Breakpoint Exception",
		.has_code = false,
		.handler = exn3,
	},{
		.name = "Overflow Exception",
		.has_code = false,
		.handler = exn4,
	},{
		.name = "BOUND Range Exceeded Exception",
		.has_code = false,
		.handler = exn5,
	},{
		.name = "Invalid Opcode Exception",
		.has_code = false,
		.handler = ill_entry,
	},{
		.name = "Device Not Available Exception",
		.has_code = false,
		.handler = exn7,
	},{
		.name = "Double Fault Exception",
		.has_code = true,
		.handler = exn8,
	},{
		.name = "Coprocessor Segment Overrun",
		.has_code = false,
		.handler = exn9,
	},{
		.name = "Invalid TSS Exception",
		.has_code = true,
		.handler = exn10,
	},{
		.name = "Segment Not Present",
		.has_code = true,
		.handler = exn11,
	},{
		.name = "Stack Fault Exception",
		.has_code = true,
		.handler = exn12,
	},{
		.name = "General Protection Exception",
		.has_code = true,
		.handler = exn13,
	},{
		.name = "Page Fault Exception",
		.has_code = true,
		.handler = pgf_entry,
	},{
		.name = "Reserved",
		.has_code = false,
		.handler = exn15,
	},{
		.name = "x87 FPU Floating-Point Error",
		.has_code = false,
		.handler = exn16,
	},{
		.name = "Alignment Check Exception",
		.has_code = true,
		.handler = exn17,
	},{
		.name = "Machine-Check Exception",
		.has_code = false,
		.handler = exn18,
	},{
		.name = "SIMD Floating-Point Exception",
		.has_code = false,
		.handler = exn19,
	},{
		.name = "Virtualization Exception",
		.has_code = false,
		.handler = exn20,
	},
};
#define NR_EXNS (sizeof(exns)/sizeof(*exns))

/*
 * Generic interrupt handler: prints some useful info, then either panics
 * (if in-kernel) or kills the current process.
 */
void exn_err(unsigned int num, struct ucontext cx)
{
	kprintf("\ntrap!\nException %d (%s mode): %s\n", num,
			is_kernel_context(&cx) ? "kernel" : "user",
			(num < 20) ? exns[num].name : "Unknown");
	if (exns[num].has_code)
		kprintf("error code  = 0x%lx\n", get_error_code(&cx));
	dump_context(&cx);
	if (is_kernel_context(&cx))
		panic("Unhandled exception in kernel");
	__kill(current, SIGKILL, 0);
}

#define INTR_GATE 0xE
#define TRAP_GATE 0xF

struct idt_entry {
	unsigned int off_low : 16;
	unsigned int selector : 16;
	unsigned int reserved : 5;
	unsigned int zero : 3;
	unsigned int type : 5;
	unsigned int dpl : 2;
	unsigned int present : 1;
	unsigned int off_high : 16;
};

static struct idt_entry idt[256];

static void set_gate(unsigned int num, unsigned long handler,
		unsigned short selector, unsigned int dpl)
{
	idt[num] = (struct idt_entry) {
		.off_low  = handler & 0xFFFF,
		.selector = selector,
		.zero     = 0,
		.type     = INTR_GATE,
		.dpl      = dpl,
		.present  = 1,
		.off_high = handler >> 16,
	};
}

/* from osdev.org wiki inline asm examples */
static inline void load_idt(void *base, unsigned short size)
{
	volatile struct {
		uint16_t length;
		uint32_t base;
	} __packed idtr = { size, (u32) base };
	asm volatile("lidt (%0)" : : "g" (&idtr));
}

void idt_install(void)
{
	memset(idt, 0, sizeof(struct idt_entry) * 256);
	for (unsigned int i = 0; i < NR_EXNS; i++)
		set_gate(i, (unsigned long) exns[i].handler, SEG_KCODE, 2);
	set_gate(INTR_TIMER,    (ulong) timer_entry,    SEG_KCODE, 2);
	set_gate(INTR_KBD,      (ulong) kbd_entry,      SEG_KCODE, 2);
	set_gate(INTR_SYSCALL,  (ulong) syscall_entry,  SEG_KCODE, 3);
	load_idt(&idt, sizeof(struct idt_entry) * 256);
}
