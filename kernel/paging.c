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

/* 
 * kernel/paging.c
 *  
 *  Code for managing page tables and the like.
 */

#include <kernel/i386.h>
#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <kernel/mem.h>
#include <kernel/process.h>

#include <string.h>

#define ADDR_TO_PDI(addr) (((addr) & 0xFFC00000) >> 22)
#define ADDR_TO_PTI(addr) (((addr) & 0x003FF000) >> 12)

#define PAGES_IN_RANGE(addr,len) \
    ((((addr & 0xFFF) + len) / FRAME_SIZE) + 1)

/* page table for temporary mappings */
pte_t *tmp_pgtab;

/* free list for page heap */
static LIST_HEAD (frame_pool);

/* page frame metadata */
static struct pf_info *frame_table;

static pmap_t kernel_pgdir;

//-----------------------------------------------------------------------------
static inline pte_t *addr_to_pte (pmap_t pgdir, ulong frame)
{
    pte_t pde = pgdir[ADDR_TO_PDI(frame)];
    pte_t *pgtab = (pte_t*) (pde & ~0xFFF);
    return pgtab + ADDR_TO_PTI (frame);
}

//-----------------------------------------------------------------------------
static inline ulong *addr_to_pde (pmap_t pgdir, ulong frame)
{
    return pgdir + ADDR_TO_PDI(frame);
}

//-----------------------------------------------------------------------------
void page_attr_off (pmap_t pgdir, ulong start, ulong end, ulong flags)
{
    pte_t *pte;
    int nr_frames;

    pte = addr_to_pte (pgdir, start);
    nr_frames = (PAGE_ALIGN (end) - start) / FRAME_SIZE;
    for (int i = 0; i < nr_frames; i++, pte++)
        *pte &= ~flags;
}

//-----------------------------------------------------------------------------
void page_attr_on (pmap_t pgdir, ulong start, ulong end, ulong flags)
{
    pte_t *pte;
    int nr_frames;

    pte = addr_to_pte (pgdir, start);
    nr_frames = (PAGE_ALIGN (end) - start) / FRAME_SIZE;
    for (int i = 0; i < nr_frames; i++, pte++)
        *pte |= flags;
}

ulong kmap_tmp_range (pmap_t pgdir, ulong addr, size_t len);
void kunmap_range (ulong addr, size_t len);
ulong kmap_tmp_page (pmap_t pgdir, ulong addr);
/*-----------------------------------------------------------------------------
 * Initialize the frame pool */
//-----------------------------------------------------------------------------
int paging_init (unsigned long start, unsigned long end)
{
    struct pf_info *page;
    int nr_frames, pdi;

    nr_frames = (end - start) / FRAME_SIZE;
    frame_table = kmalloc (nr_frames * sizeof (struct pf_info));
    if (frame_table == NULL)
        return -1;

    for (int i = 0; i < nr_frames; i++) {
        frame_table[i].addr = start + (i * FRAME_SIZE);
        list_insert_tail (&frame_pool, (list_entry_t) &frame_table[i]);
    }

    /* disable R/W flag for read-only sections */
    page_attr_off (&_kernel_pgd, (ulong) &_urostart, (ulong) &_uroend, PE_RW);
    page_attr_off (&_kernel_pgd, (ulong) &_krostart, (ulong) &_kroend, PE_RW);

    /* make a 'dummmy' kernel page table that only maps the kernel */
    page = kalloc_page ();
    memset ((void*) page->addr, 0, FRAME_SIZE);
    kernel_pgdir = (pmap_t) page->addr;

    pdi = ADDR_TO_PDI ((ulong) &KERNEL_PAGE_OFFSET);
    kernel_pgdir[pdi] = ((pmap_t) &_kernel_pgd)[pdi];

    /* set up page table for temporary mappings */
    page = kalloc_page ();
    memset ((void*) page->addr, 0, FRAME_SIZE);
    tmp_pgtab = (pte_t*) page->addr;
    ((pmap_t)&_kernel_pgd)[ADDR_TO_PDI(0xB0400000)] =
            page->addr | PE_P | PE_RW;

    return 0;
}

//-----------------------------------------------------------------------------
int map_pages (pmap_t pgdir, ulong start, int pages, uchar attr,
        list_t page_list)
{
    pte_t *pte;
    struct pf_info *frame;

    pte = pgdir + ADDR_TO_PDI (start);
    if (!(*pte & PE_P)) {
        if ((frame = kalloc_page ()) == NULL)
            return -ENOMEM;
        memset ((void*) frame->addr, 0, FRAME_SIZE);
        *pte = frame->addr | attr | PE_P;
        list_insert_tail (page_list, (list_entry_t) frame);
    }

    pte = (pte_t*) (*pte & ~0xFFF) + ADDR_TO_PTI (start);
    for (int i = 0; i < pages; i++, pte++) {
        if ((frame = kalloc_page ()) == NULL)
            return -ENOMEM;
        *pte = frame->addr | attr | PE_P;
        list_insert_tail (page_list, (list_entry_t) frame);
    }

    return 0;
}

//-----------------------------------------------------------------------------
ulong virt_to_phys (pmap_t pgdir, ulong addr)
{
    pte_t *pte;

    pte = pgdir + ADDR_TO_PDI (addr);
    if ((*pte & PE_P) == 0)
        return 0;

    pte = (pte_t*) (*pte & ~0xFFF) + ADDR_TO_PTI (addr);
    if ((*pte & PE_P) == 0)
        return 0;

    return (*pte & ~0xFFF) | (addr & 0xFFF);
}

