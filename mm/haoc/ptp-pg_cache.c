// SPDX-License-Identifier: GPL-2.0
#include <linux/memblock.h>
#include <linux/gfp_types.h>
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/cpu.h>
#include <linux/ptp-cache.h>
#include <asm/haoc/iee-access.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-func.h>

struct pg_cache pg_cache;

static inline void *__get_freepointer(void *object)
{
	return *((void **)object);
};

static inline void __set_freepointer(void *object, void *next_object)
{
	*(void **)object = next_object;
};

static inline void __iee_set_freepointer(void *object, void *next_object)
{
	iee_set_freeptr(object, next_object);
};

static inline bool __update_freelist(struct pg_cache *cache, void *freelist_old, void *freelist_new,
										unsigned long tid)
{
	union freelist_aba_t old = { .freelist = freelist_old, .counter = tid };
	union freelist_aba_t new = { .freelist = freelist_new, .counter = tid + 1 };

	return try_cmpxchg128(&(cache->freelist_tid.full), &old.full, new.full);
}

#ifdef CONFIG_ARM64
static void __ptp_set_iee_pages(unsigned long start_addr, unsigned long end_addr,
				struct pg_cache *cache)
{
	unsigned long addr;

	if (start_addr != ALIGN(start_addr, PMD_SIZE))
		panic("IEE: %s pool not aligned.", cache->name);

	addr = start_addr;
	while (addr < end_addr) {
		set_iee_page(addr, PMD_ORDER, IEE_PGTABLE);
		addr += PMD_SIZE;
	}
	flush_tlb_kernel_range(start_addr, end_addr);
}
#endif

void __init ptp_pg_cache_init(struct pg_cache *cache, unsigned long object_order,
								int levels, const char *name)
{
	unsigned long addr;
	unsigned long addr_next;
	unsigned long reserve_order;
	unsigned long reserve_pages;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long object_size;
	struct page *page;

	object_size = (1 << object_order) * PAGE_SIZE;
	reserve_order = CONFIG_PTP_RESERVE_ORDER;
	while (1) {
		reserve_pages = (1 << reserve_order) * levels;
		start_addr = (unsigned long)memblock_alloc(reserve_pages * PAGE_SIZE,
			reserve_pages * PAGE_SIZE);
		if (start_addr)
			break;
		reserve_order--;
		/* Allocate pages in pmd blocks to reduce the mapping cost. */
		if (reserve_order < PMD_ORDER)
			panic("IEE: fail to reserve pages for %s", name);
	}
	end_addr = start_addr + reserve_pages * PAGE_SIZE;
	pr_err("IEE: reserve %ld pages for %s, range[0x%lx, 0x%lx]",
			reserve_pages, name, start_addr, end_addr);

	addr = start_addr;
	addr_next = addr + object_size;
	while (addr < end_addr) {
		page = virt_to_page(addr);
		set_page_count(page, 1);
		__set_freepointer((void *)addr, (void *)addr_next);
		addr += object_size;
		addr_next += object_size;
	}
	__set_freepointer((void *)(addr - object_size), NULL);

	cache->object_order = object_order;
	cache->reserve_order = reserve_order;
	cache->reserve_start_addr = start_addr;
	cache->reserve_end_addr = end_addr;
	cache->levels = levels;
	cache->freelist = (void *)start_addr;
	cache->tid = 0;
	cache->levels = levels;
	cache->name = name;

#ifdef CONFIG_ARM64
	/* IEE for ARM64 needs to access these data by IEE addresses. */
	__ptp_set_iee_pages(start_addr, end_addr, cache);
#endif
#ifdef DEBUG
	atomic_set(&cache->count, reserve_pages);
	atomic_set(&cache->fail_count, 0);
	pr_info("IEE: %s ready. object size 0x%lx, count %d.",
				name, object_size, atomic_read(&cache->count));
#endif
}

void __init ptp_set_iee_reserved(struct pg_cache *cache)
{
	#ifdef CONFIG_X86_64
	unsigned long addr = cache->reserve_start_addr;

	for (int i = 0; i < cache->levels; i++) {
		set_iee_page(addr,cache->reserve_order);
		addr += (1 << cache->reserve_order) * PAGE_SIZE;
	}
	#endif
}

