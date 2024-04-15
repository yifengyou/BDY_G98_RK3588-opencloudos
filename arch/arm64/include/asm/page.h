/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/page.h
 *
 * Copyright (C) 1995-2003 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_PAGE_H
#define __ASM_PAGE_H

#include <asm/page-def.h>

#ifndef __ASSEMBLY__

#include <linux/personality.h> /* for READ_IMPLIES_EXEC */
#include <linux/types.h> /* for gfp_t */
#include <asm/pgtable-types.h>
#include <linux/preempt.h>
#include <asm/simd.h>

struct page;
struct vm_area_struct;

extern void slow_copy_page(void *to, const void *from);
#ifdef CONFIG_KERNEL_MODE_NEON
extern struct static_key_false fast_copy_page_enabled;
extern void fast_copy_page_switched(const void *from, void *to);
extern int fast_copy_page(void *to, const void *from);
extern void pagefault_disable_wrap(void);
extern void pagefault_enable_wrap(void);
static inline void copy_page(void *to, const void *from)
{
	long ret;

	if (!static_branch_unlikely(&fast_copy_page_enabled))
		return slow_copy_page(to, from);

	if (unlikely(!may_use_simd()))
		return slow_copy_page(to, from);

	pagefault_disable_wrap();
	ret = fast_copy_page(to, from);
	pagefault_enable_wrap();
	if (ret) {
		fast_copy_page_switched(from, to);
		slow_copy_page(to, from);
	}
}
#else
static inline void copy_page(void *to, const void *from)
{
	slow_copy_page(to, from);
}
#endif
extern void clear_page(void *to);

void copy_user_highpage(struct page *to, struct page *from,
			unsigned long vaddr, struct vm_area_struct *vma);
#define __HAVE_ARCH_COPY_USER_HIGHPAGE

void copy_highpage(struct page *to, struct page *from);
#define __HAVE_ARCH_COPY_HIGHPAGE

#ifdef CONFIG_ARCH_HAS_COPY_MC
int copy_mc_page(void *to, const void *from);
int copy_mc_highpage(struct page *to, struct page *from);
#define __HAVE_ARCH_COPY_MC_HIGHPAGE

int copy_mc_user_highpage(struct page *to, struct page *from,
		unsigned long vaddr, struct vm_area_struct *vma);
#define __HAVE_ARCH_COPY_MC_USER_HIGHPAGE
#endif

struct folio *vma_alloc_zeroed_movable_folio(struct vm_area_struct *vma,
						unsigned long vaddr);
#define vma_alloc_zeroed_movable_folio vma_alloc_zeroed_movable_folio

void tag_clear_highpage(struct page *to);
#define __HAVE_ARCH_TAG_CLEAR_HIGHPAGE

#define clear_user_page(page, vaddr, pg)	clear_page(page)
#define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)

typedef struct page *pgtable_t;

int pfn_is_map_memory(unsigned long pfn);

#include <asm/memory.h>

#endif /* !__ASSEMBLY__ */

#define VM_DATA_DEFAULT_FLAGS	(VM_DATA_FLAGS_TSK_EXEC | VM_MTE_ALLOWED)

#include <asm-generic/getorder.h>

#endif

