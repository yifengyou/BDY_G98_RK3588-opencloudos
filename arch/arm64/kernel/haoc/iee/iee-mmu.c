// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/mutex.h>
#include <linux/hugetlb.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-asm.h>
#include <asm/haoc/iee-init.h>
#ifdef CONFIG_PTP
#include <asm/haoc/iee-ptp-init.h>
#endif

#define IEE_EARLY_BLOCK_NR	64

struct iee_block {
	phys_addr_t start;
	unsigned int order;
};

struct iee_early_alloc {
	phys_addr_t begin;
	phys_addr_t end;
	int pos;
	struct iee_block blocks[IEE_EARLY_BLOCK_NR];
	int curr_block_nr;
	char *name;
};

static struct iee_early_alloc iee_data = {
	.name = "iee_early_data",
	.curr_block_nr = -1
};

static struct iee_early_alloc iee_stack = {
	.name = "iee_stack",
	.curr_block_nr = -1
};

#ifdef CONFIG_PTP
static struct iee_early_alloc iee_pgtable = {
	.name = "iee_early_pgtable",
	.curr_block_nr = -1
};
#endif

DEFINE_SPINLOCK(swapper_pgdir_lock);
DEFINE_MUTEX(fixmap_lock);

__aligned(PAGE_SIZE) DECLARE_PER_CPU(u64*[(PAGE_SIZE/8)],
				iee_cpu_stack_ptr);

/* reserve 8 pages for iee init stack. */
__aligned(IEE_STACK_SIZE) __initdata u64 iee_init_stack[IEE_STACK_SIZE/8];

/* Setup global values used in verifications of TCR_EL1 to protect IEE switch gate.
 * Use fixmap functions as these globals are put inside IEE text section.
 */
#ifdef CONFIG_PTP
void __init iee_init_tcr_ptp(void)
{
	unsigned long ptr = pte_set_fixmap_pre_init(__pa_symbol(&kernel_tcr));
	*((u64 *)ptr) = read_sysreg(tcr_el1) & IEE_TCR_MASK & ~(TCR_HPD1 | TCR_A1);
	pte_clear_fixmap_pre_init();
	ptr = pte_set_fixmap_pre_init(__pa_symbol(&iee_tcr));
	*((u64 *)ptr) = kernel_tcr | TCR_HPD1 | TCR_A1;
	pte_clear_fixmap_pre_init();
}
#else
void __init iee_init_tcr(void)
{
	unsigned long ptr = (unsigned long)(fix_to_virt(FIX_PTE));

	__set_fixmap(FIX_PTE, __pa_symbol(&kernel_tcr), FIXMAP_PAGE_NORMAL);
	ptr += (unsigned long)(&kernel_tcr) & (PAGE_SIZE - 1);
	*((u64 *)ptr) = read_sysreg(tcr_el1) & IEE_TCR_MASK & ~(TCR_HPD1 | TCR_A1);
	clear_fixmap(FIX_PTE);
	ptr = (unsigned long)(fix_to_virt(FIX_PTE));
	__set_fixmap(FIX_PTE, __pa_symbol(&iee_tcr), FIXMAP_PAGE_NORMAL);
	ptr += (unsigned long)(&iee_tcr) & (PAGE_SIZE - 1);
	*((u64 *)ptr) = kernel_tcr | TCR_HPD1 | TCR_A1;
	clear_fixmap(FIX_PTE);
}
#endif

static void __init iee_setup_bootcpu_stack(void)
{
	u64 *cpu_stack_ptr = (u64 *)(SHIFT_PERCPU_PTR(iee_cpu_stack_ptr,
				__per_cpu_offset[0]));

	/* Simply use kernel image address here. */
	*cpu_stack_ptr = (u64)(&iee_init_stack) + IEE_STACK_SIZE;
}

