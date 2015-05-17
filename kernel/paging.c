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
#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <kernel/mmap.h>
#include <kernel/process.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/paging.h>

#include <string.h>

#define PAGE_TABLE(name) \
	pte_t name[1024] __attribute__((aligned(0x1000)))

#define TMP_PGTAB_BASE 0xFFC00000

#define NR_RESERVED_PAGES 4

#define kernel_pgdir (&_kernel_pgd)

/* page table for temporary mappings */
static PAGE_TABLE(_tmp_pgtab);
#define tmp_pgtab ((pmap_t) 0xFFFFE000)

#define current_pgdir ((pmap_t) 0xFFFFF000)

/* free list for page heap */
static LIST_HEAD(frame_pool);
static struct pf_info *frame_table;
static uintptr_t fp_start;
static unsigned int first_free;

#define flush_page(addr) \
	asm volatile("invlpg (%0)" : : "b" (addr));

static inline void flush_pages(uintptr_t addr, unsigned int n)
{
	for (unsigned int i = 0; i < n; i++)
		flush_page(addr + n*FRAME_SIZE);
}

static inline unsigned int addr_to_pdi(uintptr_t addr)
{
	return (addr & 0xFFC00000) >> 22;
}

static inline uintptr_t pdi_to_addr(unsigned int pdi)
{
	return pdi << 22;
}

static inline unsigned int addr_to_pti(uintptr_t addr)
{
	return (addr & 0x003FF000) >> 12;
}

static inline uintptr_t pti_to_addr(unsigned int pdi, unsigned int pti)
{
	return pdi_to_addr(pdi) + (pti << 12);
}

static struct pf_info *phys_to_info(uintptr_t addr)
{
	return &frame_table[(addr - fp_start) / FRAME_SIZE];
}

/*
 * Get the page table entry associated with a given address in a given
 * address space.  Assumes a page table exists mapping the region containing
 * the given address.
 */
static inline pte_t *addr_to_pte(pmap_t pgdir, uintptr_t addr)
{
	pte_t pde = pgdir[addr_to_pdi(addr)];
	pte_t *pgtab = (pte_t*) (pde & ~0xFFF);
	return pgtab + addr_to_pti(addr);
}

/*
 * Get the page directory entry associated with a given address in a given
 * address space.
 */
static inline pte_t *addr_to_pde(pmap_t pgdir, uintptr_t frame)
{
	return pgdir + addr_to_pdi(frame);
}

/*
 * Unset a page attribute (PE_P, PE_RW, PE_U).
 */
static void page_attr_off(pmap_t pgdir, uintptr_t start, uintptr_t end,
		int flags)
{
	pte_t *pte;
	int nr_frames;

	pte = addr_to_pte(pgdir, start);
	nr_frames = (page_align(end) - start) / FRAME_SIZE;
	for (int i = 0; i < nr_frames; i++, pte++)
		*pte &= ~flags;
}

/*
 * Set a page attribute (PE_P, PE_RW, PE_U).
 */
static void page_attr_on(pmap_t pgdir, uintptr_t start, uintptr_t end,
		int flags)
{
	pte_t *pte;
	int nr_frames;

	pte = addr_to_pte(pgdir, start);
	nr_frames = (page_align(end) - start) / FRAME_SIZE;
	for (int i = 0; i < nr_frames; i++, pte++)
		*pte |= flags;
}

static int frame_pool_init(uintptr_t start, uintptr_t end)
{
	unsigned nr_frames = (end - start) / FRAME_SIZE;
	frame_table = kmalloc(nr_frames * sizeof(struct pf_info));
	if (frame_table == NULL)
		return -1;

	fp_start = start;

	for (unsigned i = 0; i < nr_frames; i++) {
		frame_table[i].addr = start + i*FRAME_SIZE;
		frame_table[i].ref = 0;
		list_add_tail(&frame_table[i].chain, &frame_pool);
	}
	return 0;
}

/*
 * Initialize the frame pool.
 */
