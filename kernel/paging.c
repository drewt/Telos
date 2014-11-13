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
static ulong fp_start;
static uint first_free;

#define flush_page(addr) \
	asm volatile("invlpg (%0)" : : "b" (addr));

static inline void flush_pages(ulong addr, uint n)
{
	for (uint i = 0; i < n; i++)
		flush_page(addr + n*FRAME_SIZE);
}

static inline unsigned int addr_to_pdi(ulong addr)
{
	return (addr & 0xFFC00000) >> 22;
}

static inline unsigned long pdi_to_addr(unsigned int pdi)
{
	return pdi << 22;
}

static inline unsigned int addr_to_pti(ulong addr)
{
	return (addr & 0x003FF000) >> 12;
}

static inline unsigned long pti_to_addr(unsigned int pdi, unsigned int pti)
{
	return pdi_to_addr(pdi) + (pti << 12);
}

static struct pf_info *phys_to_info(ulong addr)
{
	return &frame_table[(addr - fp_start) / FRAME_SIZE];
}

/*
 * Get the page table entry associated with a given address in a given
 * address space.  Assumes a page table exists mapping the region containing
 * the given address.
 */
static inline pte_t *addr_to_pte(pmap_t pgdir, ulong addr)
{
	pte_t pde = pgdir[addr_to_pdi(addr)];
	pte_t *pgtab = (pte_t*) (pde & ~0xFFF);
	return pgtab + addr_to_pti(addr);
}

/*
 * Get the page directory entry associated with a given address in a given
 * address space.
 */
static inline ulong *addr_to_pde(pmap_t pgdir, ulong frame)
{
	return pgdir + addr_to_pdi(frame);
}

/*
 * Unset a page attribute (PE_P, PE_RW, PE_U).
 */
static void page_attr_off(pmap_t pgdir, ulong start, ulong end, ulong flags)
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
static void page_attr_on(pmap_t pgdir, ulong start, ulong end, ulong flags)
{
	pte_t *pte;
	int nr_frames;

	pte = addr_to_pte(pgdir, start);
	nr_frames = (page_align(end) - start) / FRAME_SIZE;
	for (int i = 0; i < nr_frames; i++, pte++)
		*pte |= flags;
}

static int frame_pool_init(ulong start, ulong end)
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
int paging_init(ulong start, ulong end)
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

static inline ulong vma_to_page_flags(ulong flags)
{
	return ((flags & VM_READ) ? PE_U : 0) | ((flags & VM_WRITE) ? PE_RW : 0);
}

/*
 * Map a page from the temporary page table to a given physical address.  This
 * function returns an address aliasing the given physical address.
 */
void *kmap_tmp_page(ulong addr)
{
	int i;
	ulong tmp_addr;

	for (i = NR_RESERVED_PAGES; i < 1022; i++) {
		if (!(tmp_pgtab[i] & PE_P))
			break;
	}
	if (i == 1022)
		return NULL;

	tmp_pgtab[i] = addr | PE_P | PE_RW;

	tmp_addr = TMP_PGTAB_BASE + i*FRAME_SIZE;
	flush_page(tmp_addr);
	return (void*) (tmp_addr + (addr & 0xFFF));
}

static void *kmap_tmp_page_n(ulong addr, unsigned n)
{
	ulong tmp_addr = TMP_PGTAB_BASE + n * FRAME_SIZE;
	tmp_pgtab[n] = addr | PE_P | PE_RW;
	flush_page(tmp_addr);
	return (void*) tmp_addr;
}

void kunmap_tmp_page(void *addr)
{
	tmp_pgtab[addr_to_pti((ulong)addr)] = 0;
	flush_page(addr);
}

/*
 * Unmap the page associated with a given virtual address from the kernel's
 * address space.
 */
void kunmap_page(void *addr)
{
	pmap_t pgtab;

	pgtab = (pmap_t) (kernel_pgdir[addr_to_pdi((ulong)addr)] & ~0xFFF);
	pgtab = kmap_tmp_page_n((ulong)pgtab, 0);

	pgtab[addr_to_pti((ulong)addr)] = 0;
	flush_page(addr);
}

