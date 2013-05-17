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

#include <string.h>

#define PE_P  0x1
#define PE_RW 0x2
#define PE_U  0x4

#define ADDR_TO_PDI(addr) (((addr) & 0xFFC00000) >> 22)
#define ADDR_TO_PTI(addr) (((addr) & 0x003FF000) >> 12)

/* free list for page heap */
static list_head_t frame_pool;

/* page frame metadata */
static struct pf_info *frame_table;

static unsigned long *addr_to_pte (unsigned long *pgdir, unsigned long frame)
{
    unsigned long *pgtab;
    unsigned long pde;

    pde = pgdir[ADDR_TO_PDI (frame)];
    pgtab = (unsigned long*) (pde & ~0xFFF);
    return pgtab + ADDR_TO_PTI (frame);
}

void page_protect_disable (unsigned long *pgdir, unsigned long start,
        unsigned long end, unsigned long flags)
{
    unsigned long *pte;
    int nr_frames;

    pte = addr_to_pte (pgdir, start);
    nr_frames = (PAGE_ALIGN (end) - start) / FRAME_SIZE;
    for (int i = 0; i < nr_frames; i++, pte++)
        *pte &= ~flags;
}

/*-----------------------------------------------------------------------------
 * Initialize the frame pool */
//-----------------------------------------------------------------------------
int paging_init (unsigned long start, unsigned long end)
{
    int nr_frames;

    list_init (&frame_pool);

    nr_frames = (end - start) / FRAME_SIZE;
    frame_table = kmalloc (nr_frames * sizeof (struct pf_info));
    if (frame_table == NULL)
        return -1;

    for (int i = 0; i < nr_frames; i++) {
        frame_table[i].addr = start + (i * FRAME_SIZE);
        list_insert_tail (&frame_pool, (list_entry_t) &frame_table[i]);
    }

    /* disable R/W flag for read-only sections */
    page_protect_disable (&_kernel_pgd, (unsigned long) &_urostart,
            (unsigned long) &_uroend, PE_RW);
    page_protect_disable (&_kernel_pgd, (unsigned long) &_krostart,
            (unsigned long) &_kroend, PE_RW);

    return 0;
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
