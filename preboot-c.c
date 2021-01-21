/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#include "libc.h"
#include "dtree.h"
#include "adtree.h"

#define BOOT_LINE_LENGTH        256
struct iphone_boot_args {
    uint16_t revision;
    uint16_t version;
    uint64_t virt_base;
    uint64_t phys_base;
    uint64_t mem_size;
    uint64_t top_of_kernel;
    struct {
        uint64_t phys, display, stride;
        uint64_t width, height, depth;
    } video;
    uint32_t machine_type;
    uint64_t dtree_virt; 
    uint32_t dtree_size;
    char cmdline[BOOT_LINE_LENGTH];
};

static unsigned warning_count;

void setarena(uint64_t base, uint64_t size);

#define DISP0_SURF0_PIXFMT      0x230850030

static void configure_x8r8g8b8(void)
{
    *(volatile uint32_t *)DISP0_SURF0_PIXFMT = 0x5000; /* pixfmt: x8r8g8b8. if you try other modes they are pretty ugly */
}

#define TUNABLE_LEGACY 0
#define TUNABLE_FANCY  1

static unsigned write_tunable_item(uint32_t *lbuf, uint64_t addr, uint32_t mask, uint32_t val, uint64_t *lreg, unsigned lnreg, const char *ldtnname)
{
    unsigned ireg;
    for(ireg=0; ireg<lnreg; ireg++)
        if(addr >= lreg[ireg*2] && addr - lreg[ireg*2] < lreg[ireg*2+1])
            break;
    if(ireg >= lnreg) {
        if(lbuf) {
            printf("Address 0x%lx not within range of target (%s).\n", addr, ldtnname);
            warning_count ++;
        }
        return 0;
    }

    if(lbuf) {
        lbuf[0] = __builtin_bswap32((addr - lreg[ireg*2]) | (ireg << 28));
        lbuf[1] = __builtin_bswap32(mask);
        lbuf[2] = __builtin_bswap32(val);
    }
    return 1;
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
        break;
    }

    return count;
}

