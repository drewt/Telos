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

unsigned long *addr_to_pte (unsigned long *pgdir, unsigned long frame)
{
    unsigned long *pgtab;
    unsigned long pde;

    pde = pgdir[ADDR_TO_PDI (frame)];
    pgtab = (unsigned long*) (pde & ~0xFFF);
    return pgtab + ADDR_TO_PTI (frame);
}

/*-----------------------------------------------------------------------------
 * Initialize the frame pool */
//-----------------------------------------------------------------------------
int paging_init (struct multiboot_info *info)
{
    list_init (&frame_pool);
    frame_table = kmalloc (NR_FRAMES * sizeof (struct pf_info));
    if (frame_table == NULL)
        return -1;

    for (int i = 0; i < NR_FRAMES; i++) {
        frame_table[i].addr = FRAME_POOL_ADDR + (i * FRAME_SIZE);
        list_insert_tail (&frame_pool, (list_entry_t) &frame_table[i]);
    }
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
