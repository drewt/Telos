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

#ifndef _KERNEL_MULTIBOOT_H_
#define _KERNEL_MULTIBOOT_H_

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#define _MULTIBOOT_FLAG_SET(info,flag) \
	((info)->flags & (1 << (flag)))

#define MULTIBOOT_MEM_VALID(info)	_MULTIBOOT_FLAG_SET(info, 0)
#define MULTIBOOT_BOOTDEV_VALID(info)	_MULTIBOOT_FLAG_SET(info, 1)
#define MULTIBOOT_CMDLINE_VALID(info)	_MULTIBOOT_FLAG_SET(info, 2)
#define MULTIBOOT_MODS_VALID(info)	_MULTIBOOT_FLAG_SET(info, 3)
#define MULTIBOOT_AOUTSYM_VALID(info)	_MULTIBOOT_FLAG_SET(info, 4)
#define MULTIBOOT_ELFSEC_VALID(info)	_MULTIBOOT_FLAG_SET(info, 5)
#define MULTIBOOT_MMAP_VALID(info)	_MULTIBOOT_FLAG_SET(info, 6)
#define MULTIBOOT_DRIVES_VALID(info)	_MULTIBOOT_FLAG_SET(info, 7)
#define MULTIBOOT_CONFTAB_VALID(info)	_MULTIBOOT_FLAG_SET(info, 8)
#define MULTIBOOT_LDRNAME_VALID(info)	_MULTIBOOT_FLAG_SET(info, 9)
#define MULTIBOOT_APMTAB_VALID(info)	_MULTIBOOT_FLAG_SET(info, 10)
#define MULTIBOOT_VIDEO_VALID(info)	_MULTIBOOT_FLAG_SET(info, 11)

#define MULTIBOOT_MMAP_FREE 1

#define MULTIBOOT_MEM_MAX(info) \
	((unsigned long) (0x100000 + (info)->mem_upper * 1024))

struct aout_symbol_table {
	unsigned long tabsize;
	unsigned long strsize;
	unsigned long addr;
	unsigned long reserved;
};

struct elf_section_header_table {
	unsigned long num;
	unsigned long size;
	unsigned long addr;
	unsigned long shndx;
};

struct multiboot_info {
	unsigned long flags;
	unsigned long mem_lower;
	unsigned long mem_upper;
	unsigned long boot_device;
	unsigned long cmdline;
	unsigned long mods_count;
	unsigned long mods_addr;
	union {
		struct aout_symbol_table aout_sym;
		struct elf_section_header_table elf_sec;
	} _u;
	unsigned long mmap_length;
	unsigned long mmap_addr;
};
#define aout_sym _u.aout_sym
#define elf_sec  _u.elf_sec

struct multiboot_mmap {
	unsigned long size;
	unsigned long addr_low;
	unsigned long addr_high;
	unsigned long len_low;
	unsigned long len_high;
	unsigned long type;
} __attribute__((packed));

/*
 * Returns the next entry in a memory map
 */
static inline struct multiboot_mmap *
multiboot_mmap_next(struct multiboot_mmap* mmap)
{
	return (struct multiboot_mmap*)
		((unsigned long) mmap + mmap->size + sizeof(mmap->size));
}

/*
 * Returns the first mmap entry in a memory map
 */
static inline struct multiboot_mmap *
multiboot_mmap_first(struct multiboot_info *info)
{
	return (struct multiboot_mmap*) info->mmap_addr;
}

/*
 * Tests whether a given mmap entry is the last
 */
static inline int
multiboot_mmap_end(struct multiboot_info *info, struct multiboot_mmap *mmap)
{
	return (unsigned long) mmap >= info->mmap_addr + info->mmap_length;
}

/*
 * multiboot_mmap_iterate(info, mmap)
 *
 *	Generates a for loop, setting mmap to
 *	each entry in the memory map in turn.
 */
#define multiboot_mmap_iterate(info,mmap)		\
	for (mmap = multiboot_mmap_first(info);		\
		!multiboot_mmap_end(info,mmap);		\
		mmap = multiboot_mmap_next(mmap))

#endif
