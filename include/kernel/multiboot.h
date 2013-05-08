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

#ifndef __MULTIBOOT_H_
#define __MULTIBOOT_H_

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

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
    } u;
    unsigned long mmap_length;
    unsigned long mmap_addr;
};

struct multiboot_mmap_entry {
    unsigned long size;
    unsigned long addr_low;
    unsigned long addr_high;
    unsigned long len_low;
    unsigned long len_high;
    unsigned long type;
} __attribute__((packed));

#endif // __MULTIBOOT_H_