/* Allocate memory block for iee early data pool. */
static phys_addr_t __init iee_mem_pool_early_alloc(struct iee_early_alloc *cache,
							unsigned int order)
{
	phys_addr_t phys = 0;
	void *ptr;
	int i;
	unsigned long block_size = (PAGE_SIZE << (order));
	/* Try smaller block if alloc failed. */
	while (!phys && order >= IEE_DATA_ORDER) {
		phys = memblock_phys_alloc_range(block_size,
					block_size, 0, MEMBLOCK_ALLOC_NOLEAKTRACE);
		if (!phys) {
			order--;
			block_size = (PAGE_SIZE << (order));
		}
	}

	if (!phys)
		panic("Failed to allocate %s page\n", cache->name);

	/*
	 * The FIX_{PGD,PUD,PMD} slots may be in active use, but the FIX_PTE
	 * slot will be free, so we can (ab)use the FIX_PTE slot to initialise
	 * any level of table.
	 */
	for (i = 0; i < (1 << (order)); i++) {
		#ifdef CONFIG_PTP
		if (haoc_enabled)
		ptr = pte_set_fixmap_pre_init(phys + i * PAGE_SIZE);
		else
		ptr = pte_set_fixmap(phys + i * PAGE_SIZE);
		#else
		ptr = pte_set_fixmap(phys + i * PAGE_SIZE);
		#endif

		memset(ptr, 0, PAGE_SIZE);

		/*
		 * Implicit barriers also ensure the zeroed page is visible to the page
		 * table walker
		 */
		#ifdef CONFIG_PTP
		if (haoc_enabled)
			pte_clear_fixmap_pre_init();
		else
			pte_clear_fixmap();
		#else
		pte_clear_fixmap();
		#endif
	}

	cache->begin = phys;
	cache->end = phys + block_size;
	/* Reset curr free page position. */
	cache->pos = 0;
	cache->curr_block_nr++;
	if (cache->curr_block_nr > IEE_EARLY_BLOCK_NR)
		panic("IEE: early data too large.");
	/* Record allocated blocks before IEE initialization finishied. */
	cache->blocks[cache->curr_block_nr].start = phys;
	cache->blocks[cache->curr_block_nr].order = order;
	return phys;
}

/* Calculate the reserved size for early data. */
static unsigned int get_iee_alloc_order(int shift)
{
	phys_addr_t start, end;
	u64 i = 0, size_order = 0;
	unsigned long size = 0;

	for_each_mem_range(i, &start, &end) {
		if (start >= end)
			break;
		size += (end - start);
	}

	size = size >> 36;
	while (size >> size_order)
		size_order++;
	return IEE_DATA_ORDER + (size_order + shift);
}

/* Prepare one block for each early page pool. */
void __init early_iee_data_cache_init(void)
{
	if (!haoc_enabled)
		return;
	/* Calculate IEE stack alloc block size. */
	iee_mem_pool_early_alloc(&iee_stack, IEE_DATA_ORDER);
	/* Calculate IEE data alloc block size. */
	iee_mem_pool_early_alloc(&iee_data, get_iee_alloc_order(1));
	#ifdef CONFIG_PTP
	iee_mem_pool_early_alloc(&iee_pgtable, get_iee_alloc_order(0));
	#endif
}

phys_addr_t __init iee_early_alloc(struct iee_early_alloc *cache,
					int order)
{
	phys_addr_t phys;
	phys_addr_t expand_phys;
	unsigned int block_order, expand_order;

redo:
	if ((cache->begin + cache->pos * PAGE_SIZE + (PAGE_SIZE << order))
				<= cache->end) {
		phys = cache->begin + cache->pos * PAGE_SIZE;
		cache->pos += (1 << order);
	} else {
		/* Use current order to expand. */
		expand_order = cache->blocks[cache->curr_block_nr].order;
		expand_phys = iee_mem_pool_early_alloc(cache, expand_order);

		/* Put the expanded memory into IEE if late enough. */
		block_order = cache->blocks[cache->curr_block_nr].order;
		if (iee_init_done)
			put_pages_into_iee((unsigned long)__va(expand_phys), block_order);
		goto redo;
	}
	return phys;
}

/* Allocate IEE Stack from the reserved page pool.
 * @order: The allocated size is (1 << order) pages.
 *
 * RETURNS:
 * the start of physical address of allocated pages.
 */
phys_addr_t __init early_iee_stack_alloc(int order)
{
	return iee_early_alloc(&iee_stack, order);
}

phys_addr_t __init early_iee_data_alloc(int shift)
{
	return iee_early_alloc(&iee_data, 0);
}

#ifdef CONFIG_PTP
phys_addr_t __init early_iee_pgtable_alloc(int shift)
{
	return iee_early_alloc(&iee_pgtable, 0);
}
#endif

