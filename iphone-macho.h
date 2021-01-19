/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef _IPHONE_MACHO_H
#define _IPHONE_MACHO_H

#include <stdint.h>

struct mach_header {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
};

#define MH_MAGIC        0xfeedface
#define MH_CIGAM        0xcefaedfe

struct mach_header_64 {
    uint32_t magic;
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t filetype;
    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t flags;
    uint32_t reserved;
};

#define MH_MAGIC_64     0xfeedfacf
#define MH_CIGAM_64     0xcffaedfe

#define CPU_TYPE_ARM64  0x0100000c
#define CPU_SUBTYPE_ARM64 0x00000002
#define CPU_SUBTYPE_ARM64E 0xc0000002

#define MH_EXECUTE      2
#define MH_KERNEL       12

#define MH_NOUNDEFS     0x00000001
#define MH_DYLDLINK     0x00000004

struct load_command {
    uint32_t cmd;
    uint32_t cmdsize;
};

#define LC_REQ_DYLD 0x80000000

#define LC_SEGMENT      0x1
#define LC_SYMTAB       0x2
#define LC_UNIXTHREAD   0x5
#define LC_DYSYMTAB     0xb
#define LC_SEGMENT_64   0x19
#define LC_DYLD_INFO    0x22
#define LC_DYLD_INFO_ONLY (0x22|LC_REQ_DYLD)

struct segment_command {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint32_t vmaddr;
    uint32_t vmsize;
    uint32_t fileoff;
    uint32_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
};

struct segment_command_64 {
    uint32_t cmd;
    uint32_t cmdsize;
    char segname[16];
    uint64_t vmaddr;
    uint64_t vmsize;
    uint64_t fileoff;
    uint64_t filesize;
    uint32_t maxprot;
    uint32_t initprot;
    uint32_t nsects;
    uint32_t flags;
};

struct section {
    char sectname[16];
    char segname[16];
    uint32_t addr;
    uint32_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved[2];
};

struct section_64 {
    char sectname[16];
    char segname[16];
    uint64_t addr;
    uint64_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved[3];
};

#define SECTION_TYPE             0x000000ff
#define S_NON_LAZY_SYMBOL_POINTERS      0x6
#define S_ATTR_SOME_INSTRUCTIONS 0x00000400

struct  __attribute__ ((packed)) __arm_thread_state32 {
    uint32_t r[13];
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
    uint32_t cpsr;
};

struct __attribute__ ((packed)) __arm_thread_state64 {
    uint64_t x[29];
    uint64_t fp;
    uint64_t lr;
    uint64_t sp;
    uint64_t pc;
    uint32_t cpsr;
    uint32_t _pad;
};

struct thread_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t flavor;
    uint32_t count;
    union {
        struct __arm_thread_state32 s32;
        struct __arm_thread_state64 s64;
    };
};

#define ARM_THREAD_STATE64 6

struct symtab_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;
};

struct dysymtab_command {
    uint32_t cmd;
    uint32_t cmdsize;

    uint32_t ilocalsym;
    uint32_t nlocalsym;

    uint32_t iextdefsym;
    uint32_t nextdefsym;

    uint32_t iundefsym;
    uint32_t nundefsym;

    uint32_t tocoff;
    uint32_t ntoc;

    uint32_t modtaboff;
    uint32_t nmodtab;

    uint32_t extrefsymoff;
    uint32_t nextrefsyms;

    uint32_t indirectsymoff;
    uint32_t nindirectsyms;

    uint32_t extreloff;
    uint32_t nextrel;

    uint32_t locreloff;
    uint32_t nlocrel;
};

struct dyld_info_command {
    uint32_t cmd;
    uint32_t cmdsize;

    uint32_t rebase_off;
    uint32_t rebase_size;

    uint32_t bind_off;
    uint32_t bind_size;

    uint32_t weak_bind_off;
    uint32_t weak_bind_size;

    uint32_t lazy_bind_off;
    uint32_t lazy_bind_size;

    uint32_t export_off;
    uint32_t export_size;
};

#define REBASE_TYPE_POINTER                                  1
#define REBASE_TYPE_TEXT_ABSOLUTE32                          2
#define REBASE_TYPE_TEXT_PCREL32                             3

