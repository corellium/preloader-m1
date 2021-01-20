/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#include "libc.h"
#include "dtree.h"

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

void setarena(uint64_t base, uint64_t size);

#define DISP0_SURF0_PIXFMT      0x230850030
#define DISP0_SURF0_CSCBYPASS   0x230850178
#define DISP0_SURF0_CSCMATRIX   0x230850188

static void configure_x8b8g8r8(void)
{
    unsigned i, j;
    *(volatile uint32_t *)DISP0_SURF0_PIXFMT = 0x5000; /* pixfmt: x8r8g8b8. if you try other modes they are pretty ugly */
    for(i=0; i<3; i++)
        for(j=0; j<3; j++)
            *(volatile uint32_t *)(DISP0_SURF0_CSCMATRIX + 4 * j + 12 * i) = (i + j == 2) ? 0x1000 : 0; /* matrix coeffs */
    *(volatile uint32_t *)DISP0_SURF0_CSCBYPASS &= ~0x11; /* enable matrix; both bits must be cleared */
}

static void byteswap32(uint32_t *buf, unsigned size)
{
    while(size --) {
        *buf = __builtin_bswap32(*buf);
        buf ++;
    }
}

void loader_main(void *linux_dtb, struct iphone_boot_args *bootargs, uint64_t smpentry, uint64_t rvbar)
{
    dtree *linux_dt;
    dt_node *node;
    dt_prop *prop;
    uint64_t memsize, base;
    unsigned i;

    memsize = (bootargs->mem_size + 0x3ffffffful) & ~0x3ffffffful;

    setarena(0x880000000ul, 0x100000);
    setvideo((void *)bootargs->video.phys, bootargs->video.width, bootargs->video.height, bootargs->video.stride);

    printf("Starting Linux loader stub.\n");

    linux_dt = dt_parse_dtb(linux_dtb, 0x20000);
    if(!linux_dt) {
        printf("Failed parsing Linux device-tree.\n");
        return;
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


    configure_x8b8g8r8();

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
