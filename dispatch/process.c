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

#include <kernel/i386.h>
#include <kernel/elf.h>
#include <kernel/dispatch.h>
#include <kernel/time.h>
#include <kernel/mmap.h>
#include <kernel/list.h>
#include <kernel/fs.h>
#include <kernel/signal.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>
#include <kernel/mm/vma.h>
#include <sys/stat.h>
#include <sys/exec.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <syscall.h>
#include <string.h>

extern void exit(int status);

struct pcb proctab[PT_SIZE];

struct proc_status {
	struct list_head chain;
	pid_t pid;
	int status;
};
DEFINE_SLAB_CACHE(proc_status_cachep, sizeof(struct proc_status));

static inline int status_to_si_code(int status)
{
	switch (WHOW(status)) {
	case _WCONTINUED: return CLD_CONTINUED;
	case _WEXITED:    return CLD_EXITED;
	case _WSIGNALED:  return CLD_KILLED;
	case _WSTOPPED:   return CLD_STOPPED;
	}
	return 0;
}

static void assert_status(struct pcb *parent, struct proc_status *status)
{
	list_add_tail(&status->chain, &parent->child_stats);
	__kill(parent, SIGCHLD, status_to_si_code(status->status));
}

void report_status(struct pcb *p, int how, int val)
{
	struct pcb *parent;
	struct proc_status *status;

	if (!(parent = get_pcb(p->parent_pid)))
		panic("Process %d has no parent", p->pid);
	if (!(status = slab_alloc(proc_status_cachep)))
		return; // FIXME: do something
	status->pid = p->pid;
	status->status = make_status(how, val);
	assert_status(parent, status);
}

SYSINIT(proctab, SUB_PROCESS)
{
	for (int i = 0; i < PT_SIZE; i++) {
		proctab[i].pid   = i - PT_SIZE;
		proctab[i].state = PROC_DEAD;
	}
	proctab[0].pid = 0; // 0 is a reserved pid
}

static struct pcb *pcb_common_init(struct pcb *p)
{
	p->t_alarm.flags = 0;
	p->t_sleep.flags = 0;
	p->timestamp = tick_count;
	p->state = PROC_NASCENT;
	INIT_LIST_HEAD(&p->children);
	INIT_LIST_HEAD(&p->child_stats);
	INIT_LIST_HEAD(&p->posix_timers);
	return p;
}

static struct pcb *get_free_pcb(void)
{
	for (int i = 0; i < PT_SIZE; i++)
		if (proctab[i].state == PROC_DEAD)
			return pcb_common_init(&proctab[i]);
	return NULL;
}

static struct pcb *pcb_clone(struct pcb *src)
{
	struct pcb *p;

	if (!(p = get_free_pcb()))
		return NULL;

	p->esp = src->esp;
	p->ifp = src->ifp;

	sig_clone(&p->sig, &src->sig);

	for (int i = 0; i < NR_FILES; i++)
		if ((p->filp[i] = src->filp[i]))
			p->filp[i]->f_count++;

	p->pid += PT_SIZE;
	p->parent_pid = src->pid;
	p->flags = src->flags;

	p->root = src->root;
	p->pwd = src->pwd;

	list_add_tail(&p->child_chain, &src->children);
	return p;
}

#define DATA_FLAGS   (VM_READ | VM_WRITE | VM_KEEP)
#define RODATA_FLAGS (VM_READ | VM_KEEP)
#define HEAP_FLAGS   (VM_READ | VM_WRITE | VM_ZERO)
#define USTACK_FLAGS (VM_READ | VM_WRITE | VM_EXEC | VM_ZERO | VM_KEEP | VM_KEEPEXEC)

struct vma *get_heap(struct mm_struct *mm)
{
	struct vma *heap;
	if ((heap = vma_get(mm, (void*)(mm->brk-1))))
		return heap;
	return vma_map(mm, mm->brk-1, 1, HEAP_FLAGS);
}

static int address_space_init(struct mm_struct *mm)
{
	mm->brk = HEAP_START + HEAP_SIZE;
	if (!vma_map(mm, HEAP_START, HEAP_SIZE, HEAP_FLAGS))
		goto abort;
	if (!vma_map(mm, STACK_START, STACK_SIZE, USTACK_FLAGS))
		goto abort;
	if (!vma_map(mm, urostart, uroend - urostart, RODATA_FLAGS))
		goto abort;
	return 0;
abort:
	return -ENOMEM;
}

#define KFRAME_ROOM (sizeof(struct kcontext) + 16)

