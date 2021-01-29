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
#include "tunable.h"

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

static void configure_x8r8g8b8(void)
{
    *(volatile uint32_t *)DISP0_SURF0_PIXFMT = 0x5000; /* pixfmt: x8r8g8b8. if you try other modes they are pretty ugly */
}

static const struct tunable_fuse_map m1_pcie_fuse_map[] = {
    /* src_addr, dst_offs, src_[lsb,width], dst_[lsb,width] */
    { 0x23d2bc084, 0x6238,  4, 6,  0, 7 },
    { 0x23d2bc084, 0x6220, 10, 3, 14, 3 },
    { 0x23d2bc084, 0x62a4, 13, 2, 17, 2 },
    { 0x23d2bc418, 0x522c, 27, 2,  9, 2 },
    { 0x23d2bc418, 0x522c, 13, 3, 12, 3 },
    { 0x23d2bc418, 0x5220, 18, 3, 14, 3 },
    { 0x23d2bc418, 0x52a4, 21, 2, 17, 2 },
    { 0x23d2bc418, 0x522c, 23, 5, 16, 5 },
    { 0x23d2bc418, 0x5278, 23, 3, 20, 3 },
    { 0x23d2bc418, 0x5018, 31, 1,  2, 1 },
    { 0x23d2bc41c, 0x1204,  0, 5,  2, 5 },
    { 0 } };