void set_swapper_pgd(pgd_t *pgdp, pgd_t pgd)
{
	pgd_t *fixmap_pgdp;

	spin_lock(&swapper_pgdir_lock);
	fixmap_pgdp = pgd_set_fixmap(__pa_symbol(pgdp));
	WRITE_ONCE(*fixmap_pgdp, pgd);
	/*
	 * We need dsb(ishst) here to ensure the page-table-walker sees
	 * our new entry before set_p?d() returns. The fixmap's
	 * flush_tlb_kernel_range() via clear_fixmap() does this for us.
	 */
	pgd_clear_fixmap();
	spin_unlock(&swapper_pgdir_lock);
}

pgprot_t phys_mem_access_prot(struct file *file, unsigned long pfn,
			      unsigned long size, pgprot_t vma_prot)
{
	if (!pfn_is_map_memory(pfn))
		return pgprot_noncached(vma_prot);
	else if (file->f_flags & O_SYNC)
		return pgprot_writecombine(vma_prot);
	return vma_prot;
}
EXPORT_SYMBOL(phys_mem_access_prot);

phys_addr_t __init early_pgtable_alloc(int shift)
{
	#ifdef CONFIG_PTP
	if (haoc_enabled)
		return early_iee_pgtable_alloc(shift);
	else
	{
		phys_addr_t phys;
		void *ptr;

		phys = memblock_phys_alloc_range(PAGE_SIZE, PAGE_SIZE, 0,
						MEMBLOCK_ALLOC_NOLEAKTRACE);
		if (!phys)
			panic("Failed to allocate page table page\n");

		/*
		* The FIX_{PGD,PUD,PMD} slots may be in active use, but the FIX_PTE
		* slot will be free, so we can (ab)use the FIX_PTE slot to initialise
		* any level of table.
		*/
		ptr = pte_set_fixmap(phys);

		memset(ptr, 0, PAGE_SIZE);

		/*
		* Implicit barriers also ensure the zeroed page is visible to the page
		* table walker
		*/
		pte_clear_fixmap();

		return phys;
	}
	#else
	phys_addr_t phys;
	void *ptr;

	phys = memblock_phys_alloc_range(PAGE_SIZE, PAGE_SIZE, 0,
					 MEMBLOCK_ALLOC_NOLEAKTRACE);
	if (!phys)
		panic("Failed to allocate page table page\n");

	/*
	 * The FIX_{PGD,PUD,PMD} slots may be in active use, but the FIX_PTE
	 * slot will be free, so we can (ab)use the FIX_PTE slot to initialise
	 * any level of table.
	 */
	ptr = pte_set_fixmap(phys);

	memset(ptr, 0, PAGE_SIZE);

	/*
	 * Implicit barriers also ensure the zeroed page is visible to the page
	 * table walker
	 */
	pte_clear_fixmap();

	return phys;
	#endif
}

bool pgattr_change_is_safe(u64 old, u64 new)
{
	/*
	 * The following mapping attributes may be updated in live
	 * kernel mappings without the need for break-before-make.
	 */
	pteval_t mask = PTE_PXN | PTE_RDONLY | PTE_WRITE | PTE_NG;

	/* creating or taking down mappings is always safe */
	if (!pte_valid(__pte(old)) || !pte_valid(__pte(new)))
		return true;

	/* A live entry's pfn should not change */
	if (pte_pfn(__pte(old)) != pte_pfn(__pte(new)))
		return false;

	/* live contiguous mappings may not be manipulated at all */
	if ((old | new) & PTE_CONT)
		return false;

	/* Transitioning from Non-Global to Global is unsafe */
	if (old & ~new & PTE_NG)
		return false;

	/*
	 * Changing the memory type between Normal and Normal-Tagged is safe
	 * since Tagged is considered a permission attribute from the
	 * mismatched attribute aliases perspective.
	 */
	if (((old & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL) ||
	     (old & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL_TAGGED)) &&
	    ((new & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL) ||
	     (new & PTE_ATTRINDX_MASK) == PTE_ATTRINDX(MT_NORMAL_TAGGED)))
		mask |= PTE_ATTRINDX_MASK;

	return ((old ^ new) & ~mask) == 0;
}

