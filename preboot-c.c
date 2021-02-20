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

static const struct tunable_fuse_map m1_acio_fuse_map[] = {
    /* src_addr, dst_offs, src_[lsb,width], dst_[lsb,width] */
    { 0x23d2bc438, 0x2a38, 19, 6,  0, 7 },
    { 0x23d2bc438, 0x2a38, 25, 6, 17, 7 },
    { 0x23d2bc438, 0x2aa4, 31, 1, 17, 2 },
    { 0x23d2bc438, 0x0a04, 14, 5,  2, 5 },
    { 0x23d2bc43c, 0x2aa4,  0, 1, 17, 2 },
    { 0x23d2bc43c, 0x2a20,  1, 3, 14, 3 },
    { 0x23d2bc438, 0x222c,  7, 2,  9, 2 },
    { 0x23d2bc438, 0x222c,  4, 3, 12, 3 },
    { 0x23d2bc438, 0x22a4, 12, 2, 17, 2 },
    { 0x23d2bc438, 0x2220,  9, 3, 14, 3 },
    { 0x23d2bc438, 0x0a04, 14, 5,  2, 5 },
    { 0 } };

static void prepare_atc_tunables(dtree *apple_dt, dtree *linux_dt, int atc)
{
    char src_phy[24], src_usb[24], src_cio[16], dst_phy[16], dst_usb[16], dst_cio[12];
    char src_pxc[24], src_pxb[32], dst_pxc[12];
    uint64_t base = atc ? 0x500000000ul : 0x380000000ul;

    strcpy(src_phy, "/arm-io/atc-phy0"); src_phy[15] += atc;
    strcpy(src_usb, "/arm-io/usb-drd0"); src_usb[15] += atc;
    strcpy(src_cio, "/arm-io/acio0");    src_cio[12] += atc;
    strcpy(src_pxc, "/arm-io/apciec0");  src_pxc[14] += atc;
    strcpy(src_pxb, "/arm-io/apciec0/pcic0-bridge");  src_pxb[14] += atc; src_pxb[20] += atc;
    strcpy(dst_phy, "/soc/atcphy0");     dst_phy[11] += atc;
    strcpy(dst_usb, "/soc/usb_drd0");    dst_usb[12] += atc;
    strcpy(dst_cio, "/soc/acio0");       dst_cio[9]  += atc;
    strcpy(dst_pxc, "/soc/pciec0");      dst_pxc[10] += atc;

    prepare_tunable(apple_dt, src_phy, "tunable_ATC0AXI2AF",             linux_dt, dst_usb, "tunable-ATC0AXI2AF",            TUNABLE_FANCY, base);
    prepare_tunable(apple_dt, src_usb, "tunable",                        linux_dt, dst_usb, "tunable",                       TUNABLE_LEGACY, 0);

    prepare_tunable(apple_dt, src_phy, "tunable_ATC0AXI2AF",             linux_dt, dst_phy, "tunable-ATC0AXI2AF",            TUNABLE_FANCY, base);
    prepare_tunable(apple_dt, src_phy, "tunable_ATC_FABRIC",             linux_dt, dst_phy, "tunable-ATC_FABRIC",            TUNABLE_FANCY, base + 0x3045000);
    prepare_tunable(apple_dt, src_phy, "tunable_AUS_CMN_SHM",            linux_dt, dst_phy, "tunable-AUS_CMN_SHM",           TUNABLE_FANCY, base + 0x3000a00);
    prepare_tunable(apple_dt, src_phy, "tunable_AUS_CMN_TOP",            linux_dt, dst_phy, "tunable-AUS_CMN_TOP",           TUNABLE_FANCY, base + 0x3000800);
    prepare_tunable(apple_dt, src_phy, "tunable_AUSPLL_CORE",            linux_dt, dst_phy, "tunable-AUSPLL_CORE",           TUNABLE_FANCY, base + 0x3002200);
    prepare_tunable(apple_dt, src_phy, "tunable_AUSPLL_TOP",             linux_dt, dst_phy, "tunable-AUSPLL_TOP",            TUNABLE_FANCY, base + 0x3002000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO3PLL_CORE",           linux_dt, dst_phy, "tunable-CIO3PLL_CORE",          TUNABLE_FANCY, base + 0x3002a00);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO3PLL_TOP",            linux_dt, dst_phy, "tunable-CIO3PLL_TOP",           TUNABLE_FANCY, base + 0x3002800);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN0_AUSPMA_RX_EQ",   linux_dt, dst_phy, "tunable-CIO_LN0_AUSPMA_RX_EQ",  TUNABLE_FANCY, base + 0x300a000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN0_AUSPMA_RX_EQ",   linux_dt, dst_phy, "tunable-USB_LN0_AUSPMA_RX_EQ",  TUNABLE_FANCY, base + 0x300a000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN0_AUSPMA_RX_SHM",  linux_dt, dst_phy, "tunable-CIO_LN0_AUSPMA_RX_SHM", TUNABLE_FANCY, base + 0x300b000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN0_AUSPMA_RX_SHM",  linux_dt, dst_phy, "tunable-USB_LN0_AUSPMA_RX_SHM", TUNABLE_FANCY, base + 0x300b000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN0_AUSPMA_RX_TOP",  linux_dt, dst_phy, "tunable-CIO_LN0_AUSPMA_RX_TOP", TUNABLE_FANCY, base + 0x3009000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN0_AUSPMA_RX_TOP",  linux_dt, dst_phy, "tunable-USB_LN0_AUSPMA_RX_TOP", TUNABLE_FANCY, base + 0x3009000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN0_AUSPMA_TX_TOP",  linux_dt, dst_phy, "tunable-CIO_LN0_AUSPMA_TX_TOP", TUNABLE_FANCY, base + 0x300c000);
    prepare_tunable(apple_dt, src_phy, "tunable_DP_LN0_AUSPMA_TX_TOP",   linux_dt, dst_phy, "tunable-DP_LN0_AUSPMA_TX_TOP",  TUNABLE_FANCY, base + 0x300c000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN0_AUSPMA_TX_TOP",  linux_dt, dst_phy, "tunable-USB_LN0_AUSPMA_TX_TOP", TUNABLE_FANCY, base + 0x300c000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN1_AUSPMA_RX_EQ",   linux_dt, dst_phy, "tunable-CIO_LN1_AUSPMA_RX_EQ",  TUNABLE_FANCY, base + 0x3011000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN1_AUSPMA_RX_EQ",   linux_dt, dst_phy, "tunable-USB_LN1_AUSPMA_RX_EQ",  TUNABLE_FANCY, base + 0x3011000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN1_AUSPMA_RX_SHM",  linux_dt, dst_phy, "tunable-CIO_LN1_AUSPMA_RX_SHM", TUNABLE_FANCY, base + 0x3012000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN1_AUSPMA_RX_SHM",  linux_dt, dst_phy, "tunable-USB_LN1_AUSPMA_RX_SHM", TUNABLE_FANCY, base + 0x3012000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN1_AUSPMA_RX_TOP",  linux_dt, dst_phy, "tunable-CIO_LN1_AUSPMA_RX_TOP", TUNABLE_FANCY, base + 0x3010000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN1_AUSPMA_RX_TOP",  linux_dt, dst_phy, "tunable-USB_LN1_AUSPMA_RX_TOP", TUNABLE_FANCY, base + 0x3010000);
    prepare_tunable(apple_dt, src_phy, "tunable_CIO_LN1_AUSPMA_TX_TOP",  linux_dt, dst_phy, "tunable-CIO_LN1_AUSPMA_TX_TOP", TUNABLE_FANCY, base + 0x3013000);
    prepare_tunable(apple_dt, src_phy, "tunable_DP_LN1_AUSPMA_TX_TOP",   linux_dt, dst_phy, "tunable-DP_LN1_AUSPMA_TX_TOP",  TUNABLE_FANCY, base + 0x3013000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_LN1_AUSPMA_TX_TOP",  linux_dt, dst_phy, "tunable-USB_LN1_AUSPMA_TX_TOP", TUNABLE_FANCY, base + 0x3013000);
    prepare_tunable(apple_dt, src_phy, "tunable_USB_ACIOPHY_TOP",        linux_dt, dst_phy, "tunable-USB_ACIOPHY_TOP",       TUNABLE_FANCY, base + 0x3000000);

    prepare_fuse_tunable(linux_dt, dst_phy, "tunable-fuse", m1_acio_fuse_map, base + 0x3000000);

    prepare_tunable(apple_dt, src_cio, "fw_int_ctl_management_tunables", linux_dt, dst_cio, "tunable-fw_int_ctl_management", TUNABLE_PCIE, 0 | 0x04000);
    prepare_tunable(apple_dt, src_cio, "hbw_fabric_tunables",            linux_dt, dst_cio, "tunable-hbw_fabric",            TUNABLE_PCIE, 3);
    prepare_tunable(apple_dt, src_cio, "hi_dn_merge_fabric_tunables",    linux_dt, dst_cio, "tunable-hi_dn_merge_fabric",    TUNABLE_PCIE, 0 | 0xfc000);
    prepare_tunable(apple_dt, src_cio, "hi_up_merge_fabric_tunables",    linux_dt, dst_cio, "tunable-hi_up_merge_fabric",    TUNABLE_PCIE, 0 | 0xf8000);
    prepare_tunable(apple_dt, src_cio, "hi_up_tx_desc_fabric_tunables",  linux_dt, dst_cio, "tunable-hi_up_tx_desc_fabric",  TUNABLE_PCIE, 0 | 0xf0000);
    prepare_tunable(apple_dt, src_cio, "hi_up_tx_data_fabric_tunables",  linux_dt, dst_cio, "tunable-hi_up_tx_data_fabric",  TUNABLE_PCIE, 0 | 0xec000);
    prepare_tunable(apple_dt, src_cio, "hi_up_rx_desc_fabric_tunables",  linux_dt, dst_cio, "tunable-hi_up_rx_desc_fabric",  TUNABLE_PCIE, 0 | 0xe8000);
    prepare_tunable(apple_dt, src_cio, "hi_up_wr_fabric_tunables",       linux_dt, dst_cio, "tunable-hi_up_wr_fabric",       TUNABLE_PCIE, 0 | 0xf4000);
    prepare_tunable(apple_dt, src_cio, "lbw_fabric_tunables",            linux_dt, dst_cio, "tunable-lbw_fabric",            TUNABLE_PCIE, 4);
    prepare_tunable(apple_dt, src_cio, "pcie_adapter_regs_tunables",     linux_dt, dst_cio, "tunable-pcie_adapter_regs",     TUNABLE_PCIE, 5);
    prepare_tunable(apple_dt, src_cio, "top_tunables",                   linux_dt, dst_cio, "tunable-top",                   TUNABLE_PCIE, 2);

    prepare_tunable(apple_dt, src_cio, "thunderbolt-drom",               linux_dt, dst_cio, "thunderbolt-drom",              TUNABLE_PLAIN, PLAIN_BYTE);

    prepare_tunable(apple_dt, src_pxc, "atc-apcie-debug-tunables",       linux_dt, dst_pxc, "tunable-debug",                 TUNABLE_PCIE, 6);
    prepare_tunable(apple_dt, src_pxc, "atc-apcie-fabric-tunables",      linux_dt, dst_pxc, "tunable-fabric",                TUNABLE_PCIE, 4);
    prepare_tunable(apple_dt, src_pxc, "atc-apcie-oe-fabric-tunables",   linux_dt, dst_pxc, "tunable-oe-fabric",             TUNABLE_PCIE, 5);
    prepare_tunable(apple_dt, src_pxc, "atc-apcie-rc-tunables",          linux_dt, dst_pxc, "tunable-rc",                    TUNABLE_PCIE, 0);
    prepare_tunable(apple_dt, src_pxb, "apcie-config-tunables",          linux_dt, dst_pxc, "tunable-port0-config",          TUNABLE_PCIE_PARENT, 3);
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

    setarena(0x880000000ul, 0x400000);
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

    prepare_atc_tunables(apple_dt, linux_dt, 0);
    prepare_atc_tunables(apple_dt, linux_dt, 1);

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

        prepare_tunable(apple_dt, "/chosen", "mac-address-ethernet0", linux_dt, "/chosen", "hwaddr-eth0", TUNABLE_PLAIN, PLAIN_BYTE);

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

    prepare_tunable(apple_dt, "/chosen", "mac-address-wifi0",      linux_dt, "/chosen", "hwaddr-wlan0", TUNABLE_PLAIN, PLAIN_BYTE);
    prepare_tunable(apple_dt, "/chosen", "mac-address-bluetooth0", linux_dt, "/chosen", "hwaddr-bt0",   TUNABLE_PLAIN, PLAIN_BYTE);
    prepare_tunable(apple_dt, "/arm-io/wlan", "module-instance",   linux_dt, "/chosen", "module-wlan0", TUNABLE_PLAIN, PLAIN_BYTE);

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