void loader_main(void *linux_dtb, struct iphone_boot_args *bootargs, uint64_t smpentry, uint64_t rvbar)
{
    dtree *linux_dt, *apple_dt;
    dt_node *node;
    dt_prop *prop;
    uint64_t memsize, base, size;
    unsigned i;
    char *model;

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
            if(base >= 0x900000000ul) { /* high range */
                base = bootargs->phys_base + bootargs->mem_size;
                size = (0x800000000ul + memsize) - base;
            } else { /* low range */
                base = 0x800000000ul;
                size = bootargs->phys_base - base;
            }
            dt_put64be(prop->buf, base);
            dt_put64be(prop->buf + 8, size);
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

    model = NULL;
    if(apple_dt) {
        node = dt_find_node(apple_dt, "/");
        if(node) {
            prop = dt_find_prop(apple_dt, node, "target-type");
            if(prop)
                model = prop->buf;
        }
    }
    if(!model) {
        model = "unknown";
        warning_count ++;
    }

    printf("Running on '%s'...\n", model);

    prepare_tunable(apple_dt, "/arm-io/atc-phy0", "tunable_ATC0AXI2AF", linux_dt, "/soc/usb_drd0", "tunable-ATC0AXI2AF", TUNABLE_FANCY, 0x380000000);
    prepare_tunable(apple_dt, "/arm-io/usb-drd0", "tunable",            linux_dt, "/soc/usb_drd0", "tunable",            TUNABLE_LEGACY, 0);
    prepare_tunable(apple_dt, "/arm-io/atc-phy1", "tunable_ATC0AXI2AF", linux_dt, "/soc/usb_drd1", "tunable-ATC0AXI2AF", TUNABLE_FANCY, 0x500000000);
    prepare_tunable(apple_dt, "/arm-io/usb-drd1", "tunable",            linux_dt, "/soc/usb_drd1", "tunable",            TUNABLE_LEGACY, 0);

    prepare_tunable(apple_dt, "/arm-io/apcie",             "apcie-axi2af-tunables",        linux_dt, "/soc/pcie", "tunable-axi2af",            TUNABLE_PCIE, 4);
    prepare_tunable(apple_dt, "/arm-io/apcie",             "apcie-common-tunables",        linux_dt, "/soc/pcie", "tunable-common",            TUNABLE_PCIE, 1);
    prepare_tunable(apple_dt, "/arm-io/apcie",             "apcie-phy-ip-auspma-tunables", linux_dt, "/soc/pcie", "tunable-phy-ip-auspma",     TUNABLE_PCIE, 3);
    prepare_tunable(apple_dt, "/arm-io/apcie",             "apcie-phy-ip-pll-tunables",    linux_dt, "/soc/pcie", "tunable-phy-ip-pll",        TUNABLE_PCIE, 3);
    prepare_tunable(apple_dt, "/arm-io/apcie",             "apcie-phy-tunables",           linux_dt, "/soc/pcie", "tunable-phy",               TUNABLE_PCIE, 2);
    prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge0", "apcie-config-tunables",        linux_dt, "/soc/pcie", "tunable-port0-config",      TUNABLE_PCIE_PARENT, 6);
    prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge0", "pcie-rc-gen3-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port0-gen3-shadow", TUNABLE_PCIE_PARENT, 0);
    prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge0", "pcie-rc-gen4-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port0-gen4-shadow", TUNABLE_PCIE_PARENT, 0);
    prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge0", "pcie-rc-tunables",             linux_dt, "/soc/pcie", "tunable-port0",             TUNABLE_PCIE_PARENT, 0);
    if(!strcmp(model, "J274")) {
        /* Mac Mini has xHCI and Ethernet */
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge1", "apcie-config-tunables",        linux_dt, "/soc/pcie", "tunable-port1-config",      TUNABLE_PCIE_PARENT, 10);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge1", "pcie-rc-gen3-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port1-gen3-shadow", TUNABLE_PCIE_PARENT, 0 | 0x8000);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge1", "pcie-rc-gen4-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port1-gen4-shadow", TUNABLE_PCIE_PARENT, 0 | 0x8000);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge1", "pcie-rc-tunables",             linux_dt, "/soc/pcie", "tunable-port1",             TUNABLE_PCIE_PARENT, 0 | 0x8000);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge2", "apcie-config-tunables",        linux_dt, "/soc/pcie", "tunable-port2-config",      TUNABLE_PCIE_PARENT, 14);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge2", "pcie-rc-gen3-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port2-gen3-shadow", TUNABLE_PCIE_PARENT, 0 | 0x10000);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge2", "pcie-rc-gen4-shadow-tunables", linux_dt, "/soc/pcie", "tunable-port2-gen4-shadow", TUNABLE_PCIE_PARENT, 0 | 0x10000);
        prepare_tunable(apple_dt, "/arm-io/apcie/pci-bridge2", "pcie-rc-tunables",             linux_dt, "/soc/pcie", "tunable-port2",             TUNABLE_PCIE_PARENT, 0 | 0x10000);

        /* remove keyboard and touchbar */
        node = dt_find_node(linux_dt, "/soc/spi@235100000");
        if(node)
            dt_delete_node(node);
        node = dt_find_node(linux_dt, "/soc/spi@23510c000");
        if(node)
            dt_delete_node(node);
    } else {
        /* don't waste time bringing these up on Macbooks */
        node = dt_find_node(linux_dt, "/soc/pcie");
        if(node) {
            prop = dt_find_prop(linux_dt, node, "devpwr-on-1");
            if(prop)
                dt_delete_prop(prop);
            prop = dt_find_prop(linux_dt, node, "devpwr-on-2");
            if(prop)
                dt_delete_prop(prop);
        }
    }
    prepare_fuse_tunable(linux_dt, "/soc/pcie", "tunable-fuse", m1_pcie_fuse_map, 0x6800c0000);

    prepare_tunable(apple_dt, "/arm-io/pmgr", "voltage-states1",         linux_dt, "/soc/cpufreq", "tunable-ecpu-states",    TUNABLE_PLAIN, PLAIN_WORD);
    prepare_tunable(apple_dt, "/arm-io/pmgr", "voltage-states5",         linux_dt, "/soc/cpufreq", "tunable-pcpu-states",    TUNABLE_PLAIN, PLAIN_WORD);
    prepare_tunable(apple_dt, "/arm-io/pmgr", "mcx-fast-pcpu-frequency", linux_dt, "/soc/cpufreq", "tunable-pcpu-fast-freq", TUNABLE_PLAIN, PLAIN_WORD);
    prepare_tunable(apple_dt, "/arm-io/mcc",  "dramcfg-data",            linux_dt, "/soc/cpufreq", "tunable-pcpu-fast-dcfg", TUNABLE_PLAIN, PLAIN_WORD);

    configure_x8r8g8b8();

    if(warning_count) {
        printf("%d warnings; waiting to let you see them.\n", warning_count);
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