static void iee_init_pte(pmd_t *pmdp, unsigned long addr, unsigned long end,
		     phys_addr_t phys, pgprot_t prot)
{
	pte_t *ptep;

	ptep = pte_set_fixmap_offset(pmdp, addr);
	do {
		pte_t old_pte = ptep_get(ptep);

		set_pte(ptep, pfn_pte(__phys_to_pfn(phys), prot));

		/*
		 * After the PTE entry has been populated once, we
		 * only allow updates to the permission attributes.
		 */
		IEE_CHECK(!pgattr_change_is_safe(pte_val(old_pte),
					      pte_val(ptep_get(ptep))));

		phys += PAGE_SIZE;
	} while (ptep++, addr += PAGE_SIZE, addr != end);

	pte_clear_fixmap();
}

static void iee_alloc_init_cont_pte(pmd_t *pmdp, unsigned long addr,
				unsigned long end, phys_addr_t phys,
				pgprot_t prot,
				phys_addr_t (*pgtable_alloc)(int),
				int flags)
{
	unsigned long next;
	pmd_t pmd = READ_ONCE(*pmdp);

	IEE_CHECK(pmd_sect(pmd));
	if (pmd_none(pmd)) {
		pmdval_t pmdval = PMD_TYPE_TABLE | PMD_TABLE_UXN | PMD_SECT_AF;
		phys_addr_t pte_phys;

		if (flags & NO_EXEC_MAPPINGS)
			pmdval |= PMD_TABLE_PXN;
		IEE_CHECK(!pgtable_alloc);
		pte_phys = pgtable_alloc(PAGE_SHIFT);
		__pmd_populate(pmdp, pte_phys, pmdval);
		pmd = READ_ONCE(*pmdp);
	}
	IEE_CHECK(pmd_bad(pmd));

	do {
		pgprot_t __prot = prot;

		next = pte_cont_addr_end(addr, end);

		/* use a contiguous mapping if the range is suitably aligned */
		if ((((addr | next | phys) & ~CONT_PTE_MASK) == 0) &&
		    (flags & NO_CONT_MAPPINGS) == 0)
			__prot = __pgprot(pgprot_val(prot) | PTE_CONT);

		iee_init_pte(pmdp, addr, next, phys, __prot);

		phys += next - addr;
	} while (addr = next, addr != end);
}

static void iee_init_pmd(pud_t *pudp, unsigned long addr, unsigned long end,
		     phys_addr_t phys, pgprot_t prot,
		     phys_addr_t (*pgtable_alloc)(int), int flags)
{
	unsigned long next;
	pmd_t *pmdp;

	pmdp = pmd_set_fixmap_offset(pudp, addr);
	do {
		pmd_t old_pmd = READ_ONCE(*pmdp);

		next = pmd_addr_end(addr, end);

		/* try section mapping first */
		if (((addr | next | phys) & ~PMD_MASK) == 0 &&
		    (flags & NO_BLOCK_MAPPINGS) == 0) {
			pmd_set_huge(pmdp, phys, prot);

			/*
			 * After the PMD entry has been populated once, we
			 * only allow updates to the permission attributes.
			 */
			IEE_CHECK(!pgattr_change_is_safe(pmd_val(old_pmd),
						      READ_ONCE(pmd_val(*pmdp))));
		} else {
			iee_alloc_init_cont_pte(pmdp, addr, next, phys, prot,
					    pgtable_alloc, flags);

			IEE_CHECK(pmd_val(old_pmd) != 0 &&
			       pmd_val(old_pmd) != READ_ONCE(pmd_val(*pmdp)));
		}
		phys += next - addr;
	} while (pmdp++, addr = next, addr != end);

	pmd_clear_fixmap();
}

