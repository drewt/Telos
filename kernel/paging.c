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

    return 0;
}

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

int copy_user_string (pmap_t pgdir, char *dst, const char *src, size_t len)
{
    char *p_src = (char*) virt_to_phys (pgdir, (ulong) src);
    if (p_src == NULL)
        return -1;

    strncpy (dst, p_src, len);
    return 0;
}

int copy_from_userspace (pmap_t pgdir, void *dst, const void *src, size_t len)
{
    void *p_src = (void*) virt_to_phys (pgdir, (ulong) src);
    if (p_src == NULL)
        return -1;

    memcpy (dst, p_src, len);
    return 0;
}

int copy_to_userspace (pmap_t pgdir, void *dst, const void *src, size_t len)
{
    void *p_dst = (void*) virt_to_phys (pgdir, (ulong) dst);
    if (p_dst == NULL)
        return -1;

    memcpy (p_dst, src, len);
    return 0;
}

int copy_through_userspace (pmap_t dst_dir, pmap_t src_dir, void *dst,
        const void *src, size_t len)
{
    void *p_dst, *p_src;

    p_dst = (void*) virt_to_phys (dst_dir, (ulong) dst);
    p_src = (void*) virt_to_phys (src_dir, (ulong) src);
    if (p_dst == NULL || p_src == NULL)
        return -1;

    memcpy (p_dst, p_src, len);
    return 0;
}

//-----------------------------------------------------------------------------
pmap_t pgdir_create (list_t page_list)
{
    struct pf_info *page;

    if ((page = kalloc_page ()) == NULL)
        return NULL;

    memcpy ((void*) page->addr, &_kernel_pgd, FRAME_SIZE);
    //memcpy ((pmap_t) page->addr, kernel_pgdir, FRAME_SIZE);

    list_insert_head (page_list, (list_entry_t) page);
    return (pmap_t) page->addr;
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