/*
 * Gets 'nr_pages' contiguous, free PTEs from the temporary page table.  This
 * function returns the address associated with the first PTE, and stores a
 * pointer to the first PTE in 'dst'.
 *
 * TODO: make this more efficient (bitmap?)
 */
static ulong get_tmp_ptes(pte_t **dst, unsigned nr_pages)
{
	pte_t *pte, *first = NULL;
	ulong first_addr;
	unsigned count = 0;

	pte = &tmp_pgtab[NR_RESERVED_PAGES];
	for (unsigned i = NR_RESERVED_PAGES; i < 1022; i++, pte++) {
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
void *kmap_tmp_range(pmap_t pgdir, ulong addr, size_t len, ulong flags)
{
	pmap_t pgtab;
	pte_t *pte, *tmp;
	ulong tmp_addr;
	unsigned nr_pages;
	uchar attr = vma_to_page_flags(flags);

	/* map page tables */
	pgdir = kmap_tmp_page_n((ulong)pgdir, 0);
	pgtab = (pmap_t) (pgdir[addr_to_pdi(addr)] & ~0xFFF);
	pgtab = kmap_tmp_page_n((ulong)pgtab, 1);
	pte = (pte_t*) &pgtab[addr_to_pti(addr)];

	nr_pages = pages_in_range(addr, len);
	if ((tmp_addr = get_tmp_ptes(&tmp, nr_pages)) == 0)
		return NULL;

	/* map memory area */
	for (unsigned i = 0; i < nr_pages; i++, pte++, tmp++) {
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
	unsigned nr_pages = pages_in_range((ulong)addrp, len);
	unsigned starti = addr_to_pti((ulong)addrp);

	for (unsigned i = starti; i < starti + nr_pages; i++) {
		tmp_pgtab[i] = 0;
	}
}

#define kernel_pdi addr_to_pdi((ulong)&KERNEL_PAGE_OFFSET)

static ulong clone_page(ulong phys_page)
{
	struct pf_info *f_page;
	void *copy, *original;

	if (!(original = kmap_tmp_page(phys_page)))
		return 0;
	if (!(f_page = kalloc_frame(VM_ZERO)))
		return 0;
	if (!(copy = kmap_tmp_page(f_page->addr))) {
		kfree_frame(f_page);
		return 0;
	}

	memcpy(copy, original, FRAME_SIZE);
	kunmap_tmp_page(copy);
	kunmap_tmp_page(original);
	return f_page->addr;
}

static pmap_t clone_pgtab(ulong phys_pgtab)
{
	struct pf_info *f_pgtab;
	pmap_t copy, original, retval;
	ulong page;

	if (!(original = kmap_tmp_page(phys_pgtab)))
		return NULL;
	if (!(f_pgtab = kalloc_frame(VM_ZERO)))
		return NULL;
	if (!(copy = kmap_tmp_page(f_pgtab->addr))) {
		kfree_frame(f_pgtab);
		return NULL;
	}

	retval = (pmap_t) f_pgtab->addr;
	for (unsigned i = 0; i < 1024; i++) {
		if (!(original[i] & PE_P))
			continue;
		if (!(page = clone_page(original[i] & ~0xFFF))) {
			kfree_frame(f_pgtab);
			retval = NULL;
			break;
		}
		copy[i] = page | (original[i] & 0xFFF);
	}

	kunmap_tmp_page(copy);
	kunmap_tmp_page(original);
	return retval;
}

pmap_t clone_pgdir(void)
{
	pmap_t p_pgdir, pgdir, pgtab;

	if (!(p_pgdir = new_pgdir()))
		return NULL;

	if (!(pgdir = kmap_tmp_page((ulong)p_pgdir)))
		return NULL;

	for (unsigned i = 0; i < kernel_pdi; i++) {
		if (!(current_pgdir[i] & PE_P))
			continue;
		if (!(pgtab = clone_pgtab(current_pgdir[i] & ~0xFFF))) {
			kunmap_tmp_page(pgdir);
			return NULL;
		}
		pgdir[i] = (ulong)pgtab | (current_pgdir[i] & 0xFFF);
	}
	kunmap_tmp_page(pgdir);
	return p_pgdir;
}

pmap_t new_pgdir(void)
{
	struct pf_info *f_pgdir, *f_pgtab;
	pmap_t pgdir, pgtab;
	unsigned pdi;

	if ((f_pgdir = kalloc_frame(VM_ZERO)) == NULL)
		goto nomem0;
	if ((f_pgtab = kalloc_frame(VM_ZERO)) == NULL)
		goto nomem1;
	if ((pgdir = kmap_tmp_page(f_pgdir->addr)) == NULL)
		goto nomem2;
	if ((pgtab = kmap_tmp_page(f_pgtab->addr)) == NULL)
		goto nomem3;

	/* map page directory to last page */
	pgdir[1023] = f_pgtab->addr | PE_P | PE_RW; /* pgtab mapping 0xFFC00000+ */
	pgtab[1023] = f_pgdir->addr | PE_P | PE_RW; /* pgdir at 0xFFFFF000 */
	pgtab[1022] = f_pgtab->addr | PE_P | PE_RW; /* tmp_pgtab at 0xFFFFE000 */

	/* map kernel memory */
	for (pdi = addr_to_pdi((ulong)&KERNEL_PAGE_OFFSET);
			kernel_pgdir[pdi] & PE_P;
			pdi++)
		pgdir[pdi] = kernel_pgdir[pdi];

	kunmap_tmp_page(pgtab);
	kunmap_tmp_page(pgdir);
	return (pmap_t) f_pgdir->addr;

nomem3:	kunmap_tmp_page(pgdir);
nomem2:	kfree_frame(f_pgtab);
nomem1:	kfree_frame(f_pgdir);
nomem0:	return NULL;
}

/*
 * Free all frames mapped in a page table.
 */
static int del_pgtab(ulong phys_pgtab)
{
	pmap_t pgtab = kmap_tmp_page(phys_pgtab);

	if (pgtab == NULL)
		return -ENOMEM;

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
int del_pgdir(pmap_t phys_pgdir)
{
	int rc;
	pmap_t pgdir = kmap_tmp_page((ulong)phys_pgdir);

	if (pgdir == NULL)
		return -ENOMEM;

	for (unsigned i = 0; i < kernel_pdi; i++) {
		if (!(pgdir[i] & PE_P))
			continue;
		if ((rc = del_pgtab(pgdir[i] & ~0xFFF)) != 0)
			return rc;
	}
	kfree_frame(phys_to_info(pgdir[1023] & ~0xFFF));
	kfree_frame(phys_to_info((ulong)phys_pgdir));
	kunmap_tmp_page(pgdir);
	return 0;
}

static int pm_unmap_pgtab(struct vma *vma, unsigned int pdi,
		unsigned long phys_pgtab)
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
	if (!pgtab)
		return -ENOMEM;
	for (; pti <= last; pti++) {
		void *addr = (void*) pti_to_addr(pdi, pti);
		if (!(pgtab[pti] & PE_P))
			continue;
		if (pgtab[pti] & PE_D)
			vm_writeback(vma, addr, FRAME_SIZE);
		kfree_frame(phys_to_info(pgtab[pti] & ~0xFFF));
		pgtab[pti] = 0;
		flush_page(addr);
	}
	kunmap_tmp_page(pgtab);
	return 0;
}

int pm_unmap(struct vma *vma)
{
	int error = 0;
	unsigned int last = addr_to_pdi(vma->end-1);
	pmap_t pgdir = kmap_tmp_page((ulong)vma->mmap->pgdir);

	if (!pgdir)
		return -ENOMEM;
	for (unsigned int pdi = addr_to_pdi(vma->start); pdi <= last; pdi++) {
		if (!(pgdir[pdi] & PE_P))
			continue;
		if ((error = pm_unmap_pgtab(vma, pdi, pgdir[pdi] & ~0xFFF)))
			break;
		// FIXME: free page table (if empty)?
	}
	kunmap_tmp_page(pgdir);
	return error;
}

/*
 * Allocate a page from the frame pool.
 */
struct pf_info *kalloc_frame(ulong flags)
{
	void *vaddr;
	struct pf_info *page;

	if (list_empty(&frame_pool))
		return NULL; /* TODO: try to free some memory */

	page = list_pop(&frame_pool, struct pf_info, chain);
	page->ref = 1;

	if (flags & VM_ZERO) {
		if (!(vaddr = kmap_tmp_page(page->addr))) {
			kfree_frame(page);
			return NULL;
		}
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
static pmap_t umap_page_table(pmap_t pgdir, ulong addr)
{
	pte_t *pde = addr_to_pde(pgdir, addr);

	if (!(*pde & PE_P)) {
		struct pf_info *frame = kalloc_frame(VM_ZERO);
		*pde = frame->addr | PE_P | PE_RW | PE_U;
	}
	return kmap_tmp_page(*pde & ~0xFFF);
}

/*
 * Map the page table for the given (kernel) address.  If the page table does
 * not yet exist, it is allocated and added to all process page directories.
 */
static pmap_t kmap_page_table(ulong addr)
{
	pte_t *pde = addr_to_pde(kernel_pgdir, addr);

	/* allocate page table if one does not already exist */
	if (!(*pde & PE_P)) {
		struct pf_info *frame = kalloc_frame(VM_ZERO);
		*pde = frame->addr | PE_P | PE_RW;
		/* update process page direcories */
		for (uint i = 0; i < PT_SIZE; i++) {
			struct pcb *p = &proctab[i];
			if (p->state != PROC_DEAD) {
				pmap_t pgdir = kmap_tmp_page((ulong)p->mm.pgdir);
				pte_t *upde = addr_to_pde(pgdir, addr);
				*upde = frame->addr | PE_P | PE_RW;
				kunmap_tmp_page(pgdir);
			}
		}
	}

	return kmap_tmp_page(*pde & ~0xFFF);
}

static pmap_t knext_page_table(uint i, pmap_t pgtab)
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
	for (uint i = start; i < (start) + (pages); \
			i++, pgtab = unext_page_table(i, pgdir, pgtab))

/*
 * Map 'pages' pages starting at the virtual address 'dst' into the (physical)
 * page directory 'pgdir'.
 */
int map_pages(pmap_t phys_pgdir, ulong dst, unsigned pages, ulong flags)
{
	struct pf_info *frame;
	uchar attr = vma_to_page_flags(flags);
	pmap_t pgdir = kmap_tmp_page((ulong)phys_pgdir);
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

int map_page(void *addr, ulong flags)
{
	pmap_t pgtab;
	struct pf_info *frame;
	uchar attr = vma_to_page_flags(flags);

	frame = kalloc_frame(flags);
	if (!frame)
		return -ENOMEM;
	pgtab = umap_page_table(current_pgdir, (ulong) addr);
	if (!pgtab) {
		kfree_frame(frame);
		return -ENOMEM;
	}

	pgtab[addr_to_pti((ulong)addr)] = frame->addr | PE_P | attr;
	kunmap_tmp_page(pgtab);
	return 0;
}

int copy_page(void *addr, ulong flags)
{
	void *tmp;
	pmap_t pgtab;
	struct pf_info *frame;
	uchar attr = vma_to_page_flags(flags);

	frame = kalloc_frame(flags);
	if (!frame)
		return -ENOMEM;
	pgtab = umap_page_table(current_pgdir, (ulong) addr);
	if (!pgtab) {
		kfree_frame(frame);
		return -ENOMEM;
	}

	tmp = kmap_tmp_page(frame->addr);
	memcpy(tmp, (void*)page_base((ulong)addr), FRAME_SIZE);
	kunmap_tmp_page(tmp);

	pgtab[addr_to_pti((ulong)addr)] = frame->addr | PE_P | attr;
	kunmap_tmp_page(pgtab);
	return 0;
}

/*
 * Allocate and map n consecutive pages in the kernel's address space.
 */
void *kalloc_pages(uint n)
{
	uint count = 0;
	uint start = 0;
	uint i = first_free;
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

void kfree_pages(void *addr, uint n)
{
	uint start = (ulong)addr / FRAME_SIZE;
	pmap_t pgtab = kmap_page_table((ulong)addr);

	for (uint i = start; i < start + n; i++, pgtab = knext_page_table(i, pgtab)) {
		struct pf_info *frame = phys_to_info(pgtab[i % 1024] & ~0xFFF);
		pgtab[i % 1024] = 0;
		kfree_frame(frame);
	}
	kunmap_tmp_page(pgtab);
	// TODO: check limit

	if (start < first_free)
		first_free = start;

	flush_pages((ulong)addr, n);
}