static void iee_alloc_init_cont_pmd(pud_t *pudp, unsigned long addr,
				unsigned long end, phys_addr_t phys,
				pgprot_t prot,
				phys_addr_t (*pgtable_alloc)(int), int flags)
{
	unsigned long next;
	pud_t pud = READ_ONCE(*pudp);

	/*
	 * Check for initial section mappings in the pgd/pud.
	 */
	IEE_CHECK(pud_sect(pud));
	if (pud_none(pud)) {
		pudval_t pudval = PUD_TYPE_TABLE | PUD_TABLE_UXN | PMD_SECT_AF;
		phys_addr_t pmd_phys;

		if (flags & NO_EXEC_MAPPINGS)
			pudval |= PUD_TABLE_PXN;
		IEE_CHECK(!pgtable_alloc);
		pmd_phys = pgtable_alloc(PMD_SHIFT);
		__pud_populate(pudp, pmd_phys, pudval);
		pud = READ_ONCE(*pudp);
	}
	IEE_CHECK(pud_bad(pud));

	do {
		pgprot_t __prot = prot;

		next = pmd_cont_addr_end(addr, end);

		/* use a contiguous mapping if the range is suitably aligned */
		if ((((addr | next | phys) & ~CONT_PMD_MASK) == 0) &&
		    (flags & NO_CONT_MAPPINGS) == 0)
			__prot = __pgprot(pgprot_val(prot) | PTE_CONT);

		iee_init_pmd(pudp, addr, next, phys, __prot, pgtable_alloc, flags);

		phys += next - addr;
	} while (addr = next, addr != end);
}

static void iee_alloc_init_pud(pgd_t *pgdp, unsigned long addr, unsigned long end,
			   phys_addr_t phys, pgprot_t prot,
			   phys_addr_t (*pgtable_alloc)(int),
			   int flags)
{
	unsigned long next;
	pud_t *pudp;
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	p4d_t p4d = READ_ONCE(*p4dp);

	if (p4d_none(p4d)) {
		p4dval_t p4dval = P4D_TYPE_TABLE | P4D_TABLE_UXN | PMD_SECT_AF;
		phys_addr_t pud_phys;

		if (flags & NO_EXEC_MAPPINGS)
			p4dval |= P4D_TABLE_PXN;
		IEE_CHECK(!pgtable_alloc);
		pud_phys = pgtable_alloc(PUD_SHIFT);
		__p4d_populate(p4dp, pud_phys, p4dval);
		p4d = READ_ONCE(*p4dp);
	}
	IEE_CHECK(p4d_bad(p4d));

	pudp = pud_set_fixmap_offset(p4dp, addr);
	do {
		pud_t old_pud = READ_ONCE(*pudp);

		next = pud_addr_end(addr, end);

		iee_alloc_init_cont_pmd(pudp, addr, next, phys, prot,
					    pgtable_alloc, flags);

		IEE_CHECK(pud_val(old_pud) != 0 &&
			       pud_val(old_pud) != READ_ONCE(pud_val(*pudp)));
		phys += next - addr;
	} while (pudp++, addr = next, addr != end);

	pud_clear_fixmap();
}