#ifdef CONFIG_ARM64
/* Expand the cache pool with PMD_SIZE for each time. */
static int __ref ptp_pg_cache_expand(struct pg_cache *cache, gfp_t gfp)
{
	unsigned long addr;
	unsigned long addr_next;
	unsigned long start_addr;
	unsigned long end_addr;
	unsigned long object_size;
	unsigned long tid;
	void **freelist;
	struct page *page;

	if (slab_is_available())
		start_addr = __get_free_pages(gfp, PMD_ORDER);
	else
		start_addr = (unsigned long)memblock_alloc(PMD_SIZE, PMD_SIZE);

	if (!start_addr)
		return 0;

	addr = start_addr;
	object_size = (1 << cache->object_order) * PAGE_SIZE;
	addr_next = addr + object_size;
	end_addr = start_addr + PMD_SIZE;
	while (addr < end_addr) {
		page = virt_to_page(addr);
		set_page_count(page, 1);
		__set_freepointer((void *)addr, (void *)addr_next);
		addr += object_size;
		addr_next += object_size;
	}
	__set_freepointer((void *)(addr - object_size), NULL);

	/* IEE for ARM64 needs to access these data by IEE addresses. */
	__ptp_set_iee_pages(start_addr, end_addr, cache);

#ifdef DEBUG
	atomic_add(1 << PMD_ORDER, &cache->count);
	pr_info("IEE: %s expand to count %d. Curr failed: %d", cache->name,
				atomic_read(&cache->count), atomic_read(&cache->fail_count));
#endif

/* Fill the new allocated pages into the cache freelist. */
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	freelist = READ_ONCE(cache->freelist);
	__iee_set_freepointer((void *)(end_addr - object_size), freelist);
	if (unlikely(!__update_freelist(cache, freelist, (void *)start_addr, tid)))
		goto redo;

	return 1;
}
#endif

void *ptp_pg_alloc(struct pg_cache *cache, gfp_t gfp)
{
	unsigned long tid;
	void *object;
	void *next_object;
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	object = READ_ONCE(cache->freelist);
	if (unlikely(!object)) {
		// slow path alloc
		#ifdef CONFIG_ARM64
		if (ptp_pg_cache_expand(cache, gfp) || READ_ONCE(cache->freelist))
			goto redo;

		/* If the expandsion failed, alloc a singel object without RO protection to
		 * avoid block spliting.
		 */
		object = (void *)__get_free_pages(gfp, cache->object_order);
		set_iee_address_valid((unsigned long)object, cache->object_order);
		iee_set_bitmap_type((unsigned long)object, cache->object_order, IEE_PGTABLE);
		#ifdef DEBUG
		WARN_ONCE(1, "IEE: Failed on %s expansion.", cache->name);
		atomic_add(1 << cache->object_order, &cache->fail_count);
		#endif
		#else
		object = (void *)__get_free_pages(gfp, cache->object_order);
		set_iee_page((unsigned long)object, cache->object_order);
		#endif
	} else {
		// fast path alloc
		next_object = __get_freepointer(object);
		if (unlikely(!__update_freelist(cache, object, next_object, tid)))
			goto redo;
		prefetchw(next_object);
		if (gfp & __GFP_ZERO)
			__iee_set_freepointer(object, NULL);
	}
	return object;
};

void ptp_pg_free(struct pg_cache *cache, void *object)
{
	unsigned long tid;
	void **freelist;

	#ifdef CONFIG_X86_64
	if (unlikely((unsigned long)object < cache->reserve_start_addr
		|| (unsigned long)object >= cache->reserve_end_addr)) {
		// slow path free
		unset_iee_page((unsigned long)object, cache->object_order);
		free_pages((unsigned long)object, cache->object_order);
		return;
	}
	#endif

	// fast path free
redo:
	tid = READ_ONCE(cache->tid);
	barrier();
	freelist = READ_ONCE(cache->freelist);
	__iee_set_freepointer(object, freelist);
	if (unlikely(!__update_freelist(cache, freelist, object, tid)))
		goto redo;
}
