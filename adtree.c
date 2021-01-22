/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2021 Corellium LLC
 * All rights reserved.
 *
 */

#include "libc.h"
#include "dtree.h"

typedef struct {
    char name[32];
    uint32_t len_flags;
    uint8_t value[0];
} __attribute__((packed)) adt_property;

static unsigned p_len(adt_property *p)
{
    return p->len_flags & 0x00FFFFFFul;
}

static unsigned p_flags(adt_property *p)
{
    return (p->len_flags & 0xFF000000ul) >> 16;
}

typedef struct {
    uint32_t nprops;
    uint32_t nnodes;
    adt_property props[0];
} __attribute__((packed)) adt_node;

static int is_nullterm(char *str, unsigned len)
{
    if(!len)
        return 0;
    if(!str[len-1])
        return 1;
    while(len--)
        if(!*(str++))
            return 1;
    return 0;
}

static int adt_parse_int(void *_adtb, int remain, dtree *tree, dt_node *parent, dt_node ***parlastp)
{
    dt_node *node, **childp;
    dt_prop *prop, **nextp;
    adt_node *adtb = _adtb, *nodes;
    adt_property *props;
    int i, delta = 0, pdelta, len, iname = -1;
    char nbuf[33];

    if(remain < sizeof(adt_node))
        return -1;
    delta += sizeof(adt_node);

    node = calloc(sizeof(dt_node), 1);
    if(!node)
        return -1;
    node->parent = parent;
    **parlastp = node;
    *parlastp = &node->next;
    nextp = &node->prop;

    nbuf[32] = 0;

    props = adtb->props;
    pdelta = delta;
    for(i=0; i<adtb->nprops; i++) {
        if(remain - pdelta < sizeof(adt_property))
            return -1;
        if(remain - pdelta < sizeof(adt_property) + ((p_len(props) + 3) & ~3))
            return -1;
        memcpy(nbuf, props->name, 32);
        if(!strcmp(nbuf, "name") && !p_flags(props))
            break;
        props = (void *)(((uint8_t *)props) + sizeof(adt_property) + ((p_len(props) + 3) & ~3));
        pdelta += sizeof(adt_property) + ((p_len(props) + 3) & ~3);
    }
    if(i < adtb->nprops) {
        iname = i;
        if(!is_nullterm((char *)props->value, p_len(props)))
            return -1;
        node->name = strdup((char *)props->value);
    } else
        node->name = strdup("*invalid*");

    props = adtb->props;
    for(i=0; i<adtb->nprops; i++) {
        memcpy(nbuf, props->name, 32);

        if(i != iname) {
            prop = calloc(sizeof(dt_prop), 1);
            prop->parent = node;
            if(!prop)
                return -1;
            prop->name = dt_dict_find(tree->names, nbuf, DT_DICT_ANY);
            if(!prop->name) {
                free(prop);
                return -1;
            }
            prop->buf = calloc(p_len(props) + 1, 1);
            if(!prop->buf) {
                free(prop);
                return -1;
            }
            memcpy(prop->buf, props->value, p_len(props));
            prop->size = p_len(props);
            *nextp = prop;
            nextp = &prop->next;
        }
        delta += sizeof(adt_property) + ((p_len(props) + 3) & ~3);
        props = (void *)(((uint8_t *)props) + sizeof(adt_property) + ((p_len(props) + 3) & ~3));
    }

    nodes = (void *)props;
    childp = &node->child;
    for(i=0; i<adtb->nnodes; i++) {
        len = adt_parse_int(nodes, remain - delta, tree, node, &childp);
        if(len < 0)
            return -1;
        nodes = (void *)((uint8_t *)nodes + len);
        delta += len;
    }

    return delta;
}

dtree *adt_parse(void *adtb, unsigned adtlen)
{
    dtree *tree;
    dt_node **rootp;

    tree = dt_new();
    if(!tree)
        return NULL;

    rootp = &tree->root;
    if(adt_parse_int(adtb, adtlen, tree, NULL, &rootp) < 0) {
        dt_free(tree);
        return NULL;
    }

    return tree;
}