int create_kernel_process(void(*func)(void*), void *arg, ulong flags)
{
	int error;
	struct pcb *p;
	unsigned char cx_mem[KFRAME_ROOM];
	struct kcontext *frame = (struct kcontext*) cx_mem;

	if (!(p = get_free_pcb()))
		return -EAGAIN;
	sig_init(&p->sig);
	for (int i = 0; i < NR_FILES; i++)
		p->filp[i] = NULL;
	p->pid += PT_SIZE;
	p->parent_pid = 1;
	p->flags = flags | PFLAG_SUPER;
	p->root = root_inode;
	p->pwd = root_inode;

	if ((error = mm_init(&p->mm)) < 0)
		return error;;
	if ((error = address_space_init(&p->mm)) < 0) {
		mm_fini(&p->mm);
		return error;
	}
	if (!vma_map(&p->mm, krostart, kroend - krostart, RODATA_FLAGS)) {
		mm_fini(&p->mm);
		return -ENOMEM;
	}

	p->esp = ((char*)KSTACK_END - KFRAME_ROOM);

	put_iret_kframe(frame, (uintptr_t)func);
	frame->stack[0] = (ulong) exit;
	frame->stack[1] = (ulong) arg;
	pm_copy_to(p->mm.pgdir, p->esp, frame, KFRAME_ROOM);

	ready(p);
	return p->pid;
}

/*
 * The init process: execs the real init at /bin/init.
 *
 * The reason for this stub is that the exec code assumes it has a running
 * process--so create_init cannot use that code.  Instead, it creates a
 * process with this function as the entry point.
 */
void __user init(void *arg)
{
	char name[] = "/bin/init";
	struct _String argv[1] = {{ .str = name, .len = 9 }};
	struct _String envp[1] = {{ .str = name, .len = 9 }};
	struct exec_args e_args = {
		.pathname = {
			.str = name,
			.len = 9
		},
		.argc = 1,
		.envc = 1,
		.argv = argv,
		.envp = envp
	};
	syscall1(SYS_EXECVE, &e_args);
}

void create_init(void)
{
	int error;
	long esp;
	struct pcb *p;

	p = pcb_common_init(&proctab[1]);
	sig_init(&p->sig);
	for (int i = 0; i < NR_FILES; i++)
		p->filp[i] = NULL;
	p->pid = 1;
	p->parent_pid = 1;
	p->flags = 0;
	p->root = root_inode;
	p->pwd = root_inode;

	if ((error = mm_init(&p->mm)) < 0)
		panic("mm_init failed with error %d", error);
	if ((error = address_space_init(&p->mm)) < 0)
		panic("address_space_init failed with error %d", error);

	p->ifp = (char*)KSTACK_END - 16;
	p->esp = (char*)p->ifp - sizeof(struct ucontext);

	current = p;
	set_page_directory((void*)p->mm.pgdir);
	esp = STACK_END - 16;
	put_iret_uframe(p->esp, (uintptr_t)init, esp);

	ready(p);
}

static int verify_exec_args(struct exec_args *args)
{
	int error;

	if (vm_verify(&current->mm, args, sizeof(*args), VM_READ))
		return -EFAULT;
	if (vm_verify(&current->mm, args->argv,
				sizeof(*(args->argv)) * args->argc, VM_READ))
		return -EFAULT;
	if (vm_verify(&current->mm, args->envp,
				sizeof(*(args->envp)) * args->envc, VM_READ))
		return -EFAULT;
	error = verify_user_string(args->pathname.str, args->pathname.len);
	if (error)
		return error;
	for (size_t i = 0; i < args->argc; i++) {
		error = verify_user_string(args->argv[i].str, args->argv[i].len);
		if (error)
			return error;
	}
	for (size_t i = 0; i < args->envc; i++) {
		error = verify_user_string(args->envp[i].str, args->envp[i].len);
		if (error)
			return error;
	}
	return 0;
}

static size_t exec_mem_needed(struct exec_args *args)
{
	size_t size = 0;
	size += (args->argc + 1) * sizeof(char*);
	size += (args->envc + 1) * sizeof(char*);
	for (size_t i = 0; i < args->argc; i++)
		size += args->argv[i].len + 1;
	for (size_t i = 0; i < args->envc; i++)
		size += args->envp[i].len + 1;
	return size;
}

/*
 * Copy the argv and envp arrays into kernel memory, so they are not lost when
 * the address space is unmapped.
 */
static int execve_copy_to_kernel(struct exec_args *args, void **ptr,
		size_t *size_out)
{
	size_t size = exec_mem_needed(args);
	char *mem, *kmem = kmalloc(size);
	if (!kmem)
		return -ENOMEM;
	mem = (char*) ((char**) kmem + args->argc + args->envc + 2);
	for (size_t i = 0; i < args->argc; i++) {
		memcpy(mem, args->argv[i].str, args->argv[i].len + 1);
		mem += args->argv[i].len + 1;
	}
	for (size_t i = 0; i < args->envc; i++) {
		memcpy(mem, args->envp[i].str, args->envp[i].len + 1);
		mem += args->envp[i].len + 1;
	}
	*ptr = kmem;
	*size_out = size;
	return 0;
}

