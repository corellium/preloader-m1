/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 *
 * Copyright (C) 2017-21 Corellium LLC
 * All rights reserved.
 *
 */

#define SMP_DEBUG 0

.section ".start", "ax"

.global smpentry
smpentry:
    b smpstart
    .asciz "PREBOOT"

    .align 7
rvbar:
    .long 0xDEADB070, 0xDEADB070, 0xDEADB070, 0xDEADB070
    .long 0xDEADB070, 0xDEADB070, 0xDEADB070, 0xDEADB070
    .long 0xDEADB070, 0xDEADB070, 0xDEADB070, 0xDEADB070
    .long 0xDEADB070, 0xDEADB070, 0xDEADB070, 0xDEADB070

    .align 8
.global entry
entry:
    b start

smpstart:
    dsb sy

#if SMP_DEBUG
    mrs x1, mpidr_el1
    lsr x0, x1, #14
    bfxil x0, x1, #0, #2
    and x0, x0, #7
    mov x1, #0x870
    lsl x1, x1, #24
    sub x1, x1, x0, lsl #18
    mov sp, x1
    bl smp_main
#endif

    mrs x1, mpidr_el1
    lsr x0, x1, #14
    bfxil x0, x1, #0, #2
    and x0, x0, #7
    adr x1, rvbar
