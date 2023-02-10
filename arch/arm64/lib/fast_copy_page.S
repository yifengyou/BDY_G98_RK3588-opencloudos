/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2023 Tencent Holding Limited.
 */

#include <linux/linkage.h>
#include <linux/const.h>
#include <asm/assembler.h>
#include <asm/page.h>
#include <asm/cpufeature.h>
#include <asm/alternative.h>

/*
 * Fast copy a page from src to dest (both are page aligned)
 *
 * Parameters:
 *	x0 - dest
 *	x1 - src
 */
SYM_FUNC_START(fast_copy_page)
	stp     x29, x30, [sp, #-64]!
	mov     x29, sp
	stp     x19, x20, [sp, #16]
	mov     x19, x0
	mov     x0, x30
	mov     x20, x1
	nop
	bl      kernel_neon_begin
	add     x0, x19, #(PAGE_SIZE >> 12), lsl #12
1:
	ldr     q7, [x20]
	add     x19, x19, #0x80
2:
	ldr     q6, [x20, #16]
	cmp     x19, x0
3:
	ldr     q5, [x20, #32]
	add     x20, x20, #0x80
4:
	ldur    q4, [x20, #-80]
5:
	ldur    q3, [x20, #-64]
6:
	ldur    q2, [x20, #-48]
7:
	ldur    q1, [x20, #-32]
8:
	ldur    q0, [x20, #-16]
9:
	stur    q7, [x19, #-128]
10:
	stur    q6, [x19, #-112]
11:
	stur    q5, [x19, #-96]
12:
	stur    q4, [x19, #-80]
13:
	stur    q3, [x19, #-64]
14:
	stur    q2, [x19, #-48]
15:
	stur    q1, [x19, #-32]
16:
	stur    q0, [x19, #-16]
	b.ne    1b
	mov	x0, #0
20:
	str	x0, [sp, #32]
	bl      kernel_neon_end
	ldr	x0,  [sp, #32]
	ldp     x19, x20, [sp, #16]
	ldp     x29, x30, [sp], #64
	ret

	.section .fixup, "ax"
	.align  2
21:
	mov x0, #-1
	b 20b
	.previous
.pushsection	__ex_table, "a"
.align		3
.long		(1b - .), (21b - .)
.long		(2b - .), (21b - .)
.long		(3b - .), (21b - .)
.long		(4b - .), (21b - .)
.long		(5b - .), (21b - .)
.long		(6b - .), (21b - .)
.long		(7b - .), (21b - .)
.long		(8b - .), (21b - .)
.long		(9b - .), (21b - .)
.long		(10b - .), (21b - .)
.long		(11b - .), (21b - .)
.long		(12b - .), (21b - .)
.long		(13b - .), (21b - .)
.long		(14b - .), (21b - .)
.long		(15b - .), (21b - .)
.long		(16b - .), (21b - .)
.popsection
SYM_FUNC_END(fast_copy_page)
EXPORT_SYMBOL(fast_copy_page)
