/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#ifndef _LIBC_H
#define _LIBC_H

#include <stdint.h>
#include <stdarg.h>
#define NULL (0)

typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef int64_t ptrdiff_t;

size_t strlen(const char *s);
void *memcpy(void *dest, const void *src, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memset(void *s, int c, size_t n);
char *strchr(const char *s, int c);
char *strdup(const char *s);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
int strncmp(const char *s1, const char *s2, size_t maxlen);

void setarena(uint64_t base, uint64_t size);
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);

extern unsigned warning_count;
void setvideo(void *base, unsigned width, unsigned height, unsigned stride);
int putchar(int c);
int puts(const char *s);
int printf(const char *fmt, ...);

void udelay(unsigned long usec);
uint64_t tmr_now(void);

#define srread(reg,val) do { uint64_t temp; __asm__ volatile ("mrs %[data], " #reg : [data] "=r" (temp) : : "cc", "memory"); val = temp; } while(0)
#define srwrite(reg,val) do { uint64_t temp = val; __asm__ volatile ("msr " #reg ", %[data]" : : [data] "r" (temp) : "memory"); } while(0)

#endif