int paging_init(uintptr_t start, uintptr_t end)
{
	frame_pool_init(start, end);

	first_free = kheap_end / FRAME_SIZE;

	/* disable R/W flag for read-only sections */
	page_attr_off(kernel_pgdir, urostart, uroend, PE_RW);
	page_attr_off(kernel_pgdir, krostart, kroend, PE_RW);

	/* set up page table for temporary mappings */
	kernel_pgdir[addr_to_pdi(TMP_PGTAB_BASE)] =
		kernel_to_phys(_tmp_pgtab) | PE_P | PE_RW;
	/* map _tmp_pgtab at 0xFFFFE000 */
	_tmp_pgtab[1022] = kernel_to_phys(_tmp_pgtab) | PE_P | PE_RW;
	/* map kernel_pgdir at 0xFFFFF000 */
	_tmp_pgtab[1023] = kernel_to_phys(kernel_pgdir) | PE_P | PE_RW;

	for (int i = 0; i < 16; i++)
		kernel_pgdir[i] = 0;

	return 0;
}

static inline int vma_to_page_flags(int flags)
{
	return ((flags & VM_READ) ? PE_U : 0) | ((flags & VM_WRITE) ? PE_RW : 0);
}

/*
 * Map a page from the temporary page table to a given physical address.  This
 * function returns an address aliasing the given physical address.
 */
void *kmap_tmp_page(uintptr_t addr)
{
	int i;
	uintptr_t tmp_addr;

	for (i = NR_RESERVED_PAGES; i < 1024 - NR_HIGH_PAGES; i++) {
		if (!(tmp_pgtab[i] & PE_P))
			break;
	}
	if (i == 1024 - NR_HIGH_PAGES)
		panic("Ran out of temporary address space!");

	tmp_pgtab[i] = addr | PE_P | PE_RW;

	tmp_addr = TMP_PGTAB_BASE + i*FRAME_SIZE;
	flush_page(tmp_addr);
	return (void*) (tmp_addr + (addr & 0xFFF));
}

static void *kmap_tmp_page_n(uintptr_t addr, unsigned n)
{
	uintptr_t tmp_addr = TMP_PGTAB_BASE + n * FRAME_SIZE;
	tmp_pgtab[n] = addr | PE_P | PE_RW;
	flush_page(tmp_addr);
	return (void*) tmp_addr;
}

void kunmap_tmp_page(void *addr)
{
	tmp_pgtab[addr_to_pti((uintptr_t)addr)] = 0;
	flush_page(addr);
}

/*
 * Unmap the page associated with a given virtual address from the kernel's
 * address space.
 */
void kunmap_page(void *addr)
{
	pmap_t pgtab;

	pgtab = (pmap_t) (kernel_pgdir[addr_to_pdi((uintptr_t)addr)] & ~0xFFF);
	pgtab = kmap_tmp_page_n((uintptr_t)pgtab, 0);

	pgtab[addr_to_pti((uintptr_t)addr)] = 0;
	flush_page(addr);
}

/*
 * Gets 'nr_pages' contiguous, free PTEs from the temporary page table.  This
 * function returns the address associated with the first PTE, and stores a
 * pointer to the first PTE in 'dst'.
 *
 * TODO: make this more efficient (bitmap?)
 */
static uintptr_t get_tmp_ptes(pte_t **dst, unsigned nr_pages)
{
	pte_t *pte, *first = NULL;
	uintptr_t first_addr;
	unsigned count = 0;

	pte = &tmp_pgtab[NR_RESERVED_PAGES];
	for (unsigned int i = NR_RESERVED_PAGES; i < 1022; i++, pte++) {
		if (!(*pte & PE_P)) {
			if (first == NULL) {
				first = pte;
				first_addr = i*FRAME_SIZE;
			}
			if (++count >= nr_pages) {
				*dst = first;
				return TMP_PGTAB_BASE + first_addr;
			}
		} else {
			first = NULL;
			count = 0;
		}
	}
	return 0;
}

/*
 * Map a region of memory from a given address space into the kernel's
 * address space.  This function returns an address aliasing the given memory
 * region.
 */