/*
 * Copy the argv and envp arrays from kernel memory back into the process's
 * address space.  A VMA is created for this purpose.
 */
static int execve_copy_to_user(struct exec_args *args, void *kmem, size_t size,
		void **argv_out, void **envp_out)
{
	char *mem;
	char **argv, **envp;
	struct vma *vma;

	vma = vma_create_high(&current->mm, user_base, kernel_base, size,
			VM_READ | VM_WRITE | VM_ZERO);
	if (!vma)
		return -ENOMEM;

	// copy back into user memory and populate argv/envp
	memcpy((void*) vma->start, kmem, size);
	argv = (char**) vma->start;
	envp = argv + args->argc + 1;
	mem = (char*) (envp + args->envc + 1);
	for (size_t i = 0; i < args->argc; i++) {
		argv[i] = mem;
		mem += args->argv[i].len + 1;
	}
	argv[args->argc] = NULL;
	for (size_t i = 0; i < args->envc; i++) {
		envp[i] = mem;
		mem += args->envp[i].len + 1;
	}
	envp[args->envc] = NULL;

	*argv_out = argv;
	*envp_out = envp;
	kfree(kmem);
	return 0;

}

static bool elf_header_valid(struct elf32_hdr *hdr)
{
	static const char elf_magic[] = { 0x7F, 'E', 'L', 'F' };
	if (memcmp(hdr->ident, elf_magic, 4))
		return false;
	if (hdr->ident[EI_CLASS] != ELF_CLASS_32)
		return false;
	if (hdr->ident[EI_DATA] != ELF_DATA_LSB)
		return false;
	return true;
}

static int elf_read_program_header(struct file *file, struct elf32_phdr *hdr,
		unsigned long phoff)
{
	ssize_t bytes;
	bytes = file->f_op->read(file, (char*)hdr, sizeof(*hdr), &phoff);
	if (bytes < 0)
		return bytes;
	if ((size_t)bytes < sizeof(*hdr))
		return -EINVAL;
	return 0;
}

static int elf_program_header_valid(struct elf32_phdr *hdr)
{
	if (hdr->type == ELF_PHTYPE_DYNAMIC ||
			hdr->type == ELF_PHTYPE_INTERP ||
			hdr->type == ELF_PHTYPE_SHLIB)
		return false;
	return true;
}

#if 1
static int elf_map_segment(struct file *file, struct elf32_phdr *hdr)
{
	void *vaddr = (void*)hdr->vaddr;
	int error = do_mmap(file, &vaddr, hdr->memsz, hdr->flags & 7,
			MAP_PRIVATE | MAP_FIXED, hdr->offset);
	if (error)
		return error;
	if (hdr->memsz > hdr->filesz)
		memset((char*)vaddr + hdr->filesz, 0, hdr->memsz - hdr->filesz);
	return 0;
}
#else
static int elf_map_segment(struct file *file, struct elf32_phdr *hdr)
{
	ssize_t bytes;
	unsigned long pos = hdr->offset;
	struct vma *vma = vma_create_fixed(&current->mm, hdr->vaddr,
			hdr->memsz, hdr->flags & 7);
	if (!vma)
		return -ENOMEM;
	disable_write_protect();
	bytes = file->f_op->read(file, (char*)vma->start, hdr->filesz, &pos);
	enable_write_protect();
	if (bytes < 0)
		return bytes;
	if ((size_t)bytes < hdr->filesz)
		return -EINVAL;
	if (hdr->memsz > hdr->filesz) {
		disable_write_protect();
		memset((char*)vma->start + hdr->filesz, 0, hdr->memsz - hdr->filesz);
		enable_write_protect();
	}
	return 0;
}
#endif

static void *load_elf(struct inode *inode)
{
	int error;
	ssize_t bytes;
	struct file *file;
	struct elf32_hdr hdr;
	unsigned long pos = 0;

	error = file_open(inode, O_RDONLY, 0, &file);
	if (error)
		return NULL;
	if (!file->f_op || !file->f_op->read)
		return NULL;
	bytes = file->f_op->read(file, (char*)&hdr, sizeof(hdr), &pos);
	if (bytes < 0 || (size_t)bytes < sizeof(hdr))
		return NULL;
	if (!elf_header_valid(&hdr))
		return NULL;

	pos = hdr.phoff;
	for (int i = 0; i < hdr.phnum; i++, pos += hdr.phentsize) {
		struct elf32_phdr phdr;
		error = elf_read_program_header(file, &phdr, pos);
		if (error)
			return NULL;
		if (!elf_program_header_valid(&phdr))
			return NULL;
		if (phdr.type != ELF_PHTYPE_LOAD)
			continue;
		error = elf_map_segment(file, &phdr);
		if (error)
			return NULL;
	}
	return (void*)hdr.entry;
}

