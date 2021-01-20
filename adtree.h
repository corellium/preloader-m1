/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#ifndef _ADTREE_H
#define _ADTREE_H

#include "dtree.h"

dtree *adt_parse(void *adtb, unsigned adtlen);

#endif