//-----------------------------------------------------------------------------
static inline ulong get_tmp_pte (pte_t **dst)
{
    pte_t *pte = tmp_pgtab;
    for (int i = 0; i < 1024; i++, pte++) {
        if (!(*pte & PE_P)) {
            *dst = pte;
            return 0xB0400000 + i*FRAME_SIZE;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
ulong kmap_tmp_page (pmap_t pgdir, ulong addr)
{
    pte_t *pte, *tmp;
    ulong tmp_addr;

    if ((tmp_addr = get_tmp_pte (&tmp)) == 0)
        return 0;

    pte  = addr_to_pte (pgdir, addr);
    *tmp = *pte | PE_RW;

    asm volatile ("invlpg (%0)" : : "b" (tmp_addr));
    return tmp_addr + (addr & 0xFFF);
}

//-----------------------------------------------------------------------------
void kunmap_page (ulong addr)
{
    pte_t *pte;

    pte = addr_to_pte (&_kernel_pgd, addr);
    *pte = 0;
    asm volatile ("invlpg (%0)" : : "b" (addr));
}

/* TODO: make this more efficient (bitmap?) */
//-----------------------------------------------------------------------------
static ulong get_tmp_ptes (pte_t **dst, unsigned nr_pages)
{
    pte_t *pte, *first = NULL;
    ulong first_addr;
    unsigned count = 0;
    
    pte = tmp_pgtab;

    for (int i = 0; i < 1024; i++, pte++) {
        if (!(*pte & PE_P)) {
            if (first == NULL) {
                first = pte;
                first_addr = i*FRAME_SIZE;
            }
            if (++count >= nr_pages) {
                *dst = first;
                return 0xB0400000 + first_addr;
            }
        } else {
            first = NULL;
            count = 0;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
ulong kmap_tmp_range (pmap_t pgdir, ulong addr, size_t len)
{
    pte_t *pte, *tmp;
    ulong tmp_addr;
    unsigned nr_pages;

    nr_pages = PAGES_IN_RANGE (addr, len);

    if ((tmp_addr = get_tmp_ptes (&tmp, nr_pages)) == 0)
        return 0;

    pte = addr_to_pte (pgdir, addr);
    for (unsigned i = 0; i < nr_pages; i++, pte++, tmp++) {
        *tmp = *pte | PE_RW;
        asm volatile ("invlpg (%0)" : : "b" (tmp_addr + i*FRAME_SIZE));
    }
    return tmp_addr + (addr & 0xFFF);
}

//-----------------------------------------------------------------------------
void kunmap_range (ulong addr, size_t len)
{
    pte_t *pte;
    unsigned nr_pages;

    nr_pages = PAGES_IN_RANGE (addr, len);
    pte = addr_to_pte (&_kernel_pgd, addr);
    for (unsigned i = 0; i < nr_pages; i++, pte++) {
        *pte = 0;
        asm volatile ("invlpg (%0)" : : "b" (addr + i*FRAME_SIZE));
    }
}

//-----------------------------------------------------------------------------
int copy_from_userspace (pmap_t pgdir, void *dst, const void *src, size_t len)
{
    ulong addr;

    if ((addr = kmap_tmp_range (pgdir, (ulong) src, len)) == 0)
        return -1;

    memcpy (dst, (void*) addr, len);
    kunmap_range (addr, len);

    return 0;
}

//-----------------------------------------------------------------------------
int copy_to_userspace (pmap_t pgdir, void *dst, const void *src, size_t len)
{
    ulong addr;

    if ((addr = kmap_tmp_range (pgdir, (ulong) dst, len)) == 0)
        return -1;

    memcpy ((void*) addr, src, len);
    kunmap_range (addr, len);

    return 0;
}

//-----------------------------------------------------------------------------
int copy_through_userspace (pmap_t dst_dir, pmap_t src_dir, void *dst,
        const void *src, size_t len)
{
    ulong dst_addr, src_addr;

    if ((dst_addr = kmap_tmp_range (dst_dir, (ulong) dst, len)) == 0)
        return -1;

    if ((src_addr = kmap_tmp_range (src_dir, (ulong) src, len)) == 0) {
        kunmap_range (dst_addr, len);
        return -1;
    }

    memcpy ((void*) dst_addr, (void*) src_addr, len);
    kunmap_range (dst_addr, len);
    kunmap_range (src_addr, len);

    return 0;
}

//-----------------------------------------------------------------------------
int copy_user_string (pmap_t pgdir, char *dst, const char *src, size_t len)
{
    ulong addr;

    if ((addr = kmap_tmp_range (pgdir, (ulong) src, len)) == 0)
        return -1;

    strncpy (dst, (char*) addr, len);
    return 0;
}

//-----------------------------------------------------------------------------
pmap_t pgdir_create (list_t page_list)
{
    struct pf_info *page;

    if ((page = kalloc_page ()) == NULL)
        return NULL;

    memcpy ((pmap_t) page->addr, kernel_pgdir, FRAME_SIZE);

    list_insert_head (page_list, (list_entry_t) page);
    return (pmap_t) page->addr;
}

/*-----------------------------------------------------------------------------
 * Allocate a page from the frame pool */
//-----------------------------------------------------------------------------
struct pf_info *kalloc_page (void)
{
    struct pf_info *page;

    if (list_empty (&frame_pool))
        return NULL;

    page = (void*) stack_pop (&frame_pool);
    return page;
}

/*-----------------------------------------------------------------------------
 * Retrun a page to the frame pool */
//-----------------------------------------------------------------------------
void kfree_page (struct pf_info *page)
{
    stack_push (&frame_pool, (list_entry_t) page);
}