long sys_execve(struct exec_args *args)
{
	int error;
	size_t size;
	void *ptr;
	void *argv, *envp, *entry;
	unsigned long esp;
	unsigned long *main_args;
	struct inode *inode;

	error = verify_exec_args(args);
	if (error)
		return error;
	error = namei(args->pathname.str, &inode);
	if (error)
		return error;
	if (!S_ISREG(inode->i_mode))
		return -EACCES;
	error = execve_copy_to_kernel(args, &ptr, &size);
	if (error)
		return error;
	mm_exec(&current->mm);
	error = execve_copy_to_user(args, ptr, size, &argv, &envp);
	if (error)
		return error; // FIXME: all memory is unmapped at this point!

	// load process image
	entry = load_elf(inode);

	if (!entry)
		return -ENOMEM; // FIXME: wrong, will segfault (as above)

	sig_exec(&current->sig);
	current->ifp = (char*) KSTACK_END - 16;
	current->esp = (char*) current->ifp - sizeof(struct ucontext);

	esp = STACK_END - 16;
	put_iret_uframe(current->esp, (uintptr_t)entry, esp);

	main_args = (unsigned long*) esp;
	main_args[0] = args->argc;
	main_args[1] = (unsigned long) argv;
	main_args[2] = (unsigned long) envp;

	return 0;
}

long sys_fork(void)
{
	int rc;
	struct pcb *p;

	// set return value to zero for child
	((struct ucontext*)current->esp)->reg.eax = 0;

	if (!(p = pcb_clone(current)))
		return -EAGAIN;

	if ((rc = mm_clone(&p->mm, &current->mm)) < 0)
		return rc;

	ready(p);
	return p->pid;
}

long sys_yield(void)
{
	ready(current);
	schedule();
	return 0;
}

void do_exit(struct pcb *p, int status)
{
	struct pcb *pit;
	struct proc_status *sit, *n;

	report_status(p, _WEXITED, status);

	for (int i = 0; i < NR_FILES; i++)
		if (current->filp[i] != NULL)
			sys_close(i);

	// re-parent orphans to init
	list_for_each_entry(pit, &p->children, child_chain) {
		pit->parent_pid = 1;
	}
	// give status events to new parent
	if (!(pit = get_pcb(1)))
		panic("No init process");
	list_for_each_entry_safe(sit, n, &p->child_stats, chain) {
		list_del(&sit->chain);
		assert_status(pit, sit);
	}

	mm_fini(&p->mm);
	zombie(p);
}

long sys_getpid(void)
{
	return current->pid;
}

long sys_getppid(void)
{
	return current->parent_pid;
}

static struct proc_status *get_status(idtype_t idtype, id_t id, int options)
{
	struct proc_status *s;
	list_for_each_entry(s, &current->child_stats, chain) {
		// TODO: P_PGID
		if (idtype == P_PID && s->pid != (pid_t)id)
			continue;
		switch (WHOW(s->status)) {
		case _WCONTINUED:
			if (options & WCONTINUED)
				return s;
			break;
		case _WEXITED:
			if (options & WEXITED)
				return s;
			break;
		case _WSTOPPED:
			if (options & WSTOPPED)
				return s;
			break;
		default:
			return s;
		}
	}
	return NULL;
}

long sys_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options)
{
	int signo;
	struct proc_status *s;

	if (vm_verify(&current->mm, infop, sizeof(*infop), VM_WRITE))
		return -EFAULT;
	if (list_empty(&current->children))
		return -ECHILD;
	for (;;) {
		if ((s = get_status(idtype, id, options)))
			break;
		if (options & WNOHANG) {
			infop->si_signo = 0;
			infop->si_pid = 0;
			return 0;
		}

		current->state = PROC_WAITING;
		if ((signo = schedule()) != SIGCHLD)
			return -EINTR;
		if (signal_from_user(current->sig.infos[signo].si_code))
			return -EINTR;
	}
	infop->si_signo = SIGCHLD;
	infop->si_code = status_to_si_code(s->status);
	infop->si_errno = 0;
	infop->si_pid = s->pid;
	infop->si_uid = 0; // TODO
	infop->si_status = WVALUE(s->status);
	infop->si_value.sigval_int = s->status; // for wait[pid] wrapper

	// reap zombie
	if (WIFEXITED(s->status) || WIFSIGNALED(s->status)) {
		struct pcb *p = get_pcb(s->pid);
		if (!p)
			panic("Tried to reap non-existant child");
		list_del(&p->child_chain);
		reap(p);
	}

	list_del(&s->chain);
	slab_free(proc_status_cachep, s);
	return 0;
}
