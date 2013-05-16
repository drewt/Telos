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

/* free list for page heap */
static list_head_t frame_pool;

/* page frame metadata */
static struct pf_info *frame_table;

unsigned long *kernel_page_dir;

/*-----------------------------------------------------------------------------
 * Initialize the frame pool */
//-----------------------------------------------------------------------------
static int init_frame_pool (void)
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
 * Set up the kernel's page tables */
//-----------------------------------------------------------------------------
int paging_init (struct multiboot_info *info)
{
    struct pf_info *kdir;
    unsigned long address;

    if (init_frame_pool () == -1)
        return -1;

    if ((kdir = kalloc_page ()) == NULL)
        return -1;

    kernel_page_dir = (unsigned long*) kdir->addr;

    memset (kernel_page_dir, 0, 4096);

    address = 0;
    for (int tab = 0; tab < 4; tab++) {
        struct pf_info *ktable;
        unsigned long *page_table;

        if ((ktable = kalloc_page ()) == NULL)
            return -1;

        page_table = (unsigned long*) ktable->addr;
        for (int i = 0; i < 1024; i++, address += 4096)
            page_table[i] = address | PE_U | PE_RW | PE_P;

        kernel_page_dir[tab] = ktable->addr | PE_U | PE_RW | PE_P;
    }

    set_page_directory (kernel_page_dir);
    enable_paging ();

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