static void __create_pgd_mapping_locked(pgd_t *pgdir, phys_addr_t phys,
					unsigned long virt, phys_addr_t size,
					pgprot_t prot,
					phys_addr_t (*pgtable_alloc)(int),
					int flags)
{
	unsigned long addr, end, next;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, virt);

	/*
	 * If the virtual and physical address don't have the same offset
	 * within a page, we cannot map the region as the caller expects.
	 */
	if (WARN_ON((phys ^ virt) & ~PAGE_MASK))
		return;

	phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do {
		next = pgd_addr_end(addr, end);
		iee_alloc_init_pud(pgdp, addr, next, phys, prot, pgtable_alloc,
			       flags);
		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

void __create_pgd_mapping(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags)
{
	mutex_lock(&fixmap_lock);
	__create_pgd_mapping_locked(pgdir, phys, virt, size, prot,
				    pgtable_alloc, flags);
	mutex_unlock(&fixmap_lock);
}

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
extern __alias(__create_pgd_mapping_locked)
void create_kpti_ng_temp_pgd(pgd_t *pgdir, phys_addr_t phys, unsigned long virt,
			     phys_addr_t size, pgprot_t prot,
			     phys_addr_t (*pgtable_alloc)(int), int flags);
#endif

phys_addr_t __pgd_pgtable_alloc(int shift)
{
	void *ptr;
	#ifdef CONFIG_PTP
	if (haoc_enabled)
		ptr = ptp_pg_alloc(&pg_cache, GFP_PGTABLE_KERNEL);
	else
		ptr = (void *)__get_free_page(GFP_PGTABLE_KERNEL);
	#else
	ptr = (void *)__get_free_page(GFP_PGTABLE_KERNEL);
	#endif
	IEE_CHECK(!ptr);

	/* Ensure the zeroed page is visible to the page table walker */
	dsb(ishst);
	return __pa(ptr);
}

phys_addr_t pgd_pgtable_alloc(int shift)
{
	phys_addr_t pa = __pgd_pgtable_alloc(shift);
	struct ptdesc *ptdesc = page_ptdesc(phys_to_page(pa));

	/*
	 * Call proper page table ctor in case later we need to
	 * call core mm functions like apply_to_page_range() on
	 * this pre-allocated page table.
	 *
	 * We don't select ARCH_ENABLE_SPLIT_PMD_PTLOCK if pmd is
	 * folded, and if so pagetable_pte_ctor() becomes nop.
	 */
	if (shift == PAGE_SHIFT)
		IEE_CHECK(!pagetable_pte_ctor(ptdesc));
	else if (shift == PMD_SHIFT)
		IEE_CHECK(!pagetable_pmd_ctor(ptdesc));

	return pa;
}

/* This function is almost the same with __create_pgd_mapping_locked()
 * but not permitting block descriptors larger than pmd block to simplify
 * page table opeartions like splitting blocks.
 */
void __iee_create_pgd_mapping_locked(pgd_t *pgdir, phys_addr_t phys,
					unsigned long virt, phys_addr_t size,
					pgprot_t prot,
					phys_addr_t (*pgtable_alloc)(int),
					int flags)
{
	unsigned long addr, end, next;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, virt);

	/*
	 * If the virtual and physical address don't have the same offset
	 * within a page, we cannot map the region as the caller expects.
	 */
	if (WARN_ON((phys ^ virt) & ~PAGE_MASK))
		return;

	phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do {
		next = pgd_addr_end(addr, end);
		iee_alloc_init_pud(pgdp, addr, next, phys, prot, pgtable_alloc,
			       flags);
		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

/* Mark the pgd entry of IEE address ranges with APTable to setup isolation. */
static void __init __create_pgd_mapping_for_iee_locked(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags)
{
	unsigned long addr, end, next;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, virt);
	p4d_t *p4dp;
	p4d_t p4d;

	/*
	 * If the virtual and physical address don't have the same offset
	 * within a page, we cannot map the region as the caller expects.
	 */
	if (WARN_ON((phys ^ virt) & ~PAGE_MASK))
		return;

	phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do {
		next = pgd_addr_end(addr, end);
		p4dp = p4d_offset(pgdp, addr);
		p4d = READ_ONCE(*p4dp);
		if (!p4d_none(p4d) && !(p4d_val(p4d) & PGD_APTABLE_RO)) {
			phys += next - addr;
			continue;
		}
		#ifdef CONFIG_PTP
		if (haoc_enabled)
			iee_alloc_init_pud_pre_init(pgdp, addr, next, phys, prot, pgtable_alloc,
					flags);
		else
			iee_alloc_init_pud(pgdp, addr, next, phys, prot, pgtable_alloc,
					flags);
		#else
		iee_alloc_init_pud(pgdp, addr, next, phys, prot, pgtable_alloc,
			       flags);
		#endif

		/* Set APTable RO on pgd entries of IEE mappings to prevent kernel access
		 * when TCR.HPD1 == 0.
		 */
		p4d = READ_ONCE(*p4dp);
		#ifdef CONFIG_PTP
		if (haoc_enabled)
			iee_set_pgtable_pre_init((unsigned long *)p4dp,
					(unsigned long)(__phys_to_p4d_val(__p4d_to_phys(p4d))
					| (PGD_APTABLE_RO | PGD_PXNTABLE | PGD_UXNTABLE | PUD_TYPE_TABLE)));
		else
				__p4d_populate(p4dp, __p4d_to_phys(p4d), (PGD_APTABLE_RO | PGD_PXNTABLE |
				PGD_UXNTABLE | PUD_TYPE_TABLE));
		#else
		__p4d_populate(p4dp, __p4d_to_phys(p4d), (PGD_APTABLE_RO | PGD_PXNTABLE |
				PGD_UXNTABLE | PUD_TYPE_TABLE));
		#endif

		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

static void __create_pgd_mapping_for_iee(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags)
{
	mutex_lock(&fixmap_lock);
	__create_pgd_mapping_for_iee_locked(pgdir, phys, virt, size, prot,
				    pgtable_alloc, flags);
	mutex_unlock(&fixmap_lock);
}

static void __init __map_memblock_for_iee(pgd_t *pgdp, phys_addr_t start,
				  phys_addr_t end, pgprot_t prot, int flags)
{
	__create_pgd_mapping_for_iee(pgdp, start, __phys_to_iee(start), end - start,
			     prot, early_pgtable_alloc, flags);
}

/*
 * First function in IEE initialization. Create IEE linear mappings inside
 * kernel address space to access the protected objects.
 */
void __init iee_init_mappings(pgd_t *pgdp)
{
	phys_addr_t start, end;
	int flags = NO_EXEC_MAPPINGS;
	u64 i;

	/* Check if haoc is enabled by kernel parameter. */
	if (!haoc_enabled) {
		pr_info("HAOC is disabled by kernel command line.");
		return;
	}

	/* Check if hardware supports IEE. */
	if (!cpuid_feature_extract_unsigned_field(read_cpuid(ID_AA64MMFR1_EL1),
						ID_AA64MMFR1_EL1_HPDS_SHIFT)) {
		pr_err("Architecture doesn't support HPDS, please disable CONFIG_IEE.\n");
		haoc_enabled = false;
		return;
	}
	else
		pr_info("HAOC: ARM64 hardware support detected.");

	/*
	 *  Not allowing block or continuous mappings on IEE for faster page
	 *  attribution modification.
	 */
	flags |= NO_BLOCK_MAPPINGS | NO_CONT_MAPPINGS;

	/* map all the memory banks non-executable but invalid on iee addresses. */
	for_each_mem_range(i, &start, &end) {
		if (start >= end)
			break;
		__map_memblock_for_iee(pgdp, start, end, SET_NG(SET_INVALID(PAGE_KERNEL)),
					flags);
	}
	#ifdef CONFIG_PTP
	iee_init_tcr_ptp();
	#else
	iee_init_tcr();
	#endif
	iee_setup_bootcpu_stack();
	#ifdef CONFIG_PTP
	pr_info("HAOC: CONFIG_PTP enabled.");
	#endif
}

static void setup_iee_data_cache_bitmap(struct iee_early_alloc *cache,
				enum HAOC_BITMAP_TYPE type)
{
	int block_nr = cache->curr_block_nr + 1;

	for (int j = 0; j < block_nr; j++) {
		iee_set_bitmap_type((unsigned long)__va(cache->blocks[j].start),
				1 << cache->blocks[j].order, type);
	}
	#ifdef DEBUG
	pr_info("IEE: Mark bitmap of %s block nr %d", cache->name, block_nr);
	#endif
}

void __init setup_iee_early_data_bitmap(void)
{
	setup_iee_data_cache_bitmap(&iee_data, IEE_DATA);
	setup_iee_data_cache_bitmap(&iee_stack, IEE_DATA);
	#ifdef CONFIG_PTP
	if (haoc_enabled)
	setup_iee_data_cache_bitmap(&iee_pgtable, IEE_PGTABLE);
	#endif
}

#ifdef CONFIG_PTP
static void setup_iee_early_address(struct iee_early_alloc *cache)
{
	for (int j = 0; j < cache->curr_block_nr+1; j++) {
		for (int i = 0; i < (1 << cache->blocks[j].order); i++) {
			set_iee_address_pre_init(__phys_to_iee(cache->blocks[j].start
						+ i * PAGE_SIZE), true);
		}
	}
}
#endif

static void prot_iee_early_data_cache(struct iee_early_alloc *cache)
{
	int block_nr = cache->curr_block_nr + 1;

	for (int j = 0; j < block_nr; j++) {
		put_pages_into_iee((unsigned long)__va(cache->blocks[j].start),
				cache->blocks[j].order);
	}
}

/* Put early allocated pages into IEE. */
void __init init_early_iee_data(void)
{
	u64 i;
	if (!haoc_enabled)
		return;

	#ifdef CONFIG_PTP
	if (haoc_enabled){
		for (i = 0; ((unsigned long)idmap_pg_dir + i * PAGE_SIZE) <
						(unsigned long)__init_begin; i++) {
			set_iee_address_pre_init(__phys_to_iee(__pa_symbol((unsigned long)idmap_pg_dir
						+ i * PAGE_SIZE)), true);
		}
		for (i = 0; ((unsigned long)__init_end + i * PAGE_SIZE) <
						(unsigned long)iee_init_data_end; i++) {
			set_iee_address_pre_init(__phys_to_iee(__pa_symbol((unsigned long)__init_end
						+ i * PAGE_SIZE)), true);
		}
		setup_iee_early_address(&iee_pgtable);
		setup_iee_early_address(&iee_data);

		prot_iee_early_data_cache(&iee_pgtable);
	}
	#endif

	for (i = 0; (iee_init_data_begin + i * PAGE_SIZE) < iee_init_data_end; i++)
		set_iee_address(__phys_to_iee(__pa_symbol(iee_init_data_begin + i * PAGE_SIZE)),
					0, true);
	prot_iee_early_data_cache(&iee_stack);
}

#ifdef CONFIG_PTP
void __init fixmap_copy_ptp(pgd_t *pgdir)
{
	if (!READ_ONCE(pgd_val(*pgd_offset_pgd(pgdir, FIXADDR_TOT_START)))) {
		/*
		 * The fixmap falls in a separate pgd to the kernel, and doesn't
		 * live in the carveout for the swapper_pg_dir. We can simply
		 * re-use the existing dir for the fixmap.
		 */
		iee_set_pgd_pre_init(pgd_offset_pgd(pgdir, FIXADDR_TOT_START),
			READ_ONCE(*pgd_offset_k(FIXADDR_TOT_START)));
	} else if (CONFIG_PGTABLE_LEVELS > 3) {
		pgd_t *bm_pgdp;
		p4d_t *bm_p4dp;
		pud_t *bm_pudp;
		pudval_t pudval;
		/*
		 * The fixmap shares its top level pgd entry with the kernel
		 * mapping. This can really only occur when we are running
		 * with 16k/4 levels, so we can simply reuse the pud level
		 * entry instead.
		 */
		IEE_CHECK(!IS_ENABLED(CONFIG_ARM64_16K_PAGES));
		bm_pgdp = pgd_offset_pgd(pgdir, FIXADDR_TOT_START);
		bm_p4dp = p4d_offset(bm_pgdp, FIXADDR_TOT_START);
		bm_pudp = pud_set_fixmap_offset(bm_p4dp, FIXADDR_TOT_START);
		pudval = PUD_TYPE_TABLE | PUD_TABLE_AF;
		pudval |= PUD_TABLE_UXN;
		iee_set_pgtable_pre_init((unsigned long *)bm_pudp,
				(unsigned long)(__phys_to_pud_val(__pa_symbol(bm_pmd)) | pudval));
		pud_clear_fixmap_pre_init();
	} else {
		BUG();
	}
}

int __pmdp_set_access_flags(struct vm_area_struct *vma,
			    unsigned long address, pmd_t *pmdp,
			    pmd_t entry, int dirty)
{
	pmdval_t old_pmdval, pmdval;
	pmd_t pmd = READ_ONCE(*pmdp);

	if (pmd_same(pmd, entry))
		return 0;

	/* only preserve the access flags and write permission */
	pmd_val(entry) &= PTE_RDONLY | PTE_AF | PTE_WRITE | PTE_DIRTY;

	/*
	 * Setting the flags must be done atomically to avoid racing with the
	 * hardware update of the access/dirty state. The PTE_RDONLY bit must
	 * be set to the most permissive (lowest value) of *ptep and entry
	 * (calculated as: a & b == ~(~a | ~b)).
	 */
	pmd_val(entry) ^= PTE_RDONLY;
	pmdval = pmd_val(pmd);
	do {
		old_pmdval = pmdval;
		pmdval ^= PTE_RDONLY;
		pmdval |= pmd_val(entry);
		pmdval ^= PTE_RDONLY;
		pmdval = iee_set_pmd_cmpxchg_relaxed(pmdp, old_pmdval, pmdval);
	} while (pmdval != old_pmdval);

	/* Invalidate a stale read-only entry */
	if (dirty)
		flush_tlb_page(vma, address);
	return 1;
}

void * __ref __ptp_vmemmap_alloc_block(unsigned long size, int node)
{
	int order = get_order(size);

	/* If the main allocator is up use that, fallback to bootmem. */
	if (slab_is_available())
		return ptp_pg_alloc(&pg_cache, GFP_KERNEL | __GFP_ZERO);

	if (order != 0)
		panic("PTP: Unsupport vmemmap alloc.");
	return __va(early_iee_pgtable_alloc(0));
}
#endif
