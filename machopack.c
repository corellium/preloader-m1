/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "iphone-macho.h"

enum itype {
    UNKNOWN,
    PREBOOT,
    KERNEL,
    DTREE };

#define MAXINPUT        16
struct {
    char *fname;
    uint8_t *data;
    uint64_t paddr, vaddr, size, asize, offs;
    enum itype type;
} input[MAXINPUT];
unsigned ninput = 0;

static void strlower(char *dst, const char *src)
{
    while(*src)
        *(dst++) = tolower(*(src++));
}

int main(int argc, char *argv[])
{
    unsigned i, d;
    char *p, *q;
    FILE *f;
    struct mach_header_64 mh = { 0 };
    struct {
        struct segment_command_64 sc;
        struct section_64 se;
    } *sc;
    struct thread_command tc = { 0 };
    uint64_t vpoffs = 0xfffffe0004000000ull - 0x800000000ull;
    uint64_t pbvirt = 0, kevirt = 0, dtphys = 0, hdrvirt = 0, hdrsize = 0;

    if(argc < 2 || argc > MAXINPUT)
        goto usage;

    for(i=0; i<argc-2; i++) {
        input[i].fname = strdup(argv[i+2]);
        p = strchr(input[i].fname, '@');
        if(!p)
            goto usage;
        *(p ++) = 0;
        q = strchr(p, '/');
        if(q) {
            *(q ++) = 0;
            input[i].vaddr = strtoul(q, NULL, 16);
        } else
            input[i].vaddr = 0ul;
        input[i].paddr = strtoul(p, NULL, 16);
        if(input[i].vaddr)
            vpoffs = input[i].vaddr - input[i].paddr;
        f = fopen(input[i].fname, "rb");
        if(!f)
            goto usage;
        fseek(f, 0, SEEK_END);
        input[i].size = ftell(f);
        input[i].asize = ((input[i].size + 16383) >> 14) * 16384;
        fseek(f, 0, SEEK_SET);
        input[i].data = calloc(1, input[i].asize);
        if(!input[i].data)
            goto alloc;
        fread(input[i].data, input[i].size, 1, f);
        fclose(f);
        input[i].type = UNKNOWN;
        if(input[i].size >= 12 && !memcmp(input[i].data + 4, "PREBOOT", 8))
            input[i].type = PREBOOT;
        if(input[i].size >= 0x40 && !memcmp(input[i].data + 0x38, "ARM\x64", 4))
            input[i].type = KERNEL;
        if(input[i].size >= 4 && !memcmp(input[i].data, "\xd0\x0d\xfe\xed", 4))
            input[i].type = DTREE;
        ninput ++;
    }

    input[0].offs = sizeof(mh) + (ninput + 1) * sizeof(sc[0]) + sizeof(tc);
    input[0].offs = ((input[0].offs + 16383) >> 14) * 16384;
    hdrsize = input[0].offs;
    for(i=0; i<ninput; i++) {
        if(!input[i].vaddr)
            input[i].vaddr = input[i].paddr + vpoffs;
        if(!hdrvirt || hdrvirt > input[i].vaddr)
            hdrvirt = input[i].vaddr;
        if(i)
            input[i].offs = input[i-1].offs + input[i-1].asize;
    }
    hdrvirt -= hdrsize;

    sc = calloc(ninput + 1, sizeof(sc[0]));
    if(!sc)
        goto alloc;

    mh.magic = MH_MAGIC_64;
    mh.cputype = CPU_TYPE_ARM64;
    mh.cpusubtype = CPU_SUBTYPE_ARM64;
    mh.filetype = MH_KERNEL;
    mh.ncmds = ninput + 2;
    mh.sizeofcmds = (ninput + 1) * sizeof(sc[0]) + sizeof(tc);
    mh.flags = MH_DYLDLINK;

    sc[0].sc.cmd = LC_SEGMENT_64;
    sc[0].sc.cmdsize = sizeof(sc[0]);
    strcpy(sc[0].sc.segname, "__HEADER");
    sc[0].sc.maxprot = 1;
    sc[0].sc.vmaddr = hdrvirt;
    sc[0].sc.vmsize = hdrsize;
    sc[0].sc.filesize = hdrsize;
    sc[0].sc.initprot = sc[0].sc.maxprot;
    sc[0].sc.nsects = 1;
    sc[0].sc.flags = 0;

    strlower(sc[0].se.sectname, sc[0].sc.segname);
    strcpy(sc[0].se.segname, sc[0].sc.segname);
    sc[0].se.addr = hdrvirt;
    sc[0].se.size = hdrsize;
    sc[0].se.align = 14;

    for(i=1; i<ninput+1; i++) {
        sc[i].sc.cmd = LC_SEGMENT_64;
        sc[i].sc.cmdsize = sizeof(sc[0]);
        switch(input[i-1].type) {
        case PREBOOT:
            strcpy(sc[i].sc.segname, "__TEXT_EXEC");
            sc[i].sc.maxprot = 7;
            sc[i].se.flags = S_ATTR_SOME_INSTRUCTIONS;
            pbvirt = input[i-1].vaddr;
            sc[i].se.align = 16;
            break;
        case KERNEL:
            strcpy(sc[i].sc.segname, "__TEXT");
            sc[i].sc.maxprot = 7;
            sc[i].se.flags = S_ATTR_SOME_INSTRUCTIONS;
            kevirt = input[i-1].vaddr;
            sc[i].se.align = 19;
            break;
        case DTREE:
            strcpy(sc[i].sc.segname, "__DATA_CONST");
            sc[i].sc.maxprot = 3;
            dtphys = input[i-1].paddr;
            sc[i].se.align = 16;
            break;
        default:
            sprintf(sc[i].sc.segname, "__DATA_%d", i);
            sc[i].sc.maxprot = 3;
            sc[i].se.align = 16;
        }
        sc[i].sc.vmaddr = input[i-1].vaddr;
        sc[i].sc.vmsize = input[i-1].asize;
        sc[i].sc.fileoff = input[i-1].offs;
        sc[i].sc.filesize = input[i-1].asize;
        sc[i].sc.initprot = sc[i].sc.maxprot;
        sc[i].sc.nsects = 1;
        sc[i].sc.flags = 0;

        strlower(sc[i].se.sectname, sc[i].sc.segname);
        strcpy(sc[i].se.segname, sc[i].sc.segname);
        sc[i].se.addr = input[i-1].vaddr;
        sc[i].se.size = input[i-1].asize;
        sc[i].se.offset = input[i-1].offs;
    }

    tc.cmd = LC_UNIXTHREAD;
    tc.cmdsize = sizeof(tc);
    tc.flavor = ARM_THREAD_STATE64;
    tc.count = 68;
    tc.s64.pc = pbvirt ? (pbvirt + 0x100) : kevirt;

    p = calloc(16384, 1);
    if(!p)
        goto alloc;

    f = fopen(argv[1], "wb");
    if(!f)
        goto usage;
    fwrite(&mh, 1, sizeof(mh), f);
    fwrite(sc, ninput + 1, sizeof(sc[0]), f);
    fwrite(&tc, 1, sizeof(tc), f);
    for(i=0; i<ninput; i++) {
        d = input[i].offs - ftell(f);
        if(d)
            fwrite(p, d, 1, f);
        fwrite(input[i].data, input[i].asize, 1, f);
    }
    fclose(f);

    return 0;

usage:
    fprintf(stderr, "usage: machopack <output.macho> <input1.bin@paddr[/vaddr]> [<input2.bin@paddr[/vaddr]> ...]\n");
    return 1;

alloc:
    fprintf(stderr, "failed to allocate memory\n");
    return 1;
}