void *kmap_tmp_range(uintptr_t phys_pgdir, uintptr_t addr, size_t len, int flags)
{
	pmap_t pgtab, pgdir;
	pte_t *pte, *tmp;
	uintptr_t tmp_addr;
	unsigned int nr_pages;
	unsigned char attr = vma_to_page_flags(flags);

	/* map page tables */
	pgdir = kmap_tmp_page_n(phys_pgdir, 0);
	pgtab = (pmap_t) (pgdir[addr_to_pdi(addr)] & ~0xFFF);
	pgtab = kmap_tmp_page_n((uintptr_t)pgtab, 1);
	pte = (pte_t*) &pgtab[addr_to_pti(addr)];

	nr_pages = pages_in_range(addr, len);
	if ((tmp_addr = get_tmp_ptes(&tmp, nr_pages)) == 0)
		return NULL;

	/* map memory area */
	for (unsigned int i = 0; i < nr_pages; i++, pte++, tmp++) {
		if (!(*pte & PE_P)) {
			struct pf_info *frame = kalloc_frame(flags);
			if (!frame)
				return NULL;
			*pte = frame->addr | attr | PE_P;
		}
		*tmp = *pte | PE_RW;
		flush_page(tmp_addr + i*FRAME_SIZE);
	}

	return (void*) (tmp_addr + (addr & 0xFFF));
}

void kunmap_tmp_range(void *addrp, size_t len)
{
	unsigned int nr_pages = pages_in_range((uintptr_t)addrp, len);
	unsigned int starti = addr_to_pti((uintptr_t)addrp);

	for (unsigned int i = starti; i < starti + nr_pages; i++) {
		tmp_pgtab[i] = 0;
	}
}

#define kernel_pdi addr_to_pdi(kernel_base)

static pmap_t clone_pgtab(uintptr_t phys_pgtab)
{
	struct pf_info *f_pgtab;
	pmap_t copy, original;

	if (!(f_pgtab = kalloc_frame(VM_ZERO)))
		return NULL;
	original = kmap_tmp_page(phys_pgtab);
	copy = kmap_tmp_page(f_pgtab->addr);

	// loop over pages to update reference counts
	for (unsigned int i = 0; i < 1024; i++) {
		if (!(original[i] & PE_P))
			continue;
		phys_to_info(original[i] & ~0xFFF)->ref++;
		copy[i] = original[i];
	}
	return (pmap_t) f_pgtab->addr;
}

uintptr_t clone_pgdir(void)
{
	uintptr_t p_pgdir;
	pmap_t pgdir, pgtab;
	if (!(p_pgdir = new_pgdir()))
		return 0;
	pgdir = kmap_tmp_page((uintptr_t)p_pgdir);
	for (unsigned i = 0; i < kernel_pdi; i++) {
		if (!(current_pgdir[i] & PE_P))
			continue;
		if (!(pgtab = clone_pgtab(current_pgdir[i] & ~0xFFF))) {
			kunmap_tmp_page(pgdir);
			return 0;
		}
		pgdir[i] = (uintptr_t)pgtab | (current_pgdir[i] & 0xFFF);
	}

	// copy kernel stack
	pgtab = kmap_tmp_page(pgdir[1023] & ~0xFFF);
	for (int i = 0; i < NR_KSTACK_PAGES; i++) {
		void *tmp = kmap_tmp_page(pgtab[(1024 - NR_HIGH_PAGES) + i] & ~0xFFF);
		memcpy(tmp, (void*)(KSTACK_START + i*FRAME_SIZE), FRAME_SIZE);
		kunmap_tmp_page(tmp);
	}

	kunmap_tmp_page(pgdir);
	return p_pgdir;
}

/*
 * Here we keep a free-list of "husk" page directories which map only the
 * kernel's address space.  This speeds up process creation, but is also useful
 * to delay freeing page directories which are in use (i.e. during exit).
 */

#define MAX_HUSKS 16
static unsigned int husks = 0;
static LIST_HEAD(husk_list);

