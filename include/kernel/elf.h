/*  Copyright 2013-2015 Drew Thoreson
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

#ifndef _KERNEL_ELF_H_
#define _KERNEL_ELF_H_

#include <stdint.h>

#define EI_NIDENT 16

enum {
	EI_MAGIC0     = 0,
	EI_MAGIC1     = 1,
	EI_MAGIC2     = 2,
	EI_MAGIC3     = 3,
	EI_CLASS      = 4,
	EI_DATA       = 5,
	EI_VERSION    = 6,
	EI_OSABI      = 7,
	EI_ABIVERSION = 8,
};

enum {
	ELF_CLASS_NONE = 0,
	ELF_CLASS_32   = 1,
	ELF_CLASS_64   = 2,
};

enum {
	ELF_DATA_NONE = 0,
	ELF_DATA_LSB  = 1,
	ELF_DATA_MSB  = 2,
};

/* ELF object types */
enum {
	ELF_TYPE_NONE	= 0,
	ELF_TYPE_REL	= 1,
	ELF_TYPE_EXEC	= 2,
	ELF_TYPE_DYN	= 3,
	ELF_TYPE_CORE	= 4,
	ELF_TYPE_LOPROC	= 0xFF00,
	ELF_TYPE_HIPROC = 0xFFFF
};

struct elf32_hdr {
	unsigned char ident[EI_NIDENT];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} __packed;

/* ELF program header types */
enum {
	ELF_PHTYPE_NULL		= 0,
	ELF_PHTYPE_LOAD		= 1,
	ELF_PHTYPE_DYNAMIC	= 2,
	ELF_PHTYPE_INTERP	= 3,
	ELF_PHTYPE_NOTE		= 4,
	ELF_PHTYPE_SHLIB	= 5,
	ELF_PHTYPE_PHDR		= 6,
	ELF_PHTYPE_LOPROC	= 0x70000000,
	ELF_PHTYPE_HIPROC	= 0x7FFFFFFF
};

/* ELF program header entry */
struct elf32_phdr {
	uint32_t type;
	uint32_t offset;
	uint32_t vaddr;
	uint32_t paddr;
	uint32_t filesz;
	uint32_t memsz;
	uint32_t flags;
	uint32_t align;
} __packed;

/* ELF section header types */
enum {
	ELF_SHTYPE_NULL		= 0,
	ELF_SHTYPE_PROGBITS	= 1,
	ELF_SHTYPE_SYMTAB	= 2,
	ELF_SHTYPE_STRTAB	= 3,
	ELF_SHTYPE_RELA		= 4,
	ELF_SHTYPE_HASH		= 5,
	ELF_SHTYPE_DYNAMIC	= 6,
	ELF_SHTYPE_NOTE		= 7,
	ELF_SHTYPE_NOBITS	= 8,
	ELF_SHTYPE_REL		= 9,
	ELF_SHTYPE_SHLIB	= 10,
	ELF_SHTYPE_DYNSYM	= 11,
	ELF_SHTYPE_LOPROC	= 0x70000000,
	ELF_SHTYPE_HIPROC	= 0x7FFFFFFF,
	ELF_SHTYPE_LOUSER	= 0x80000000,
	ELF_SHTYPE_HIUSER	= 0xFFFFFFFF
};

/* ELF section header flags */
enum {
	ELF_SHFLAG_WRITE	= 0x1,
	ELF_SHFLAG_ALLOC	= 0x2,
	ELF_SHFLAG_EXECINSTR	= 0x4,
	ELF_SHFLAG_MASKPROC	= 0xF0000000
};

/* ELF section header entry */
struct elf32_shdr {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t offset;
	uint32_t size;
	uint32_t link;
	uint32_t info;
	uint32_t addralign;
	uint32_t entsize;
} __packed;

static inline struct elf32_phdr *elf32_get_phtab(struct elf32_hdr *hdr)
{
	return (struct elf32_phdr*) ((uint32_t) hdr + hdr->phoff);
}

static inline struct elf32_shdr *elf32_get_shtab(struct elf32_hdr *hdr)
{
	return (struct elf32_shdr*) ((uint32_t) hdr + hdr->shoff);
}

static inline struct elf32_phdr *elf32_next_phdr(struct elf32_phdr *phdr,
		struct elf32_hdr *hdr)
{
	return (struct elf32_phdr*) ((ulong) phdr + hdr->phentsize);
}

static inline struct elf32_shdr *elf32_next_shdr(struct elf32_shdr *shdr,
		struct elf32_hdr *hdr)
{
	return (struct elf32_shdr*) ((ulong) shdr + hdr->shentsize);
}

static inline struct elf32_phdr *elf32_get_phdr(struct elf32_hdr *hdr,
		unsigned idx)
{
	return (void*) ((ulong) elf32_get_phtab(hdr) + hdr->phentsize * idx);
}

static inline struct elf32_shdr *elf32_get_shdr(struct elf32_hdr *hdr,
		unsigned idx)
{
	return (void*) ((ulong) elf32_get_shtab(hdr) + hdr->shentsize * idx);
}

static inline char *elf32_get_strtab(struct elf32_hdr *hdr)
{
	struct elf32_shdr *strhdr = elf32_get_shdr(hdr, hdr->shstrndx);
	return (char*) ((ulong) hdr + strhdr->offset);
}

/*
 * elf_shdr_iterate(tab,ent,size)
 *
 *	Generates a for loop, setting ent to
 *	each entry in the ELF section header
 *	table in turn.
 */
#define elf_shdr_iterate(tab,ent,size) \
	for ((ent)=&(tab)[1]; (ent)-(tab) < (signed) size; ent++)

#define elf32_for_each_phdr(pos, i, hdr) \
	for (pos = elf32_get_phtab(hdr), i = 0; \
			i < (hdr)->phnum; \
			pos = elf32_next_phdr(pos, hdr), i++)

#define elf32_for_each_shdr(pos, i, hdr) \
	for (pos = elf32_get_shtab(hdr), i = 0; \
			i < (hdr)->shnum; \
			pos = elf32_next_shdr(pos, hdr), i++)

#endif
