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
	stp     x29, x30, [sp, #-32]!
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
	ldr     q6, [x20, #16]
	cmp     x19, x0
	ldr     q5, [x20, #32]
	add     x20, x20, #0x80
	ldur    q4, [x20, #-80]
	ldur    q3, [x20, #-64]
	ldur    q2, [x20, #-48]
	ldur    q1, [x20, #-32]
	ldur    q0, [x20, #-16]
	stur    q7, [x19, #-128]
	stur    q6, [x19, #-112]
	stur    q5, [x19, #-96]
	stur    q4, [x19, #-80]
	stur    q3, [x19, #-64]
	stur    q2, [x19, #-48]
	stur    q1, [x19, #-32]
	stur    q0, [x19, #-16]
	b.ne    1b
	bl      kernel_neon_end
	ldp     x19, x20, [sp, #16]
	ldp     x29, x30, [sp], #32
	ret
SYM_FUNC_END(fast_copy_page)
EXPORT_SYMBOL(fast_copy_page)