static void prepare_tunable(dtree *adt, const char *adtnname, const char *adtpname, dtree *ldt, const char *ldtnname, const char *ldtpname, unsigned mode, uint64_t base)
{
    dt_node *adtnode, *ldtnode;
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
    if(mode == TUNABLE_LEGACY) {
        adtreg = dt_find_prop(adt, adtnode, "reg");
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
    ldtreg = dt_find_prop(ldt, ldtnode, "reg");
    if(!ldtreg) {
        printf("Property '%s.%s' not found in %s device tree.\n", ldtnname, "reg", "Linux");
        warning_count ++;
        return;
    }

    res = rebuild_tunable(adtprop->buf, adtprop->size, adtreg ? adtreg->buf : vbase, adtreg ? adtreg->size / 16 : 1, NULL, ldtreg->buf, ldtreg->size / 16, mode, ldtnname, adtnname, adtpname);
    if(res < 0)
        return;

    ldtprop = dt_set_prop(ldt, ldtnode, ldtpname, NULL, res * 12);
    if(!ldtprop)
        return;

    rebuild_tunable(adtprop->buf, adtprop->size, adtreg ? adtreg->buf : vbase, adtreg ? adtreg->size / 16 : 1, ldtprop->buf, ldtreg->buf, ldtreg->size / 16, mode, ldtnname, adtnname, adtpname);
}

void loader_main(void *linux_dtb, struct iphone_boot_args *bootargs, uint64_t smpentry, uint64_t rvbar)
{
    dtree *linux_dt, *apple_dt;
    dt_node *node;
    dt_prop *prop;
    uint64_t memsize, base;
    unsigned i;

    warning_count = 0;
    memsize = (bootargs->mem_size + 0x3ffffffful) & ~0x3ffffffful;

    setarena(0x880000000ul, 0x100000);
    setvideo((void *)bootargs->video.phys, bootargs->video.width, bootargs->video.height, bootargs->video.stride);

    printf("Starting Linux loader stub.\n");

    linux_dt = dt_parse_dtb(linux_dtb, 0x20000);
    if(!linux_dt) {
        printf("Failed parsing Linux device-tree.\n");
        return;
    }

    apple_dt = adt_parse((void *)(bootargs->dtree_virt - bootargs->virt_base + bootargs->phys_base), bootargs->dtree_size);
    if(!apple_dt) {
        printf("Failed parsing Apple device-tree.\n");
        warning_count ++;
    }

    node = dt_find_node(linux_dt, "/framebuffer");
    if(node) {
        prop = dt_find_prop(linux_dt, node, "reg");
        if(prop) {
            dt_put64be(prop->buf, bootargs->video.phys);
            dt_put64be(prop->buf + 8, (bootargs->video.height * bootargs->video.stride * 4 + 0x3FFFul) & ~0x3FFFul);
        }

        prop = dt_find_prop(linux_dt, node, "width");
        if(prop)
            dt_put32be(prop->buf, bootargs->video.width);

        prop = dt_find_prop(linux_dt, node, "height");
        if(prop)
            dt_put32be(prop->buf, bootargs->video.height);

        prop = dt_find_prop(linux_dt, node, "stride");
        if(prop)
            dt_put32be(prop->buf, bootargs->video.stride);

        dt_set_prop(linux_dt, node, "format", "x8r8g8b8", 9);
    }

    node = dt_find_node(linux_dt, "/memory");
    if(node) {
        prop = dt_find_prop(linux_dt, node, "reg");
        if(prop)
            dt_put64be(prop->buf + 8, memsize);
    }

    for(i=0; ; i++) {
        node = dt_find_node_idx(linux_dt, NULL, "/reserved-memory/fw_area", i);
        if(!node)
            break;

        prop = dt_find_prop(linux_dt, node, "reg");
        if(prop) {
            base = dt_get64be(prop->buf);
            if(base >= 0x900000000ul) {
                base += memsize - 0x200000000ul;
                dt_put64be(prop->buf, base);
            }
        }
    }

    node = dt_find_node(linux_dt, "/reserved-memory/smpentry");
    if(node) {
        prop = dt_find_prop(linux_dt, node, "reg");
        if(prop)
            dt_put64be(prop->buf, smpentry);
    }

    node = dt_find_node(linux_dt, "/soc/applestart");
    if(node) {
        prop = dt_find_prop(linux_dt, node, "reg");
        if(prop)
            for(i=0; i<prop->size/48; i++)
                dt_put64be(prop->buf + 48 * i + 16, rvbar + 8 * i);
    }

    prepare_tunable(apple_dt, "/arm-io/atc-phy0", "tunable_ATC0AXI2AF", linux_dt, "/soc/usb_drd0", "tunable-ATC0AXI2AF", TUNABLE_FANCY, 0);
    prepare_tunable(apple_dt, "/arm-io/usb-drd0", "tunable",            linux_dt, "/soc/usb_drd0", "tunable",            TUNABLE_LEGACY, 0x380000000);
    prepare_tunable(apple_dt, "/arm-io/atc-phy1", "tunable_ATC0AXI2AF", linux_dt, "/soc/usb_drd1", "tunable-ATC0AXI2AF", TUNABLE_FANCY, 0);
    prepare_tunable(apple_dt, "/arm-io/usb-drd1", "tunable",            linux_dt, "/soc/usb_drd1", "tunable",            TUNABLE_LEGACY, 0x500000000);

    configure_x8r8g8b8();

    if(warning_count) {
        printf("%d warnings; waiting to let you see them.\n");
        udelay(7000000);
    }

    printf("Loader complete, relocating kernel...\n");
    dt_write_dtb(linux_dt, linux_dtb, 0x20000);
    dt_free(linux_dt);
}

void smp_main(unsigned cpu)
{
    uint64_t mpidr_el1, midr_el1;
    srread(mpidr_el1, mpidr_el1);
    srread(midr_el1, midr_el1);
    printf("CPU%d: MPIDR %016lx, MIDR %016lx\n", cpu, mpidr_el1, midr_el1);
}
