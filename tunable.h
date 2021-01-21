/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#ifndef _TUNABLE_H
#define _TUNABLE_H

#include "libc.h"
#include "dtree.h"

#define TUNABLE_LEGACY          0
#define TUNABLE_FANCY           1
#define TUNABLE_PCIE            2
#define TUNABLE_PCIE_PARENT     3

void prepare_tunable(dtree *adt, const char *adtnname, const char *adtpname, dtree *ldt, const char *ldtnname, const char *ldtpname, unsigned mode, uint64_t base);

struct tunable_fuse_map {
    uint64_t src_addr;
    uint32_t dst_offs;
    uint8_t src_lsb, src_width;
    uint8_t dst_lsb, dst_width;
};
void prepare_fuse_tunable(dtree *ldt, const char *ldtnname, const char *ldtpname, const struct tunable_fuse_map *map, uint64_t base);

#endif
