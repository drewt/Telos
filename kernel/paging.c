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

#define TMP_PGTAB_BASE 0xB0400000

#define NR_RESERVED_PAGES 4

#define kernel_pgdir (&_kernel_pgd)

/* page table for temporary mappings */
static PAGE_TABLE(tmp_pgtab);

/* free list for page heap */
static LIST_HEAD(frame_pool);

/* page frame metadata */
static struct pf_info **frame_table;
static unsigned int ft_i = 0;

static ulong fp_start;
static ulong fp_end;

static uint first_free;

static inline unsigned addr_to_pdi(ulong addr)
{
	return (addr & 0xFFC00000) >> 22;
}

static inline unsigned addr_to_pti(ulong addr)
{
	return (addr & 0x003FF000) >> 12;
}

static inline unsigned pages_in_range(ulong addr, ulong len)
{
	return (((addr & 0xFFF) + len) / FRAME_SIZE) + 1;
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
void page_attr_off(pmap_t pgdir, ulong start, ulong end, ulong flags)
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
void page_attr_on(pmap_t pgdir, ulong start, ulong end, ulong flags)
{
	pte_t *pte;
	int nr_frames;

	pte = addr_to_pte(pgdir, start);
	nr_frames = (page_align(end) - start) / FRAME_SIZE;
	for (int i = 0; i < nr_frames; i++, pte++)
		*pte |= flags;
}

/*
 * Allocate memory for pf_info structures.
 */
int grow_frame_pool(void)
{
	struct pf_info *info;
	int next;

	if ((info = kmalloc(FRAME_SIZE)) == NULL)
		return -1;

	next = FRAME_SIZE / sizeof(struct pf_info);

	for (int i = 0; i < next; i++, info++) {
		info->addr = fp_start + (ft_i + i)*FRAME_SIZE;
		list_add_tail(&info->chain, &frame_pool);
		frame_table[ft_i + i] = info;
	}

	ft_i += next;

	return 0;
}		

/*
 * Initialize the frame pool.
 */
int paging_init(ulong start, ulong end)
{
	unsigned nr_frames = (end - start) / FRAME_SIZE;
	frame_table = kmalloc(nr_frames * sizeof(struct pf_info*));
	if (frame_table == NULL)
		return -1;

	fp_start = start;
	fp_end = end;

	first_free = kheap_end / FRAME_SIZE;

	/* disable R/W flag for read-only sections */
	page_attr_off(kernel_pgdir, urostart, uroend, PE_RW);
	page_attr_off(kernel_pgdir, krostart, kroend, PE_RW);

	/* set up page table for temporary mappings */
	kernel_pgdir[addr_to_pdi(TMP_PGTAB_BASE)] =
		kernel_to_phys(tmp_pgtab) | PE_P | PE_RW;

	for (int i = 0; i < 16; i++)
		kernel_pgdir[i] = 0;

	return 0;
}

/*
 * Map a page from the temporary page table to a given physical address.  This
 * function returns an address aliasing the given physical address.
 */
void *kmap_tmp_page(ulong addr)
{
	int i;
	ulong tmp_addr;

	for (i = NR_RESERVED_PAGES; i < 1024; i++) {
		if (!(tmp_pgtab[i] & PE_P))
			break;
	}
	if (i == 1024)
		return NULL;

	tmp_pgtab[i] = addr | PE_P | PE_RW;

	tmp_addr = TMP_PGTAB_BASE + i*FRAME_SIZE;
	asm volatile("invlpg (%0)" : : "b" (tmp_addr));
	return (void*) (tmp_addr + (addr & 0xFFF));
}

static void *kmap_tmp_page_n(ulong addr, unsigned n)
{
	ulong tmp_addr = TMP_PGTAB_BASE + n * FRAME_SIZE;
	tmp_pgtab[n] = addr | PE_P | PE_RW;
	asm volatile("invlpg (%0)" : : "b" (tmp_addr));
	return (void*) tmp_addr;
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
	asm volatile("invlpg (%0)" : : "b" (addr));
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
	for (unsigned i = NR_RESERVED_PAGES; i < 1024; i++, pte++) {
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
void *kmap_tmp_range(pmap_t pgdir, ulong addr, size_t len)
{
	pmap_t pgtab;
	pte_t *pte, *tmp;
	ulong tmp_addr;
	unsigned nr_pages;

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
		*tmp = *pte | PE_RW;
		asm volatile("invlpg (%0)" : : "b" (tmp_addr + i*FRAME_SIZE));
	}

	return (void*) (tmp_addr + (addr & 0xFFF));
}

/*
 * Unmap all pages overlapping the given memory region.
 */
void kunmap_range(void *addrp, size_t len)
{
	pte_t *pte;
	pmap_t pgtab;
	ulong addr = (ulong) addrp;
	unsigned nr_pages = pages_in_range(addr, len);

	pgtab = (pmap_t) (kernel_pgdir[addr_to_pdi(addr)] & ~0xFFF);
	pgtab = kmap_tmp_page_n((ulong)pgtab, 0);
	pte = &pgtab[addr_to_pti(addr)];

	for (unsigned i = 0; i < nr_pages; i++, pte++) {
		*pte = 0;
		asm volatile("invlpg (%0)" : : "b" (addr + i*FRAME_SIZE));
	}
}

int map_pages(pmap_t pgdir, ulong start, int pages, uchar attr,
		struct list_head *page_list)
{
	pte_t *pte;
	pmap_t pgtab;
	struct pf_info *frame;

	pgdir = kmap_tmp_page_n((ulong)pgdir, 2);

	pte = pgdir + addr_to_pdi(start);
	if (!(*pte & PE_P)) {
		if ((frame = kzalloc_frame()) == NULL)
			return -ENOMEM;

		*pte = frame->addr | attr | PE_P;
		list_add_tail(&frame->chain, page_list);
	}

	pgtab = kmap_tmp_page_n((ulong)(*pte & ~0xFFF), 3);

	pte = pgtab + addr_to_pti(start);
	for (int i = 0; i < pages; i++, pte++) {
		if ((frame = kalloc_frame()) == NULL)
			return -ENOMEM;

		*pte = frame->addr | attr | PE_P;
		list_add_tail(&frame->chain, page_list);
	}

	return 0;
}

int copy_from_user(struct pcb *p, void *dst, const void *src, size_t len)
{
	void *addr;
	if ((addr = kmap_tmp_range(p->pgdir, (ulong) src, len)) == 0)
		return -1;

	memcpy(dst, addr, len);
	kunmap_range(addr, len);

	return 0;
}

int copy_to_user(struct pcb *p, void *dst, const void *src, size_t len)
{
	void *addr;
	if ((addr = kmap_tmp_range(p->pgdir, (ulong) dst, len)) == 0)
		return -1;

	memcpy(addr, src, len);
	kunmap_range(addr, len);

	return 0;
}

int copy_through_user(struct pcb *dst_p, struct pcb *src_p, void *dst,
		const void *src, size_t len)
{
	void *dst_addr, *src_addr;

	if ((dst_addr = kmap_tmp_range(dst_p->pgdir, (ulong) dst, len)) == 0)
		return -1;

	if ((src_addr = kmap_tmp_range(src_p->pgdir, (ulong) src, len)) == 0) {
		kunmap_range(dst_addr, len);
		return -1;
	}

	memcpy(dst_addr, src_addr, len);
	kunmap_range(dst_addr, len);
	kunmap_range(src_addr, len);

	return 0;
}

int copy_string_through_user(struct pcb *dst_p, struct pcb *src_p, void *dst,
		const void *src, size_t len)
{
	void *dst_addr, *src_addr;

	if ((dst_addr = kmap_tmp_range(dst_p->pgdir, (ulong) dst, len)) == 0)
		return -1;

	if ((src_addr = kmap_tmp_range(src_p->pgdir, (ulong) src, len)) == 0) {
		kunmap_range(dst_addr, len);
		return -1;
	}

	strncpy(dst_addr, src_addr, len);
	kunmap_range(dst_addr, len);
	kunmap_range(src_addr, len);

	return 0;
}

int copy_user_string(struct pcb *p, char *dst, const char *src, size_t len)
{
	void *addr;
	if ((addr = kmap_tmp_range(p->pgdir, (ulong) src, len)) == 0)
		return -1;

	strncpy(dst, addr, len);
	return 0;
}

/*
 * Allocate and initialize a page directory.  The returned directory will map
 * the kernel, but nothing more.
 */
int address_space_init(struct pcb *p)
{
	struct pf_info *page;
	pmap_t vaddr;
	unsigned pdi;

	struct pf_info *kzalloc_frame(void);
	if ((page = kzalloc_frame()) == NULL)
		return -ENOMEM;

	if ((vaddr = kmap_tmp_page(page->addr)) == NULL) {
		kfree_frame(page);
		return -ENOMEM;
	}

	/* map kernel memory */
	for (pdi = addr_to_pdi((ulong)&KERNEL_PAGE_OFFSET);
			kernel_pgdir[pdi] & PE_P;
			pdi++)
		vaddr[pdi] = kernel_pgdir[pdi];

	pdi = addr_to_pdi(TMP_PGTAB_BASE);
	vaddr[pdi] = kernel_pgdir[pdi];

	list_add(&page->chain, &p->page_mem);
	kunmap_page(vaddr);

	p->pgdir = (void*) page->addr;

	return 0;
}

/*
 * Allocate a page from the frame pool.
 */
struct pf_info *kalloc_frame(void)
{
	struct pf_info *page;

	if (list_empty(&frame_pool) && grow_frame_pool() == -1)
		return NULL;

	page = list_pop(&frame_pool, struct pf_info, chain);
	return page;
}

/*
 * Allocate a page from the frame pool and fill it with zeros.
 */
struct pf_info *kzalloc_frame(void)
{
	struct pf_info *page;
	void *vaddr;

	if ((page = kalloc_frame()) == NULL)
		return NULL;

	if ((vaddr = kmap_tmp_page(page->addr)) == NULL) {
		kfree_frame(page);
		return NULL;
	}

	memset(vaddr, 0, FRAME_SIZE);
	kunmap_page(vaddr);
	return page;
}


/*
 * Return a page to the frame pool.
 */
void kfree_frame(struct pf_info *page)
{
	list_push(&page->chain, &frame_pool);
}

static pmap_t kmap_page_table(ulong addr)
{
	pte_t *pde = addr_to_pde(kernel_pgdir, addr);

	/* allocate page table if one does not already exist */
	if (!(*pde & PE_P)) {
		struct pf_info *frame = kzalloc_frame();
		*pde = frame->addr | PE_P | PE_RW;
		/* update process page direcories */
		for (uint i = 0; i < PT_SIZE; i++) {
			struct pcb *p = &proctab[i];
			if (p->state != STATE_STOPPED) {
				pmap_t pgdir = kmap_tmp_page((ulong)p->pgdir);
				pte_t *upde = addr_to_pde(pgdir, addr);
				*upde = frame->addr | PE_P | PE_RW;
				kunmap_page(pgdir);
			}
		}
	}

	return kmap_tmp_page(*pde & ~0xFFF);
}

#define kunmap_page_table(pgtab) kunmap_page(pgtab)

static void inval_pages(ulong addr, uint n)
{
	for (uint i = 0; i < n; i++)
		asm volatile("invlpg (%0)" : : "b" (addr + n * FRAME_SIZE));
}

void *kalloc_pages(uint n)
{
	uint count = 0;
	uint start = 0;
	uint i = first_free;
	pmap_t pgtab = kmap_page_table(i * FRAME_SIZE);

	/* find n consecutive, free pages */
	for (uint i = first_free; /*TODO*/; i++) {
		if (pgtab[i % 1024] & PE_P) {
			count = 0;
			start = 0;
		} else {
			if (start == 0)
				start = i;
			if (++count == n)
				break;
		}

		/* next page table */
		if ((i+1) % 1024 == 0) {
			kunmap_page_table(pgtab);
			kmap_page_table((i+1) * FRAME_SIZE);
		}
	}
	kunmap_page_table(pgtab);
	// TODO: if (i == LIMIT) fail()

	pgtab = kmap_page_table(start * FRAME_SIZE);

	/* allocate and map frames */
	for (uint i = start; i < start + n; i++) {
		struct pf_info *frame = kalloc_frame();
		pgtab[i % 1024] = frame->addr | PE_P | PE_RW;

		/* next page table */
		if ((i+1) % 1024 == 0) {
			kunmap_page_table(pgtab);
			kmap_page_table((i+1) * FRAME_SIZE);
		}
	}
	kunmap_page_table(pgtab);

	/* update first_free */
	if (start == first_free) {
		pgtab = kmap_page_table((start+n) * FRAME_SIZE);
		for (uint i = start + n; ; i++) {
			if (!(pgtab[i % 1024] & PE_P)) {
				first_free = i;
				break;
			}

			/* next page table */
			if ((i+1) % 1024 == 0) {
				kunmap_page_table(pgtab);
				kmap_page_table((i+1) * FRAME_SIZE);
			}
		}
		kunmap_page_table(pgtab);
		// TODO: check limit
	}
	inval_pages(start * FRAME_SIZE, n);
	return (void*) (start * FRAME_SIZE);
}

static struct pf_info *phys_to_info(ulong addr)
{
	return frame_table[ (addr - fp_start) / FRAME_SIZE ];
}

void kfree_pages(void *addr, uint n)
{
	uint start = (ulong)addr / FRAME_SIZE;
	pmap_t pgtab = kmap_page_table((ulong)addr);

	for (uint i = start; i < start + n; i++) {
		struct pf_info *frame = phys_to_info(pgtab[i % 1024] & ~0xFFF);
		pgtab[i % 1024] = 0;
		kfree_frame(frame);

		/* next page table */
		if ((i+1) % 1024 == 0) {
			kunmap_page_table(pgtab);
			kmap_page_table((i+1) * FRAME_SIZE);
		}
	}
	kunmap_page_table(pgtab);
	// TODO: check limit

	if (start < first_free)
		first_free = start;

	inval_pages((ulong)addr, n);
}
