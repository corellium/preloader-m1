/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#include "tunable.h"

static unsigned write_tunable_item(uint32_t *lbuf, uint64_t addr, uint32_t mask, uint32_t val, uint64_t *lreg, unsigned lnreg, const char *ldtnname)
{
    unsigned ireg;
    for(ireg=0; ireg<lnreg; ireg++)
        if(addr >= __builtin_bswap64(lreg[ireg*2]) && addr - __builtin_bswap64(lreg[ireg*2]) < __builtin_bswap64(lreg[ireg*2+1]))
            break;
    if(ireg >= lnreg) {
        if(!lbuf) {
            printf("Address 0x%lx not within range of target (%s).\n", addr, ldtnname);
            warning_count ++;
        }
        return 0;
    }

    if(lbuf) {
        lbuf[0] = __builtin_bswap32((addr - __builtin_bswap64(lreg[ireg*2])) | (ireg << 28));
        lbuf[1] = __builtin_bswap32(mask);
        lbuf[2] = __builtin_bswap32(val);
    }
    return 1;
}

static void swapcpy(void *_d, const void *_s, size_t sz, size_t axor)
{
    uint8_t *d = _d;
    const uint8_t *s = _s;
    size_t i;
    for(i=0; i<sz; i++)
        d[i] = s[i ^ axor];
}

static int rebuild_tunable(void *abuf, unsigned asize, uint64_t *areg, unsigned anreg, uint32_t *lbuf, uint64_t *lreg, unsigned lnreg, unsigned mode, const char *ldtnname, const char *adtnname, const char *adtpname)
{
    unsigned count = 0, aoffs, rid, offs, size;

    switch(mode) {
    case TUNABLE_FANCY:
        for(aoffs=0; aoffs<asize; ) {
            offs = *(uint32_t *)(abuf + aoffs);
            size = offs >> 24;
            offs &= 0xFFFFFF;
            switch(size) {
            case 0:
            case 32:
                count += write_tunable_item(lbuf ? lbuf + count * 3 : NULL, areg[0] + offs, *(uint32_t *)(abuf + aoffs + 4), *(uint32_t *)(abuf + aoffs + 8), lreg, lnreg, ldtnname);
                aoffs += 12;
                break;
            case 255:
                return 0;
            default:
                printf("Tunable %s.%s has unexpected item size %d\n", adtnname, adtpname, size);
                warning_count ++;
                return 0;
            }
        }
        count *= 12;
        break;
    case TUNABLE_LEGACY:
        for(aoffs=0; aoffs<asize; aoffs+=16) {
            rid = *(uint32_t *)(abuf + aoffs);
            if(rid > anreg) {
                printf("Tunable %s.%s has invalid range index %d at %d.\n", adtnname, adtpname, rid, aoffs);
                warning_count ++;
                continue;
            }
            offs = *(uint32_t *)(abuf + aoffs + 4);
            count += write_tunable_item(lbuf ? lbuf + count * 3 : NULL, areg[rid*2] + offs, *(uint32_t *)(abuf + aoffs + 8), *(uint32_t *)(abuf + aoffs + 12), lreg, lnreg, ldtnname);
        }
        count *= 12;
        break;
    case TUNABLE_PCIE:
        for(aoffs=0; aoffs<asize; aoffs+=24) {
            size = *(uint32_t *)(abuf + aoffs + 4);
            if(size != 4) {
                printf("Tunable %s.%s has unexpected item size %d\n", adtnname, adtpname, size);
                warning_count ++;
                continue;
            }
            offs = *(uint32_t *)(abuf + aoffs);
            count += write_tunable_item(lbuf ? lbuf + count * 3 : NULL, areg[0] + offs, *(uint32_t *)(abuf + aoffs + 8), *(uint32_t *)(abuf + aoffs + 16), lreg, lnreg, ldtnname);
        }
        count *= 12;
        break;
    case TUNABLE_PLAIN:
        size = areg[0] & 3;
        count = asize & ((-1ul) << size);
        if(lbuf) {
            if(size)
                swapcpy(lbuf, abuf, count, (1 << size) - 1);
            else
                memcpy(lbuf, abuf, count);
        }
        break;
    }

    return count;
}

