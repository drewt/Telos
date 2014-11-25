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

#define SEGNR_NULL  0
#define SEGNR_KCODE 1
#define SEGNR_KDATA 2
#define SEGNR_UCODE 3
#define SEGNR_UDATA 4
#define SEGNR_TSS   5

#define SEG_KCODE (SEGNR_KCODE << 3)
#define SEG_KDATA (SEGNR_KDATA << 3)
#define SEG_UCODE (SEGNR_UCODE << 3)
#define SEG_UDATA (SEGNR_UDATA << 3)
#define SEG_TSS   (SEGNR_TSS   << 3)

#define EXN_DBZ   0x00
#define EXN_DBG   0x01
#define EXN_NMI   0x02
#define EXN_BP    0x03
#define EXN_OF    0x04
#define EXN_BRE   0x05
#define EXN_ILLOP 0x06
#define EXN_DNA   0x07
#define EXN_DF    0x08
#define EXN_CSO   0x09
#define EXN_TSS   0x0A
#define EXN_SNP   0x0B
#define EXN_SF    0x0C
#define EXN_GP    0x0D
#define EXN_PF    0x0E
#define EXN_RSRV  0x0F
#define EXN_FPU   0x10
#define EXN_AC    0x11
#define EXN_MC    0x12
#define EXN_SIMD  0x13

#define INTR_TIMER   0x20
#define INTR_KBD     0x21
#define INTR_SYSCALL 0x80

#define EBX 0x00
#define ECX 0x04
#define EDX 0x08
#define EDI 0x0C
#define ESI 0x10
#define EAX 0x14
#define EBP 0x18
#define ESP 0x1C
#define DS  0x20
#define ES  0x24
#define FS  0x28
#define GS  0x2C

#ifndef __ASSEMBLER__

#define EFLAGS_IOPL(x) ((x) << 12)
#define EFLAGS_IF 0x0200

/* general purpose registers (SAVE_ALL ordering) */
struct gp_regs {
	unsigned long ebx;
	unsigned long ecx;
	unsigned long edx;
	unsigned long edi;
	unsigned long esi;
	unsigned long eax;
	unsigned long ebp;
	unsigned long esp;
	unsigned long ds;
	unsigned long es;
	unsigned long fs;
	unsigned long gs;
	unsigned long stack[];
};

#define assert_reg_offset(member, offset) \
	assert_struct_offset(struct gp_regs, member, offset)
assert_reg_offset(ebx, EBX);
assert_reg_offset(ecx, ECX);
assert_reg_offset(edx, EDX);
assert_reg_offset(edi, EDI);
assert_reg_offset(esi, ESI);
assert_reg_offset(eax, EAX);
assert_reg_offset(ebp, EBP);
assert_reg_offset(esp, ESP);
assert_reg_offset(ds,  DS);
assert_reg_offset(es,  ES);
assert_reg_offset(fs,  FS);
assert_reg_offset(gs,  GS);

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
} __packed;

extern struct tss_entry tss;

static inline bool is_kernel_context(struct ucontext *cx)
{
	return cx->iret_cs >> 3 == SEGNR_KCODE;
}

static inline void outb(port_t port, unsigned char data)
{
	asm volatile("outb %1, %0" : : "d" (port), "a" (data));
}

static inline unsigned char inb(port_t port)
{
	unsigned char ret;
	asm volatile("inb %1, %0" : "=a" (ret) : "d" (port));
	return ret;
}

static inline _Noreturn void halt(void)
{
	asm volatile("_halt: hlt\njmp _halt");
	__builtin_unreachable();
}

#define MOV(reg,loc) \
	asm volatile("mov %%"reg", %0" : "=g" (loc) : : )

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

static inline void enable_write_protect(void)
{
	asm volatile(
		"mov %%cr0, %%eax\n"
		"or  %0,    %%eax\n"
		"mov %%eax, %%cr0\n"
	: : "i" (1 << 16) : "%eax"
	);
}

static inline void disable_write_protect(void)
{
	asm volatile(
		"mov %%cr0, %%eax\n"
		"and %0,    %%eax\n"
		"mov %%eax, %%cr0\n"
	: : "i" (~(1 << 16)) : "%eax"
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

	f->reg.ds = SEG_UDATA | 3;
	f->reg.es = SEG_UDATA | 3;
	f->reg.fs = SEG_UDATA | 3;
	f->reg.gs = SEG_UDATA | 3;
}

static inline void put_iret_kframe(struct kcontext *f, unsigned long eip)
{
	f->iret_eip = eip;
	f->iret_cs  = SEG_KCODE;
	f->eflags   = EFLAGS_IOPL(3) | EFLAGS_IF;

	f->reg.ds = SEG_KDATA;
	f->reg.es = SEG_KDATA;
	f->reg.fs = SEG_KDATA;
	f->reg.gs = SEG_KDATA;
}

extern void gdt_install(void);
extern void idt_install(void);
extern void pic_init(u16 off1, u16 off2);
extern void enable_irq(unsigned char irq, bool disable);
extern void pic_eoi(void);
extern void pit_init(int div);

struct tm;

void rtc_date(struct tm *date);
void clock_init(void);

#endif /* __ASSEMBLER__ */
#endif /* _KERNEL_I386_H_ */