static inline uintptr_t pop_husk(void)
{
	struct pf_info *frame;
	frame = list_first_entry(&husk_list, struct pf_info, chain);
	list_del(&frame->chain);
	husks--;
	return frame->addr;
}

static void push_husk(uintptr_t phys_pgdir)
{
	struct pf_info *frame = phys_to_info((uintptr_t)phys_pgdir);
	list_add(&frame->chain, &husk_list);
	husks++;
}

static uintptr_t make_new_pgdir(void)
{
	struct pf_info *frames[NR_HIGH_PAGES];
	pmap_t pgdir, pgtab;
	unsigned int pdi;
	int i;

	for (i = 0; i < NR_HIGH_PAGES; i++) {
		if (!(frames[i] = kalloc_frame(VM_ZERO))) {
			for (i = i - 1; i >= 0; i--)
				kfree_frame(frames[i]);
			return 0;
		}
	}
	pgdir = kmap_tmp_page(frames[NR_HIGH_PAGES-1]->addr);
	pgtab = kmap_tmp_page(frames[NR_HIGH_PAGES-2]->addr);

	pgdir[1023] = frames[NR_HIGH_PAGES-2]->addr | PE_P | PE_RW;
	for (int i = 0; i < NR_HIGH_PAGES; i++)
		pgtab[(1024 - NR_HIGH_PAGES) + i] = frames[i]->addr | PE_P | PE_RW;

	for (pdi = addr_to_pdi(kernel_base); pdi < 1023; pdi++) {
		pgdir[pdi] = kernel_pgdir[pdi];
	}

	kunmap_tmp_page(pgtab);
	kunmap_tmp_page(pgdir);
	return frames[NR_HIGH_PAGES-1]->addr;
}

uintptr_t new_pgdir(void)
{
	if (husks > 0)
		return pop_husk();
	return make_new_pgdir();
}

/*
 * Free all frames mapped in a page table.
 */
static int del_pgtab(uintptr_t phys_pgtab)
{
	pmap_t pgtab = kmap_tmp_page(phys_pgtab);

	for (int i = 0; i < 1024; i++) {
		if (!(pgtab[i] & PE_P))
			continue;
		kfree_frame(phys_to_info(pgtab[i] & ~0xFFF));
	}
	kunmap_tmp_page(pgtab);
	kfree_frame(phys_to_info(phys_pgtab));
	return 0;
}

/*
 * Free all frames mapped in a page directory.
 */
static int del_pgdir(uintptr_t phys_pgdir)
{
	int rc;
	pmap_t pgdir = kmap_tmp_page(phys_pgdir);

	for (unsigned i = 0; i < kernel_pdi; i++) {
		if (!(pgdir[i] & PE_P))
			continue;
		if ((rc = del_pgtab(pgdir[i] & ~0xFFF)) != 0)
			return rc;
	}
	kfree_frame(phys_to_info(pgdir[1023] & ~0xFFF));
	kfree_frame(phys_to_info(phys_pgdir));
	kunmap_tmp_page(pgdir);
	return 0;
}

static inline void free_pte(pte_t pte)
{
	kfree_frame(phys_to_info(pte & ~0xFFF));
}

static int free_husk(uintptr_t phys_pgdir)
{
	pmap_t pgdir, pgtab;
	pgdir = kmap_tmp_page(phys_pgdir);
	pgtab = kmap_tmp_page(pgdir[1023] & ~0xFFF);
	for (int i = 0; i < NR_HIGH_PAGES; i++)
		free_pte(pgtab[(1024 - NR_HIGH_PAGES) + i]);
	kunmap_tmp_page(pgtab);
	kunmap_tmp_page(pgdir);
	return 0;
}

/*
 * Free all user-space memory.  In theory, this is a no-op (pm_unmap() should
 * have freed everything before this is called), but we do it anyways.
 */
