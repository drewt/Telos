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

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4

#define ADDR_TO_PDI(addr) (((addr) & 0xFFC00000) >> 22)
#define ADDR_TO_PTI(addr) (((addr) & 0x003FF000) >> 12)

/* free list for page heap */
static LIST_HEAD (frame_pool);

/* page frame metadata */
static struct pf_info *frame_table;

static ulong *kernel_pgdir;

//-----------------------------------------------------------------------------
static inline pte_t *addr_to_pte (ulong *pgdir, ulong frame)
{
    pte_t pde = pgdir[ADDR_TO_PDI(frame)];
    pte_t *pgtab = (pte_t*) (pde & ~0xFFF);
    return pgtab + ADDR_TO_PTI (frame);
}

//-----------------------------------------------------------------------------
static inline ulong *addr_to_pde (ulong *pgdir, ulong frame)
{
    return pgdir + ADDR_TO_PDI(frame);
}

//-----------------------------------------------------------------------------
void page_attr_off (ulong *pgdir, ulong start, ulong end, ulong flags)
{
    unsigned long *pte;
    int nr_frames;

    pte = addr_to_pte (pgdir, start);
    nr_frames = (PAGE_ALIGN (end) - start) / FRAME_SIZE;
    for (int i = 0; i < nr_frames; i++, pte++)
        *pte &= ~flags;
}

//-----------------------------------------------------------------------------
void page_attr_on (ulong *pgdir, ulong start, ulong end, ulong flags)
{
    pte_t *pte;
    int nr_frames;

    pte = addr_to_pte (pgdir, start);
    nr_frames = (PAGE_ALIGN (end) - start) / FRAME_SIZE;
    for (int i = 0; i < nr_frames; i++, pte++)
        *pte |= flags;
}

/*-----------------------------------------------------------------------------
 * Initialize the frame pool */
//-----------------------------------------------------------------------------
int paging_init (unsigned long start, unsigned long end)
{
    struct pf_info *page;
    ulong pdi;
    int nr_frames;

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
    kernel_pgdir = (ulong*) page->addr;

    pdi = ADDR_TO_PDI ((ulong) &KERNEL_PAGE_OFFSET);
    kernel_pgdir[pdi] = ((ulong*) &_kernel_pgd)[pdi];

    return 0;
}

//-----------------------------------------------------------------------------
ulong *pgdir_create (list_t page_list)
{
    struct pf_info *page;

    if ((page = kalloc_page ()) == NULL)
        return NULL;

    memcpy ((void*) page->addr, &_kernel_pgd, FRAME_SIZE);

    list_insert_head (page_list, (list_entry_t) page);
    return (ulong*) page->addr;
}

/*-----------------------------------------------------------------------------
 * Allocate a page from the frame pool */
//-----------------------------------------------------------------------------
struct pf_info *kalloc_page (void)
{
    if (list_empty (&frame_pool))
        return NULL;

    return (struct pf_info*) stack_pop (&frame_pool);
}

/*-----------------------------------------------------------------------------
 * Retrun a page to the frame pool */
//-----------------------------------------------------------------------------
void kfree_page (struct pf_info *page)
{
    stack_push (&frame_pool, (list_entry_t) page);
}