#define REBASE_OPCODE_MASK                                   0xF0
#define REBASE_IMMEDIATE_MASK                                0x0F
#define REBASE_OPCODE_DONE                                   0x00
#define REBASE_OPCODE_SET_TYPE_IMM                           0x10
#define REBASE_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB            0x20
#define REBASE_OPCODE_ADD_ADDR_ULEB                          0x30
#define REBASE_OPCODE_ADD_ADDR_IMM_SCALED                    0x40
#define REBASE_OPCODE_DO_REBASE_IMM_TIMES                    0x50
#define REBASE_OPCODE_DO_REBASE_ULEB_TIMES                   0x60
#define REBASE_OPCODE_DO_REBASE_ADD_ADDR_ULEB                0x70
#define REBASE_OPCODE_DO_REBASE_ULEB_TIMES_SKIPPING_ULEB     0x80

#define BIND_TYPE_POINTER                                    1
#define BIND_TYPE_TEXT_ABSOLUTE32                            2
#define BIND_TYPE_TEXT_PCREL32                               3

#define BIND_SPECIAL_DYLIB_SELF                               0
#define BIND_SPECIAL_DYLIB_MAIN_EXECUTABLE                   -1
#define BIND_SPECIAL_DYLIB_FLAT_LOOKUP                       -2

#define BIND_SYMBOL_FLAGS_WEAK_IMPORT                        0x1
#define BIND_SYMBOL_FLAGS_NON_WEAK_DEFINITION                0x8

#define BIND_OPCODE_MASK                                     0xF0
#define BIND_IMMEDIATE_MASK                                  0x0F
#define BIND_OPCODE_DONE                                     0x00
#define BIND_OPCODE_SET_DYLIB_ORDINAL_IMM                    0x10
#define BIND_OPCODE_SET_DYLIB_ORDINAL_ULEB                   0x20
#define BIND_OPCODE_SET_DYLIB_SPECIAL_IMM                    0x30
#define BIND_OPCODE_SET_SYMBOL_TRAILING_FLAGS_IMM            0x40
#define BIND_OPCODE_SET_TYPE_IMM                             0x50
#define BIND_OPCODE_SET_ADDEND_SLEB                          0x60
#define BIND_OPCODE_SET_SEGMENT_AND_OFFSET_ULEB              0x70
#define BIND_OPCODE_ADD_ADDR_ULEB                            0x80
#define BIND_OPCODE_DO_BIND                                  0x90
#define BIND_OPCODE_DO_BIND_ADD_ADDR_ULEB                    0xA0
#define BIND_OPCODE_DO_BIND_ADD_ADDR_IMM_SCALED              0xB0
#define BIND_OPCODE_DO_BIND_ULEB_TIMES_SKIPPING_ULEB         0xC0

#define EXPORT_SYMBOL_FLAGS_KIND_MASK                        0x03
#define EXPORT_SYMBOL_FLAGS_KIND_REGULAR                     0x00
#define EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL                0x01
#define EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION                  0x04
#define EXPORT_SYMBOL_FLAGS_REEXPORT                         0x08
#define EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER                0x10

struct nlist_64 {
    union {
        uint32_t n_strx;
    } n_un;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
};

#define N_STAB          0xe0
#define N_PEXT          0x10
#define N_TYPE          0x0e
#define N_EXT           0x01

#define N_UNDF          0x0
#define N_ABS           0x2
#define N_SECT          0xe
#define N_PBUD          0xc
#define N_INDR          0xa

#define NO_SECT         0
#define MAX_SECT        255

struct relocation_info {
   int32_t  r_address;
   uint32_t r_symbolnum:24,
            r_pcrel:1,
            r_length:2,
            r_extern:1,
            r_type:4;
};

#define R_ABS   0

#define FAT_MAGIC       0xcafebabe
#define FAT_CIGAM       0xbebafeca
#define FAT_MAGIC_64    0xcafebabf
#define FAT_CIGAM_64    0xbfbafeca

struct macho_fat_header {
    uint32_t magic;
    uint32_t nfat_arch;
};

struct macho_fat_arch {
    uint32_t cputype;
    uint32_t cpusubtype;
    uint32_t offset;
    uint32_t size;
    uint32_t align;
};

struct macho_fat_arch_64 {
    uint32_t cputype;
    uint32_t cpusubtype;
    uint64_t offset;
    uint64_t size;
    uint32_t align;
    uint32_t reserved;
};

#endif