static int husk_pgdir(uintptr_t phys_pgdir)
{
	int error = 0;
	pmap_t pgdir = kmap_tmp_page(phys_pgdir);
	for (unsigned int i = 0; i < kernel_pdi; i++) {
		if (!(pgdir[i] & PE_P))
			continue;
		if ((error = del_pgtab(pgdir[i] & ~0xFFF)))
			break;
		pgdir[i] = 0;
	}
	kunmap_tmp_page(pgdir);
	return error;
}

int free_pgdir(uintptr_t phys_pgdir)
{
	if (husks >= MAX_HUSKS)
		free_husk(pop_husk());
	husk_pgdir(phys_pgdir);
	push_husk(phys_pgdir);
	return 0;
}

typedef pte_t (*apply_fn)(struct vma *, void *, pte_t);

static int pm_apply_pgtab(struct vma *vma, unsigned int pdi,
		uintptr_t phys_pgtab, apply_fn fn)
{
	pmap_t pgtab;
	unsigned int pti = 0;
	unsigned int last = 1023;

	// start in middle of page table
	if (vma->start > pdi_to_addr(pdi))
		pti = addr_to_pti(vma->start);
	// end in middle of page table
	if (vma->end < pdi_to_addr(pdi+1))
		last = addr_to_pti(vma->end-1);

	pgtab = kmap_tmp_page(phys_pgtab);
	for (; pti <= last; pti++) {
		void *addr = (void*) pti_to_addr(pdi, pti);
		if (!(pgtab[pti] & PE_P))
			continue;
		pgtab[pti] = fn(vma, addr, pgtab[pti]);
		flush_page(addr);
	}
	kunmap_tmp_page(pgtab);
	return 0;
}

static int pm_apply(struct vma *vma, apply_fn fn)
{
	int error = 0;
	unsigned int last = addr_to_pdi(vma->end-1);
	pmap_t pgdir = kmap_tmp_page(vma->mmap->pgdir);

	for (unsigned int pdi = addr_to_pdi(vma->start); pdi <= last; pdi++) {
		if (!(pgdir[pdi] & PE_P))
			continue;
		error = pm_apply_pgtab(vma, pdi, pgdir[pdi] & ~0xFFF, fn);
		if (error)
			break;
	}
	kunmap_tmp_page(pgdir);
	return error;
}

static pte_t pm_unmap_fn(struct vma *vma, void *addr, pte_t pte)
{
	if (pte & PE_D)
		vm_writeback(vma, addr, FRAME_SIZE);
	kfree_frame(phys_to_info(pte & ~0xFFF));
	return 0;
}

int pm_unmap(struct vma *vma)
{
	return pm_apply(vma, pm_unmap_fn);
}

static pte_t pm_disable_write_fn(struct vma *vma, void *addr, pte_t pte)
{
	return pte & ~PE_RW;
}

int pm_disable_write(struct vma *vma)
{
	return pm_apply(vma, pm_disable_write_fn);
}

static pte_t pm_copy_fn(struct vma *vma, void *addr, pte_t pte)
{
	void *tmp;
	struct pf_info *frame;

	if (!(frame = kalloc_frame(vma->flags)))
		panic("pm_copy_fn: out of memory...");
	tmp = kmap_tmp_page(frame->addr);
	memcpy(tmp, addr, FRAME_SIZE);
	kunmap_tmp_page(tmp);

	kfree_frame(phys_to_info(pte & ~0xFFF));
	return frame->addr | (pte & 0xFFF);
}

int pm_copy(struct vma *vma)
{
	return pm_apply(vma, pm_copy_fn);
}

int pm_copy_to(uintptr_t phys_pgdir, void *dst, const void *src, size_t len)
{
	void *addr = kmap_tmp_range(phys_pgdir, (uintptr_t)dst, len, VM_WRITE);
	if (!addr)
		return -ENOMEM;
	memcpy(addr, src, len);
	kunmap_tmp_range(addr, len);
	return 0;
}

/*
 * Allocate a page from the frame pool.
 */
struct pf_info *kalloc_frame(int flags)
{
	void *vaddr;
	struct pf_info *page;

