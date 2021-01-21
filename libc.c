/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#include "libc.h"

#define UART_BASE 0x235200000ul

size_t strlen(const char *s)
{
    size_t len = 0;
    while(*(s++))
        len ++;
    return len;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    uintptr_t sptr = (uintptr_t)src, dptr = (uintptr_t)dest, send = sptr + n - 1;

    if(((sptr ^ send) & ~7ul) && !((sptr ^ dptr) & 7ul)) {
        for(; sptr&7; sptr++, dptr++)
            *(uint8_t *)dptr = *(uint8_t *)sptr;
        for(; sptr+7<send; sptr+=8, dptr+=8)
            *(uint64_t *)dptr = *(uint64_t *)sptr;
    }
    for(; sptr<=send; sptr++, dptr++)
        *(uint8_t *)dptr = *(uint8_t *)sptr;
    return dest;
}

void *memchr(const void *s, int c, size_t n)
{
    const char *cs = s;
    while(n--) {
        if(*cs == c)
            return (void *)cs;
        cs ++;
    }
    return NULL;
}

void *memset(void *s, int c, size_t n)
{
    uint64_t c8;
    uintptr_t ptr = (uintptr_t)s, end = ptr + n - 1;

    if((ptr ^ end) & ~7ul) {
        for(; ptr&7; ptr++)
            *(uint8_t *)ptr = c;
        if(ptr + 7 < end) {
            c &= 0xFF;
            c8 = (uint32_t)(c | (c << 8) | (c << 16) | (c << 24));
            c8 |= c8 << 32;
            for(; ptr+7<end; ptr+=8)
                *(uint64_t *)ptr = c8;
        }
    }
    for(; ptr<=end; ptr++)
        *(uint8_t *)ptr = c;
    return s;
}

char *strchr(const char *s, int c)
{
    for(; *s; s++)
        if(*s == c)
            return (char *)s;
    return NULL;
}

char *strdup(const char *s)
{
    size_t len = strlen(s);
    char *res = malloc(len + 1);
    if(!res)
        return NULL;
    memcpy(res, s, len + 1);
    return res;
}

int strcmp(const char *s1, const char *s2)
{
    while(*s1 && *s2) {
        if(*s1 < *s2)
            return -1;
        if(*s1 > *s2)
            return 1;
        s1 ++;
        s2 ++;
    }
    if(*s1 < *s2)
        return -1;
    if(*s1 > *s2)
        return 1;
    return 0;
}

char *strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while(*src)
        *(dest++) = *(src++);
    *dest = 0;
    return ret;
}

int strncmp(const char *s1, const char *s2, size_t maxlen)
{
    while(*s1 && *s2 && maxlen) {
        if(*s1 < *s2)
            return -1;
        if(*s1 > *s2)
            return 1;
        s1 ++;
        s2 ++;
        maxlen --;
    }
    if(!maxlen)
        return 0;
    if(*s1 < *s2)
        return -1;
    if(*s1 > *s2)
        return 1;
    return 0;
}

void *calloc(size_t nmemb, size_t size)
{
    void *ptr;
    if(!nmemb || !size)
        nmemb = size = 1;
    size *= nmemb;
    ptr = malloc(size);
    if(ptr)
        memset(ptr, 0, size);
    return ptr;
}

struct range {
    uint64_t size;
    struct range *next, *prev;
};

static struct arena_s {
    uint64_t base, size;
    struct range head;
} arena;

void setarena(uint64_t base, uint64_t size)
{
    struct range *range;
    base += 56;
    size -= 64;
    arena.base = base;
    arena.size = size;
    range = (void *)base;
    range->size = size;
    range->next = &arena.head;
    range->prev = &arena.head;
    arena.head.prev = range;
    arena.head.next = range;
}

void *malloc(size_t size)
{
    struct range *range, *tail;

    if(!size)
        size = 1;

    size = (size + 8 + 63) & ~63;
    for(range=arena.head.next; range!=&arena.head; range=range->next)
        if(range->size >= size)
            break;
    if(range == &arena.head)
        return NULL;

    if(range->size - size >= 64) {
        tail = (void *)range + size;
        tail->size = range->size - size;
        tail->prev = range->prev;
        tail->next = range->next;
        tail->prev->next = tail;
        tail->next->prev = tail;
        range->size = size;
    } else {
        range->next->prev = range->prev;
        range->prev->next = range->next;
    }

    return (void *)range + 8;
}

static void free_merge(struct range *base)
{
    if(base == &arena.head || base->next == &arena.head)
        return;
    if((void*)base->next != (void *)base + base->size)
        return;
    base->size += base->next->size;
    base->next = base->next->next;
    base->next->prev = base;
}

void free(void *ptr)
{
    struct range *range, *prev;

    if(!ptr)
        return;
    range = (struct range *)(ptr - 8);
    for(prev=&arena.head; prev->next!=&arena.head; prev=prev->next)
        if(prev->next > range)
            break;
    range->next = prev->next;
    range->prev = prev;
    prev->next = range;
    range->next->prev = range;
    free_merge(range);
    free_merge(prev);
}

extern const unsigned char font8x16[];
static struct video_s {
    void *base;
    uint64_t width, height, stride;
    uint64_t xpos, ypos;
} video;

void setvideo(void *base, unsigned width, unsigned height, unsigned stride)
{
    if(width < 256 || height < 64) {
        video.base = NULL;
        return;
    }
    video.base = base;
    video.width = width;
    video.height = height;
    video.stride = stride;
    video.xpos = video.ypos = 0;
}

int puts(const char *s)
{
    while(*s)
        putchar(*(s++));
    putchar('\n');
    return 0;
}

int putchar(int c)
{
    volatile uint32_t *uart = (void *)UART_BASE;
    unsigned x, y, rb, print = 0, ch = c & 0xFF;
    const unsigned char *rp;
    uint32_t *wp;

    while(!(uart[4] & 4));
    uart[8] = ch;

    if(video.base) {
        if(ch == '\n') {
            video.ypos += 16;
        } else if(ch == '\r') {
            video.xpos = 0;
        } else if(ch == '\t') {
            video.xpos = (video.xpos + 63) & -64;
            if(video.xpos + 8 > video.width) {
                video.xpos = 0;
                video.ypos += 16;
            }
        } else {
            print = 1;
            if(video.xpos + 8 > video.width) {
                video.xpos = 0;
                video.ypos += 16;
            }
        }
        if(video.ypos + 16 > video.height) {
            memcpy(video.base, video.base + 16 * video.stride, (video.height / 16 - 1) * video.stride * 16);
            wp = video.base + (video.height / 16 - 1) * video.stride * 16;
            y = (16 * video.stride) / 4;
            for(x=0; x<y; x++)
                *(wp ++) = 0x0000001f;
            video.ypos -= 16;
        }
        if(print) {
            wp = video.base + video.xpos * 4 + video.ypos * video.stride;
            rp = font8x16 + 16 * ch;
            for(y=0; y<16; y++) {
                rb = *(rp ++);
                for(x=0; x<8; x++) {
                    *(wp ++) = (rb & 0x80) ? 0x3fffffff : 0x0000002f;
                    rb <<= 1;
                }
                wp += video.stride / 4 - 8;
            }
            video.xpos += 8;
        }
    }

    if(ch == '\n')
        putchar('\r');
    return c;
}

unsigned warning_count;
