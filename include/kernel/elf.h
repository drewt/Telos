#ifndef _KERNEL_ELF_H_
#define _KERNEL_ELF_H_

/* elf section header types */
enum {
    ELF_SHTYPE_NULL     = 0,
    ELF_SHTYPE_PROGBITS = 1,
    ELF_SHTYPE_SYMTAB   = 2,
    ELF_SHTYPE_STRTAB   = 3,
    ELF_SHTYPE_RELA     = 4,
    ELF_SHTYPE_HASH     = 5,
    ELF_SHTYPE_DYNAMIC  = 6,
    ELF_SHTYPE_NOTE     = 7,
    ELF_SHTYPE_NOBITS   = 8,
    ELF_SHTYPE_REL      = 9,
    ELF_SHTYPE_SHLIB    = 10,
    ELF_SHTYPE_DYNSYM   = 11,
    ELF_SHTYPE_LOPROC   = 0x70000000,
    ELF_SHTYPE_HIPROC   = 0x7FFFFFFF,
    ELF_SHTYPE_LOUSER   = 0x80000000,
    ELF_SHTYPE_HIUSER   = 0xFFFFFFFF
};

/* elf section header flags */
enum {
    ELF_SHFLAG_WRITE     = 0x1,
    ELF_SHFLAG_ALLOC     = 0x2,
    ELF_SHFLAG_EXECINSTR = 0x4,
    ELF_SHFLAG_MASKPROC  = 0xF0000000
};

/* elf section header entry */
struct elf32_shdr {
    unsigned long sh_name;
    unsigned long sh_type;
    unsigned long sh_flags;
    unsigned long sh_addr;
    unsigned long sh_offset;
    unsigned long sh_size;
    unsigned long sh_link;
    unsigned long sh_info;
    unsigned long sh_addralign;
    unsigned long sh_entsize;
};

/*
 * elf_shdr_iterate(tab,ent,size)
 *
 *      Generates a for loop, setting ent to
 *      each entry in the elf section header
 *      table in turn.
 */
#define elf_shdr_iterate(tab,ent,size) \
    for ((ent)=&(tab)[1]; (ent)-(tab) < (signed) size; ent++)

#endif /* _KERNEL_ELF_H_ */
