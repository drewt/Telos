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

#ifndef _KERNEL_I386_H_
#define _KERNEL_I386_H_

#define EFLAGS_IOPL(x) ((x) << 12)
#define EFLAGS_IF 0x0200

enum {
	SEGNR_NULL,
	SEGNR_KCODE,
	SEGNR_KDATA,
	SEGNR_UCODE,
	SEGNR_UDATA,
	SEGNR_TSS
};

enum {
	SEG_KCODE = SEGNR_KCODE << 3,
	SEG_KDATA = SEGNR_KDATA << 3,
	SEG_UCODE = SEGNR_UCODE << 3,
	SEG_UDATA = SEGNR_UDATA << 3,
	SEG_TSS   = SEGNR_TSS   << 3
};

/* general purpose registers (pusha ordering) */
struct gp_regs {
	unsigned long edi;
	unsigned long esi;
	unsigned long ebp;
	unsigned long esp;
	unsigned long ebx;
	unsigned long edx;
	unsigned long ecx;
	unsigned long eax;
	unsigned long stack[];
}; 

struct ucontext {
	struct gp_regs reg;
	unsigned long iret_eip;
	unsigned long iret_cs;
	unsigned long eflags;
	unsigned long iret_esp;
	unsigned long iret_ss;
	unsigned long stack[];
};

struct kcontext {
	struct gp_regs reg;
	unsigned long iret_eip;
	unsigned long iret_cs;
	unsigned long eflags;
	unsigned long stack[];
};

struct tss_entry {
	unsigned long prev;
	unsigned long esp0;
	unsigned long ss0;
	unsigned long esp1;
	unsigned long ss1;
	unsigned long esp2;
	unsigned long ss2;
	unsigned long cr3;
	unsigned long eip;
	unsigned long eflags;
	unsigned long eax;
	unsigned long ecx;
	unsigned long edx;
	unsigned long ebx;
	unsigned long esp;
	unsigned long ebp;
	unsigned long esi;
	unsigned long edi;
	unsigned long es;
	unsigned long cs;
	unsigned long ss;
	unsigned long ds;
	unsigned long fs;
	unsigned long gs;
	unsigned long ldt;
	unsigned short trap;
	unsigned short iomap_base;
} __attribute__((packed));

extern struct tss_entry tss;

/* wrapper for outb instruction */
static inline void outb(port_t port, unsigned char data)
{
	asm volatile("outb %1, %0" : : "d" (port), "a" (data));
}

/* wrapper for inb instruction */
static inline unsigned char inb(port_t port)
{
	unsigned char ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "d" (port));
	return ret;
}

static inline void halt(void)
{
	asm volatile("_halt: hlt\njmp _halt");
}

#define MOV(reg,loc) \
	asm volatile("mov %%"reg", %0" : "=g" (loc) : : )

#define GET_REG(reg,dst) \
	asm volatile("movl %%"reg", %0\n" : "=g" (dst) : : )

#define GET_SELECTOR(sel,dst) \
	asm volatile("movw %%"sel", %0\n" : "=g" (dst) : : )

/* from osdev.org wiki inline asm examples */
/*-----------------------------------------------------------------------------
 * Loads the interrupt descriptor table given by (base,size) */
//-----------------------------------------------------------------------------
static inline void load_idt(void *base, unsigned short size)
{
	volatile struct {
		u16 length;
		u32 base;
	} __attribute__((__packed__)) idtr = { size, (u32) base };
	asm volatile("lidt (%0)" : : "g" (&idtr));
}

/* from osdev.org wiki inline asm examples (modified to update selectors) */
/*-----------------------------------------------------------------------------
 * Loads the global descriptor table given by (base,size) and updates the
 * segment selectors */
//-----------------------------------------------------------------------------
static inline void load_gdt(void *base, unsigned short size)
{
	volatile struct {
		u16 length;
		u32 base;
	} __attribute__((__packed__)) gdtr = { size, (u32) base };
	asm volatile(
	"lgdt (%0)		\n"
	"ljmp %1,   $setcs	\n"
"setcs:				\n"
	"movw %2,   %%ax	\n"
	"movw %%ax, %%ds	\n" 
	"movw %%ax, %%es	\n"
	"movw %%ax, %%ss	\n"
	:
	: "g" (&gdtr), "i" (SEG_KCODE), "i" (SEG_KDATA)
	: "%eax"
	);
}

/*-----------------------------------------------------------------------------
 * Loads the TSS register with a given value */
//-----------------------------------------------------------------------------
static inline void load_tss(unsigned short val)
{
	asm volatile(
	"movw %0, %%ax	\n"
	"ltr  %%ax	\n"
	: : "g" (val) : "%eax"
	);
}

static inline void enable_paging(void)
{
	asm volatile(
	"mov %%cr0, %%eax\n"
	"or  %0,    %%eax\n"
	"mov %%eax, %%cr0\n"
	: : "i" (0x80000000) : "%eax"
	);
}

static inline void disable_paging(void)
{
	asm volatile(
	"mov %%cr0, %%eax\n"
	"and %0,    %%eax\n"
	"mov %%eax, %%cr0\n"
	: : "i" (~0x80000000) : "%eax"
	);
}

static inline void set_page_directory(void *addr)
{
	asm volatile("mov %0, %%cr3" : : "r" (addr) :);
}

static inline void put_iret_uframe(struct ucontext *f, unsigned long eip,
		unsigned long esp)
{
	f->iret_cs  = SEG_UCODE | 3;
	f->iret_eip = eip;
	f->eflags   = EFLAGS_IOPL(0) | EFLAGS_IF;
	f->iret_esp = esp;
	f->iret_ss  = SEG_UDATA | 3;
}

static inline void put_iret_kframe(struct kcontext *f, unsigned long eip)
{
	f->iret_eip = eip;
	f->iret_cs  = SEG_KCODE;
	f->eflags   = EFLAGS_IOPL(3) | EFLAGS_IF;
}

extern void gdt_install(void);
extern void idt_install(void);
extern void set_gate(unsigned int num, unsigned long handler,
		unsigned short selector);
extern void pic_init(u16 off1, u16 off2);
extern void enable_irq(unsigned char irq, bool disable);
extern void pic_eoi(void);
extern void pit_init(int div);

struct tm;

void rtc_date(struct tm *date);
void clock_init(void);

#endif