void prepare_tunable(dtree *adt, const char *adtnname, const char *adtpname, dtree *ldt, const char *ldtnname, const char *ldtpname, unsigned mode, uint64_t base)
{
    dt_node *adtnode, *ldtnode, *adtrnode;
    dt_prop *adtprop, *adtreg, *ldtprop, *ldtreg;
    int res;
    uint64_t vbase[2] = { base, 0 };

    /* this somewhat optimistically assumes 64-bit address and size in both "reg" props */

    if(!adt)
        return;
    adtnode = dt_find_node(adt, adtnname);
    if(!adtnode) {
        printf("Node '%s' not found in %s device tree.\n", adtnname, "Apple");
        warning_count ++;
        return;
    }
    adtprop = dt_find_prop(adt, adtnode, adtpname);
    if(!adtprop) {
        printf("Property '%s.%s' not found in %s device tree.\n", adtnname, adtpname, "Apple");
        warning_count ++;
        return;
    }

    if(mode == TUNABLE_PCIE_PARENT) {
        adtrnode = adtnode->parent;
        if(!adtrnode)
            return;
        mode = TUNABLE_PCIE;
    } else
        adtrnode = adtnode;
    if(mode == TUNABLE_LEGACY || mode == TUNABLE_PCIE) {
        adtreg = dt_find_prop(adt, adtrnode, "reg");
        if(!adtreg) {
            printf("Property '%s.%s' not found in %s device tree.\n", adtnname, "reg", "Apple");
            warning_count ++;
            return;
        }
    } else
        adtreg = NULL;

    ldtnode = dt_find_node(ldt, ldtnname);
    if(!ldtnode) {
        printf("Node '%s' not found in %s device tree.\n", ldtnname, "Linux");
        warning_count ++;
        return;
    }
    if(mode != TUNABLE_PLAIN) {
        ldtreg = dt_find_prop(ldt, ldtnode, "reg");
        if(!ldtreg) {
            printf("Property '%s.%s' not found in %s device tree.\n", ldtnname, "reg", "Linux");
            warning_count ++;
            return;
        }
    } else
        ldtreg = NULL;

    if(mode == TUNABLE_PCIE) {
        res = base & 0xFF;
        base &= ~0xFFul;
        if(res >= adtreg->size / 16) {
            printf("Tunable %s.%s has invalid range index %d.\n", adtnname, adtpname, res);
            warning_count ++;
        }
        vbase[0] = ((uint64_t *)(adtreg->buf))[res * 2] + base;
        adtreg = NULL;
    }

    res = rebuild_tunable(adtprop->buf, adtprop->size, adtreg ? adtreg->buf : vbase, adtreg ? adtreg->size / 16 : 1,
                          NULL, ldtreg ? ldtreg->buf : NULL, ldtreg ? ldtreg->size / 16 : 0, mode, ldtnname, adtnname, adtpname);
    if(res < 0)
        return;

    ldtprop = dt_set_prop(ldt, ldtnode, ldtpname, NULL, res);
    if(!ldtprop)
        return;

    rebuild_tunable(adtprop->buf, adtprop->size, adtreg ? adtreg->buf : vbase, adtreg ? adtreg->size / 16 : 1,
                    ldtprop->buf, ldtreg ? ldtreg->buf : NULL, ldtreg ? ldtreg->size / 16 : 0, mode, ldtnname, adtnname, adtpname);
}

static int build_fuse_tunable(const struct tunable_fuse_map *map, uint64_t base, uint32_t *lbuf, uint64_t *lreg, unsigned lnreg, const char *ldtnname)
{
    unsigned count = 0, idx;
    uint32_t mask, val = 0;

    for(idx=0; map[idx].src_addr; idx++) {
        if(lbuf)
            val = *(volatile uint32_t *)map[idx].src_addr;
        val = (val >> map[idx].src_lsb) & ((1 << map[idx].src_width) - 1);
        mask = ((1 << map[idx].dst_width) - 1) << map[idx].dst_lsb;
        val = (val << map[idx].dst_lsb) & mask;
        count += write_tunable_item(lbuf ? lbuf + count * 3 : NULL, base + map[idx].dst_offs, mask, val, lreg, lnreg, ldtnname);
    }

    return count;
}

void prepare_fuse_tunable(dtree *ldt, const char *ldtnname, const char *ldtpname, const struct tunable_fuse_map *map, uint64_t base)
{
    dt_node *ldtnode;
    dt_prop *ldtprop, *ldtreg;
    int res;

    ldtnode = dt_find_node(ldt, ldtnname);
    if(!ldtnode) {
        printf("Node '%s' not found in %s device tree.\n", ldtnname, "Linux");
        warning_count ++;
        return;
    }
    ldtreg = dt_find_prop(ldt, ldtnode, "reg");
    if(!ldtreg) {
        printf("Property '%s.%s' not found in %s device tree.\n", ldtnname, "reg", "Linux");
        warning_count ++;
        return;
    }

    res = build_fuse_tunable(map, base, NULL, ldtreg->buf, ldtreg->size / 16, ldtnname);
    if(res < 0)
        return;

    ldtprop = dt_set_prop(ldt, ldtnode, ldtpname, NULL, res * 12);
    if(!ldtprop)
        return;

    build_fuse_tunable(map, base, ldtprop->buf, ldtreg->buf, ldtreg->size / 16, ldtnname);
}