2:  dsb sy
    ldr x2, [x1, x0, lsl #3]
    tbz x2, #0, 1f
    and x2, x2, #0xFFFFFFFFFC

    bl cpuinit

    blr x2
3:  wfe
    b 3b
1:  wfe
    b 2b

#define SR_H13_MIGSTS		s3_4_c15_c0_4
#define SR_H13_HID0		s3_0_c15_c0_0
#define SR_H13_HID1		s3_0_c15_c1_0
#define SR_H13_HID3		s3_0_c15_c3_0
#define SR_H13_HID4		s3_0_c15_c4_0
#define SR_H13_EHID4		s3_0_c15_c4_1
#define SR_H13_HID5		s3_0_c15_c5_0
#define SR_H13_HID6		s3_0_c15_c6_0
#define SR_H13_HID7		s3_0_c15_c7_0
#define SR_H13_HID9		s3_0_c15_c9_0
#define SR_H13_EHID10		s3_0_c15_c10_1
#define SR_H13_HID11		s3_0_c15_c11_0
#define SR_H13_CYC_CFG		s3_5_c15_c4_0
#define SR_H13_CYC_OVRD		s3_5_c15_c5_0
#define SR_H13_LLC_ERR_STS	s3_3_c15_c8_0
#define SR_H13_LLC_ERR_ADR	s3_3_c15_c9_0
#define SR_H13_LLC_ERR_INF	s3_3_c15_c10_0
#define SR_H13_LSU_ERR_STS	s3_3_c15_c2_0
#define SR_H13_LSU_ERR_STS_P	s3_3_c15_c0_0
#define SR_H13_FED_ERR_STS	s3_4_c15_c0_2
#define SR_H13_FED_ERR_STS_P	s3_4_c15_c0_0
#define SR_H13_MMU_ERR_STS	s3_6_c15_c2_0
#define SR_H13_MMU_ERR_STS_P	s3_6_c15_c0_0
#define SR_H13_DPC_ERR_STS	s3_5_c15_c0_5
#define SR_H13_KTRR_LOCK	s3_4_c15_c2_2
#define SR_H13_KTRR_MODE	s3_4_c15_c2_5
#define SR_H13_KTRR_LOWER	s3_4_c15_c2_3
#define SR_H13_KTRR_UPPER	s3_4_c15_c2_4

cpuinit:
    msr oslar_el1, xzr

    mrs x18, midr_el1
    and x18, x18, #0xfff0
    cmp x18, #0x0220
    beq cpuinit_h13e
    cmp x18, #0x0230
    beq cpuinit_h13p

    ret

cpuinit_h13e:
    msr oslar_el1, xzr
    mov x0, #1
    msr s3_6_c15_c1_0, x0
    tlbi vmalle1
    ldr x0, =0x2020a505f020f0f0
    msr s3_6_c15_c1_6, x0
    msr s3_6_c15_c1_0, xzr
    tlbi vmalle1
1:  mrs x0, s3_6_c15_c12_4
    tbz x0, #0, 1b
    mrs x0, SR_H13_MIGSTS
    bic x0, x0, #6
    orr x0, x0, #0x10
    orr x0, x0, #0x1
    msr SR_H13_MIGSTS, x0
    mrs x0, SR_H13_MIGSTS
    tbnz x0, #4, 1f
    orr x0, x0, #2
    msr SR_H13_MIGSTS, x0
1:  mrs x0, SR_H13_EHID4
    orr x0, x0, #0x800
    orr x0, x0, #0x100000000000
    msr SR_H13_EHID4, x0
    mrs x0, SR_H13_HID5
    orr x0, x0, #0x2000000000000000
    msr SR_H13_HID5, x0
    mrs x0, SR_H13_EHID10
    orr x0, x0, #0x100000000
    orr x0, x0, #0x2000000000000
    msr SR_H13_EHID10, x0
    mrs x0, s3_0_c15_c1_2
    orr x0, x0, #0x100
    msr s3_0_c15_c1_2, x0
    mrs x0, s3_0_c15_c9_1
    bic x0, x0, #0x20
    msr s3_0_c15_c9_1, x0
    mrs x0, s3_0_c15_c1_2
    orr x0, x0, #0x8000
    msr s3_0_c15_c1_2, x0
    mrs x0, s3_0_c15_c1_2
    orr x0, x0, #0x10000
    msr s3_0_c15_c1_2, x0
    mrs x0, s3_0_c15_c1_2
    orr x0, x0, #0x600000
    msr s3_0_c15_c1_2, x0
    mrs x0, mpidr_el1
    and x0, x0, #3
    msr s3_4_c15_c5_0, x0
    mov x0, #0x100
    msr s3_4_c15_c1_4, x0
    mrs x0, SR_H13_CYC_OVRD
    bic x0, x0, #0xf00000
    msr SR_H13_CYC_OVRD, x0
    mrs x0, actlr_el1
    orr x0, x0, #0x200 /* something to do with dsb? */
    msr actlr_el1, x0
    mrs x0, SR_H13_CYC_CFG
    orr x0, x0, #12
    msr SR_H13_CYC_CFG, x0
    msr SR_H13_LLC_ERR_STS, xzr

    ret

cpuinit_h13p:
    msr oslar_el1, xzr
    mov x0, #1
    msr s3_6_c15_c1_0, x0
    tlbi vmalle1
    ldr x0, =0x2020a505f020f0f0
    msr s3_6_c15_c1_6, x0
    msr s3_6_c15_c1_0, xzr
    tlbi vmalle1
    mrs x0, s3_0_c15_c14_0
    bic x0, x0, 0xf000000000000000
    orr x0, x0, 0xc000000000000000
    msr s3_0_c15_c14_0, x0
    mrs x0, s3_0_c15_c15_0
    orr x0, x0, 0x100000000
    msr s3_0_c15_c15_0, x0
1:  mrs x0, s3_6_c15_c12_4
    tbz x0, #0, 1b
    mrs x0, SR_H13_MIGSTS
    bic x0, x0, #6
    orr x0, x0, #0x10
    orr x0, x0, #0x1
    msr SR_H13_MIGSTS, x0
    mrs x0, SR_H13_MIGSTS
    tbnz x0, #4, 1f
    orr x0, x0, #2
    msr SR_H13_MIGSTS, x0
1:  mrs x0, SR_H13_HID4
    orr x0, x0, #0x800
    orr x0, x0, #0x100000000000
    msr SR_H13_HID4, x0
    mrs x0, SR_H13_HID5
    orr x0, x0, #0x2000000000000000
    msr SR_H13_HID5, x0
    mrs x0, s3_0_c15_c14_0
    bic x0, x0, #0x3c000
    orr x0, x0, #0x10000
    msr s3_0_c15_c14_0, x0
    mrs x0, SR_H13_HID0
    orr x0, x0, #0x200000000000
    msr SR_H13_HID0, x0
    mrs x0, SR_H13_HID3
    bic x0, x0, #0x8000000000000000
    bic x0, x0, #0x100000000000
    msr SR_H13_HID3, x0
    mrs x0, SR_H13_HID1
    orr x0, x0, #0x40000000000000
    msr SR_H13_HID1, x0
    mrs x0, s3_0_c15_c15_2
    orr x0, x0, #0x100000000000000
    orr x0, x0, #0x800000000000000
    orr x0, x0, #0x2000000000000000
    orr x0, x0, #0x4000000000000000
    msr s3_0_c15_c15_2, x0
    mrs x0, SR_H13_HID9
    orr x0, x0, #0x4000000
    msr SR_H13_HID9, x0
    mrs x0, SR_H13_HID4
    orr x0, x0, #0x30000000000
    msr SR_H13_HID4, x0
    mrs x0, SR_H13_HID11
    orr x0, x0, #0x800000000000000
    msr SR_H13_HID11, x0
    mrs x0, SR_H13_HID0
    orr x0, x0, #0x10000000
    orr x0, x0, #0x1000000000
    msr SR_H13_HID0, x0
    mrs x0, SR_H13_HID6
    bic x0, x0, #0x3e0
    msr SR_H13_HID6, x0
    mrs x0, SR_H13_HID7
    orr x0, x0, #0x100000
    orr x0, x0, #0x80000
    orr x0, x0, #0x3000000
    msr SR_H13_HID7, x0
    mrs x0, SR_H13_HID9
    orr x0, x0, #0x1000000000000
    orr x0, x0, #0x20000000
    msr SR_H13_HID9, x0
    mrs x0, s3_0_c15_c11_2
    orr x0, x0, #0x4000
    msr s3_0_c15_c11_2, x0
    mrs x0, s3_0_c15_c1_3
    bic x0, x0, #0x80000
    msr s3_0_c15_c1_3, x0
    mrs x0, SR_H13_HID4
    orr x0, x0, #0x2000000000000
    orr x0, x0, #0x20000000000000
    msr SR_H13_HID4, x0
    mrs x0, SR_H13_HID9
    orr x0, x0, #0x80000000000000
    msr SR_H13_HID9, x0
    mrs x0, SR_H13_HID11
    orr x0, x0, #0x8000
    msr SR_H13_HID11, x0
    mrs x0, SR_H13_HID1
    orr x0, x0, #0x400000000000000
    orr x0, x0, #0x1000000000000000
    msr SR_H13_HID1, x0
    mrs x0, s3_0_c15_c1_3
    orr x0, x0, #0x2000000000000
    msr s3_0_c15_c1_3, x0
    mrs x0, mpidr_el1
    and x0, x0, #3
    msr s3_4_c15_c5_0, x0
    mov x0, #0x100
    msr s3_4_c15_c1_4, x0
    mrs x0, SR_H13_CYC_OVRD
    bic x0, x0, #0xf00000
    msr SR_H13_CYC_OVRD, x0
    mrs x0, actlr_el1
    orr x0, x0, #0x200 /* something to do with dsb? */
    msr actlr_el1, x0
    mrs x0, SR_H13_CYC_CFG
    orr x0, x0, #12
    msr SR_H13_CYC_CFG, x0
    msr SR_H13_LLC_ERR_STS, xzr

    ret

start:
    mov x24, x0

    mov x0, #0
    mov x1, #0
    ldr x2, [x24, #0x40]
    ldr x3, [x24, #0x48]
    mov w4, #0x0000001f
    bl fill_rect

    mov x1, #0x35200000
    orr x1, x1, #0x200000000
    mov x0, #3
    str w0, [x1]
    mov x0, #5
    str w0, [x1, #4]
    str wzr, [x1, #0xc]
    mov x0, #0xc
    str w0, [x1, #28]
    mov x0, #0x3
    str w0, [x1, #8]
    mov x0, #0x1
    str w0, [x1, #0xc]

    mov x0, #0x3d2b0000
    orr x0, x0, #0x200000000
    str wzr, [x0, #0x1c]

    mov x0, #0x800000000
    orr x0, x0, #0x80000000
    sub x0, x0, #16
    mov sp, x0
    adr x0, smpentry
    add x0, x0, #0x20000
    mov x1, x24
    adr x2, smpentry
    adr x3, rvbar
    bl loader_main

    adr x0, smpentry
    mov x1, #0x800000000
    orr x1, x1, #0x80000000
    add x1, x1, #0x40000
    add x2, x0, #0x40000
    ldr x2, [x2, #16]
    add x2, x2, #0x40000
    add x2, x2, #15
    bic x2, x2, #15
1:  ldp x8, x9, [x0], #16
    stp x8, x9, [x1], #16
    sub x2, x2, #16
    cbnz x2, 1b
    isb

    adr x0, smpentry
    adr x1, 1f
    sub x1, x1, x0
    mov x0, #0x800000000
    orr x0, x0, #0x80000000
    add x0, x0, #0x40000
    add x1, x1, x0
    br x1
1:  isb

    bl cpuinit

    mov x0, #0x800000000
    orr x0, x0, #0x80000000
    add x0, x0, #0x60000
    mov x1, #0
    mov x2, #0
    mov x3, #0
    add x18, x0, #0x20000
    br x18

uart_putchar:
    ldr w16, [x1, #0x10]
    tbnz w16, #2, 1f
    b uart_putchar
1:  str w0, [x1, #0x20]
    ret

fill_rect:
    ldr x8, [x24, #0x28]
    ldr x9, [x24, #0x38]
    mov w12, w4
    mul x10, x1, x9
    add x8, x8, x0, lsl #2
    add x8, x8, x10
    mov x10, x3
1:  mov x11, x2
2:  str w12, [x8], #4
    sub x11, x11, #1
    cbnz x11, 2b
    add x8, x8, x9
    sub x8, x8, x2, lsl #2
    sub x10, x10, #1
    cbnz x10, 1b
    ret

.align 4
.pool

.section ".text", "ax"

.global udelay
udelay:
    mov x1, #24
    mul x0, x0, x1
    mrs x1, cntvct_el0
    add x1, x1, x0
1:  mrs x0, cntvct_el0
    cmp x0, x1
    ble 1b
    ret

.global tmr_now
tmr_now:
    mrs x0, cntvct_el0
    ret