	if (list_empty(&frame_pool))
		panic("out of memory!"); // TODO: try to free some memory

	page = list_pop(&frame_pool, struct pf_info, chain);
	page->ref = 1;

	if (flags & VM_ZERO) {
		vaddr = kmap_tmp_page(page->addr);
		memset(vaddr, 0, FRAME_SIZE);
		kunmap_tmp_page(vaddr);
	}
	return page;
}

/*
 * Return a page to the frame pool.
 */
void _kfree_frame(struct pf_info *page)
{
	list_push(&page->chain, &frame_pool);
}

/*
 * Map the page table for the given address, allocating it if one does not
 * already exist.
 */
static pmap_t umap_page_table(pmap_t pgdir, uintptr_t addr)
{
	pte_t *pde = addr_to_pde(pgdir, addr);

	if (!(*pde & PE_P)) {
		struct pf_info *frame = kalloc_frame(VM_ZERO);
		*pde = frame->addr | PE_P | PE_RW | PE_U;
	}
	return kmap_tmp_page(*pde & ~0xFFF);
}

static void do_map_pgtab(uintptr_t phys_pgdir, uintptr_t phys_pgtab,
		uintptr_t addr)
{
	pmap_t pgdir = kmap_tmp_page(phys_pgdir);
	*(addr_to_pde(pgdir, addr)) = phys_pgtab | PE_P | PE_RW;
	kunmap_tmp_page(pgdir);
}

/*
 * Map the page table for the given (kernel) address.  If the page table does
 * not yet exist, it is allocated and added to all process page directories.
 */
static pmap_t kmap_page_table(uintptr_t addr)
{
	pte_t *pde = addr_to_pde(kernel_pgdir, addr);

	// allocate page table if one does not already exist
	if (!(*pde & PE_P)) {
		struct pf_info *husk;
		struct pf_info *frame = kalloc_frame(VM_ZERO);
		*pde = frame->addr | PE_P | PE_RW;
		// update process page direcories
		for (unsigned int i = 0; i < PT_SIZE; i++) {
			struct pcb *p = &proctab[i];
			if (p->state == PROC_DEAD)
				continue;
			do_map_pgtab(p->mm.pgdir, frame->addr, addr);
		}
		list_for_each_entry(husk, &husk_list, chain) {
			do_map_pgtab(husk->addr, frame->addr, addr);
		}
	}

	return kmap_tmp_page(*pde & ~0xFFF);
}

static pmap_t knext_page_table(unsigned int i, pmap_t pgtab)
{
	if (i % 1024 != 0)
		return pgtab;
	kunmap_tmp_page(pgtab);
	return kmap_page_table(i * FRAME_SIZE);
}

static pmap_t unext_page_table(unsigned i, pmap_t pgdir, pmap_t pgtab)
{
	if (i % 1024 != 0)
		return pgtab;
	kunmap_tmp_page(pgtab);
	return umap_page_table(pgdir, i * FRAME_SIZE);
}

#define for_each_upage(i, pgdir, pgtab, start, pages) \
	for (unsigned int i = start; i < (start) + (pages); \
			i++, pgtab = unext_page_table(i, pgdir, pgtab))

/*
 * Map 'pages' pages starting at the virtual address 'dst' into the (physical)
 * page directory 'pgdir'.
 */
int map_pages(uintptr_t phys_pgdir, uintptr_t dst, unsigned int pages, int flags)
{
	struct pf_info *frame;
	unsigned char attr = vma_to_page_flags(flags);
	pmap_t pgdir = kmap_tmp_page(phys_pgdir);
	pmap_t pgtab = umap_page_table(pgdir, dst);

	for_each_upage(i, pgdir, pgtab, dst / FRAME_SIZE, pages) {
		frame = kalloc_frame(flags);
		if (!frame)
			return -ENOMEM;
		pgtab[i % 1024] = frame->addr | PE_P | attr;
	}
	kunmap_tmp_page(pgtab);
	kunmap_tmp_page(pgdir);
	return 0;
}

