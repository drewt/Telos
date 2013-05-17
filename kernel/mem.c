/*  Copyright 2013 Drew T.
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
 * mem.c
 *
 * Simple memory manager for shared heap.
 *
 */

#include <kernel/common.h>
#include <kernel/multiboot.h>
#include <kernel/elf.h>
#include <kernel/list.h>
#include <kernel/mem.h>

#include <string.h> /* memcpy */

/*
 * unsigned long PARAGRAPH_ALIGN (unsigned long a)
 *      Takes an address and rounds it up to the nearest paragraph boundary.
 */
#define PARAGRAPH_ALIGN(a) \
    ((a) & 0xF ? ((a) + 0x10) & ~0xF : (a))

/* magic numbers for kernel heap headers */
#define MAGIC_OK   0x600DC0DE
#define MAGIC_FREE 0xF2EEB10C

/* free list for kernel heap */
static list_head_t free_list;

struct mem_area {
    list_chain_t chain;
    unsigned long addr;
    unsigned long size;
};

/* Statically allocated space for use by mem_init() */
int rmem_i = 0;
struct mem_area rmem[256];

/* ELF section headers */
static struct elf32_shdr elf_shtab[32];
static char elf_strtab[512];

/*-----------------------------------------------------------------------------
 * Marks a memory area as reserved, inserting it into a sorted list of reserved
 * memory areas */
//-----------------------------------------------------------------------------
static void mark_reserved (list_t memory, unsigned long addr,
        unsigned long size)
{
    struct mem_area *it, *block;
    
    block = &rmem[rmem_i++];
    block->addr = addr;
    block->size = size;

    if (list_empty (memory)) {
        list_insert_head (memory, (list_entry_t) block);
        return;
    }

    /* iterate until finding the block that 'block' belongs before */
    list_iterate (memory, it, struct mem_area*, chain) {
        if (addr < it->addr) {
            unsigned long diff = it->addr - addr;
            if (diff >= size)
                break; // 'block' goes before 'it'
            kprintf ("*** TODO ***: overlapping allocation\n");
            return; // overlap
        }

        unsigned long diff = addr - it->addr;
        if (diff < it->size) {
            if (diff + size <= it->size)
                return; // already reserved
            kprintf ("*** TODO ***: overlapping allocation\n");
            return; // overlap
        }
    }

    insqueue ((list_entry_t) block, list_prev ((list_entry_t) it));
}

/*-----------------------------------------------------------------------------
 * Initializes the free list given a list of reserved memory areas */
//-----------------------------------------------------------------------------
static void init_free_list (list_t res_list, unsigned long limit)
{
    struct mem_header *head;
    struct mem_area *rsv;

    rsv = (struct mem_area*) list_remove_head (res_list);
    head = (struct mem_header*) (rsv->addr + rsv->size);
    head->size = limit - (unsigned long) head;
    head->magic = MAGIC_FREE;
    list_insert_head (&free_list, (list_entry_t) head);

    /* punch holes in the free list based on entries in res_list */
    list_iterate (res_list, rsv, struct mem_area*, chain) {
        struct mem_header *tail, *new;
        tail = (struct mem_header*) list_last (&free_list);
        new = (struct mem_header*) (rsv->addr + rsv->size); // XXX: overflow!

        if ((unsigned long) new >= limit) {
            tail->size = rsv->addr - (unsigned long) tail->data;
            break;
        }

        unsigned long diff = new->data - tail->data;
        new->size = tail->size - diff;
        new->magic = MAGIC_FREE;

        tail->size = rsv->addr - (unsigned long) tail->data;

        list_insert_tail (&free_list, (list_entry_t) new);
    }
}

/*-----------------------------------------------------------------------------
 * Initializes the memory system */
//-----------------------------------------------------------------------------
unsigned long mem_init (struct multiboot_info *info)
{
    struct elf32_shdr *str_hdr;
    list_head_t res_list;

    // copy ELF section headers & string table into kernel memory
    memcpy (elf_shtab, (char*) info->elf_sec.addr,
            info->elf_sec.num * info->elf_sec.size);
    str_hdr = &elf_shtab[info->elf_sec.shndx];
    memcpy (elf_strtab, (char*) str_hdr->sh_addr, str_hdr->sh_size);

    if (!MULTIBOOT_MEM_VALID (info)) {
        wprints ("failed to detect memory limits; assuming 8MB total");
        info->mem_upper = 0x800000;
    }

    list_init (&res_list);
    list_init (&free_list);

    mark_reserved (&res_list, 0x00000000, PAGE_ALIGN((unsigned long) &uend));
    mark_reserved (&res_list, 0x00400000, 0x00400000);
    init_free_list (&res_list, MULTIBOOT_MEM_MAX (info));
    paging_init (0x00400000, 0x00800000);
    return MULTIBOOT_MEM_MAX (info) - (unsigned long) &uend;
}

/*-----------------------------------------------------------------------------
 * Allocates size bytes of memory, returning a pointer to the start of the
 * allocated block.  If hdr is not NULL and the call succeeds in allocating
 * size bytes of memory, *hdr will point to the struct mem_header corresponding
 * to the allocated block when this function returns */
//-----------------------------------------------------------------------------
void *hmalloc (unsigned int size, struct mem_header **hdr)
{
    struct mem_header *p, *r;

    size = PARAGRAPH_ALIGN (size);

    // find a large enough segment of free memory
    list_iterate (&free_list, p, struct mem_header*, chain) {
        if (p->size >= size)
            break;
    }

    // not enough memory
    if (list_end (&free_list, (list_entry_t) p))
        return NULL;

    // if p is a perfect fit...
    if (p->size - size <= sizeof (struct mem_header)) {
        p->magic = MAGIC_OK;
        list_remove (&free_list, (list_entry_t) p);
    } else {
        // split p into adjacent segments p and r
        r = (struct mem_header*) (p->data + size);
        *r = *p;

        r->size = p->size - size - sizeof (struct mem_header);
        p->size = size;
        r->magic = MAGIC_FREE;
        p->magic = MAGIC_OK;

        // replace p with r in the free list
        list_replace_entry ((list_entry_t) p, (list_entry_t) r);
    }

    if (hdr)
        *hdr = p;

    return p->data;
}

/*-----------------------------------------------------------------------------
 * Returns a segment of previously allocated memory to the free list, given the
 * header for the segment */
//-----------------------------------------------------------------------------
void hfree (struct mem_header *hdr)
{
    // check that the supplied address is sane
    if (hdr->magic == MAGIC_FREE) {
        kprintf ("kfree(): detected double free\n");
        return;
    }
    if (hdr->magic != MAGIC_OK) {
        kprintf ("kfree(): detected double free or corruption\n");
        return;
    }

    // insert freed at the beginning of the free list
    list_insert_head (&free_list, (list_entry_t) hdr);
}