int map_frame(struct pf_info *frame, void *addr, int flags)
{
	pmap_t pgtab;
	unsigned char attr = vma_to_page_flags(flags);

	pgtab = umap_page_table(current_pgdir, (uintptr_t) addr);
	pgtab[addr_to_pti((uintptr_t)addr)] = frame->addr | PE_P | attr;
	kunmap_tmp_page(pgtab);
	return 0;
}

int map_page(void *addr, int flags)
{
	struct pf_info *frame = kalloc_frame(flags);
	if (!frame)
		return -ENOMEM;
	return map_frame(frame, addr, flags);
}

int copy_page(void *addr, int flags)
{
	void *tmp;
	pmap_t pgtab;
	struct pf_info *frame;
	unsigned char attr = vma_to_page_flags(flags);
	unsigned int pti = addr_to_pti((uintptr_t)addr);

	pgtab = umap_page_table(current_pgdir, (uintptr_t) addr);
	if (!pgtab)
		return -ENOMEM;
	// no need to copy if frame is only referenced once
	if (phys_to_info(pgtab[pti] & ~0xFFF)->ref == 1) {
		pgtab[pti] |= attr;
		goto success;
	}
	frame = kalloc_frame(flags);
	if (!frame) {
		kunmap_tmp_page(pgtab);
		return -ENOMEM;
	}
	tmp = kmap_tmp_page(frame->addr);
	memcpy(tmp, (void*)page_base(addr), FRAME_SIZE);
	kunmap_tmp_page(tmp);

	kfree_frame(phys_to_info(pgtab[pti] & ~0xFFF));
	pgtab[pti] = frame->addr | PE_P | attr;
success:
	kunmap_tmp_page(pgtab);
	flush_page(addr);
	return 0;
}

/*
 * Allocate and map n consecutive pages in the kernel's address space.
 */
void *kalloc_pages(unsigned int n)
{
	unsigned int count = 0;
	unsigned int start = 0;
	unsigned int i = first_free;
	pmap_t pgtab = kmap_page_table(i * FRAME_SIZE);

	/* find n consecutive, free pages */
	for (i = first_free; /*TODO*/; i++, pgtab = knext_page_table(i, pgtab)) {
		if (pgtab[i % 1024] & PE_P) {
			count = 0;
			start = 0;
		} else {
			if (start == 0)
				start = i;
			if (++count == n)
				break;
		}
	}
	kunmap_tmp_page(pgtab);
	// TODO: if (i == LIMIT) fail()

	pgtab = kmap_page_table(start * FRAME_SIZE);

	/* allocate and map frames */
	for (i = start; i < start + n; i++, pgtab = knext_page_table(i, pgtab)) {
		struct pf_info *frame = kalloc_frame(0);
		pgtab[i % 1024] = frame->addr | PE_P | PE_RW;
	}
	kunmap_tmp_page(pgtab);

	/* update first_free */
	if (start == first_free) {
		pgtab = kmap_page_table((start+n) * FRAME_SIZE);
		for (i = start + n; ; i++, pgtab = knext_page_table(i, pgtab)) {
			if (!(pgtab[i % 1024] & PE_P)) {
				first_free = i;
				break;
			}
		}
		kunmap_tmp_page(pgtab);
		// TODO: check limit
	}
	flush_pages(start * FRAME_SIZE, n);
	return (void*) (start * FRAME_SIZE);
}

void kfree_pages(void *addr, unsigned int n)
{
	unsigned int start = (uintptr_t)addr / FRAME_SIZE;
	pmap_t pgtab = kmap_page_table((uintptr_t)addr);

	for (unsigned int i = start; i < start + n; i++, pgtab = knext_page_table(i, pgtab)) {
		struct pf_info *frame = phys_to_info(pgtab[i % 1024] & ~0xFFF);
		pgtab[i % 1024] = 0;
		kfree_frame(frame);
	}
	kunmap_tmp_page(pgtab);
	// TODO: check limit

	if (start < first_free)
		first_free = start;

	flush_pages((uintptr_t)addr, n);
}
