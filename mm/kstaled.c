// SPDX-License-Identifier: GPL-2.0
#include "linux/bitops.h"
#include "linux/bits.h"
#include <linux/kthread.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/mm_inline.h>
#include <linux/module.h>
#include <linux/pagemap.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/page-flags.h>
#include <linux/page_idle.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/kstaled.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/memblock.h>
#include <uapi/linux/sched/types.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pagevec.h>
#include "internal.h"

#define CREATE_TRACE_POINTS
#include <trace/events/kstaled.h>

/*
 * This global scanner named *kstaled* because it's based on the
 * *idle* page tracking feature, and referenced Google and Alibaba's
 * similar feature:
 *
 * 1. https://lore.kernel.org/lkml/20110922161448.91a2e2b2.akpm@google.com/T/
 * 2. https://gitee.com/anolis/cloud-kernel/pulls/565
 *
 * In Michel Lespinasse's patch, each page has a corresponding 8 bits attribute
 * which was called ide_page_age, and use buckets to do histogram sampling.
 * This is a good idea! And Alibaba's kidled also referenced kstaled designs.
 * But in our design, it's spendthrift to reserve 8 bits of page->flags for each
 * page when MGLRU enabled. our kstaled is desiged to coexist with MGLRU while
 * accelerating aging.
 *
 * Thanks Google's kstaled and Alibaba's kidled!
 */

/*
 * In order to speed up the scanning of all PFNs, we use for_each_mem_pfn_range()
 * to skip gigantic holes, especially, the number of invalid PFNs is 85 times
 * that of valid PFNs if NUMA is turned off in arm64. That way is a necessary
 * improvement in kstaled. But a function with __init_memblock attribute is used
 * in for_each_mem_pfn_range(). So __ref is needed in these caller to avoid the
 * warning from compiler when CONFIG_ARCH_KEEP_MEMBLOCK disabled.
 */
#ifdef CONFIG_ARCH_KEEP_MEMBLOCK
#define __kstaled_ref
#else
#define __kstaled_ref __ref
#endif

DEFINE_STATIC_KEY_FALSE(kstaled_enabled_key);

unsigned int kstaled_scan_target __read_mostly = KSTALED_SCAN_PAGE;
struct kstaled_scan_control kstaled_scan_control;
/*
 * These bucket values are copied from Michel Lespinasse's patch, they are
 * the default buckets to do histogram sampling.
 *
 * kstaled also supports each memory cgroup has it's own sampling buckets by
 * configuring memory.idle_page_stats file, and the child memcg will inherit
 * parent's bucket values.
 */
const int kstaled_default_buckets[NUM_KSTALED_BUCKETS] = {
	1, 2, 5, 15, 30, 60, 120, 240 };
static DECLARE_WAIT_QUEUE_HEAD(kstaled_wait);
unsigned long kstaled_scan_rounds __read_mostly;

static inline int kstaled_get_bucket(int *idle_buckets, int age)
{
	int bucket;

	if (age < idle_buckets[0])
		return -EINVAL;

	for (bucket = 1; bucket <= (NUM_KSTALED_BUCKETS - 1); bucket++) {
		if (age < idle_buckets[bucket])
			return bucket - 1;
	}

	return NUM_KSTALED_BUCKETS - 1;
}

static inline int kstaled_get_idle_type(struct folio *folio)
{
	int idle_type = KSTALE_BASE;

	if (folio_test_dirty(folio) || folio_test_writeback(folio))
		idle_type |= KSTALE_DIRTY;
	if (folio_is_file_lru(folio))
		idle_type |= KSTALE_FILE;
	/*
	 * Couldn't call page_evictable() here, because we have not held
	 * the page lock, so use page flags instead. Different from
	 * PageMlocked().
	 */
	if (folio_test_unevictable(folio))
		idle_type |= KSTALE_UNEVICT;
	if (folio_test_active(folio))
		idle_type |= KSTALE_ACTIVE;

	return idle_type;
}

static bool page_idle_clear_pte_refs_one(struct folio *folio,
					struct vm_area_struct *vma,
					unsigned long addr, void *arg)
{
	DEFINE_FOLIO_VMA_WALK(pvmw, folio, vma, addr, 0);
	bool referenced = false;

	while (page_vma_mapped_walk(&pvmw)) {
		addr = pvmw.address;
		if (pvmw.pte) {
			/*
			 * For PTE-mapped THP, one sub page is referenced,
			 * the whole THP is referenced.
			 */
			if (ptep_clear_young_notify(vma, addr, pvmw.pte))
				referenced = true;
		} else if (IS_ENABLED(CONFIG_TRANSPARENT_HUGEPAGE)) {
			if (pmdp_clear_young_notify(vma, addr, pvmw.pmd))
				referenced = true;
		} else {
			/* unexpected pmd-mapped page? */
			WARN_ON_ONCE(1);
		}
	}

	if (referenced) {
		folio_clear_idle(folio);
		/*
		 * We cleared the referenced bit in a mapping to this page. To
		 * avoid interference with page reclaim, mark it young so that
		 * folio_referenced() will return > 0.
		 */
		folio_set_young(folio);
	}
	return true;
}

/*
 * It borrowed from page_idle.c to stay independent from
 * CONFIG_IDLE_PAGE_TRACKING.
 */
static void page_idle_clear_pte_refs(struct folio *folio)
{
	/*
	 * Since rwc.try_lock is unused, rwc is effectively immutable, so we
	 * can make it static to save some cycles and stack.
	 */
	static struct rmap_walk_control rwc = {
		.rmap_one = page_idle_clear_pte_refs_one,
		.anon_lock = folio_lock_anon_vma_read,
	};
	bool need_lock;

	if (!folio_mapped(folio) || !folio_raw_mapping(folio))
		return;

	need_lock = !folio_test_anon(folio) || folio_test_ksm(folio);
	if (need_lock && !folio_trylock(folio))
		return;

	rmap_walk(folio, &rwc);

	if (need_lock)
		folio_unlock(folio);
}

#ifdef CONFIG_MEMCG
void kstaled_mem_cgroup_account(struct folio *folio,
		void *ptr, int age, unsigned long size)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stats;
	int type, bucket;

	if (mem_cgroup_disabled())
		return;

	type = kstaled_get_idle_type(folio);
	folio_memcg_lock(folio);
	memcg = folio_memcg(folio);
	if (unlikely(!memcg)) {
		folio_memcg_unlock(folio);
		return;
	}

	stats = mem_cgroup_get_unstable_idle_stats(memcg);
	bucket = kstaled_get_bucket(stats->buckets, age);
	if (bucket >= 0)
		kstaled_atomic_add(&stats->count[type][bucket], size);

	folio_memcg_unlock(folio);
}

void kstaled_mem_cgroup_move_stats(struct mem_cgroup *from,
				  struct mem_cgroup *to,
				  struct folio *folio,
				  unsigned long size)
{
	pg_data_t *pgdat = folio_pgdat(folio);
	unsigned long pfn = folio_pfn(folio);
	struct idle_page_stats *stats[4] = { NULL, };
	int type, bucket, age;

	if (mem_cgroup_disabled())
		return;

	type = kstaled_get_idle_type(folio);
	stats[0] = mem_cgroup_get_stable_idle_stats(from);
	stats[1] = mem_cgroup_get_unstable_idle_stats(from);
	if (to) {
		stats[2] = mem_cgroup_get_stable_idle_stats(to);
		stats[3] = mem_cgroup_get_unstable_idle_stats(to);
	}

	/*
	 * We assume the all page ages are same if this is a compound page.
	 * Also we uses node's cursor (@node_idle_scan_pfn) to check if current
	 * page should be removed from the source memory cgroup or charged
	 * to target memory cgroup, without introducing locking mechanism.
	 * This may lead to slightly inconsistent statistics, but it's fine
	 * as it will be reshuffled in next round of scanning.
	 */
	age = kstaled_get_folio_age(pgdat, pfn);
	if (age < 0)
		return;

	bucket = kstaled_get_bucket(stats[1]->buckets, age);
	if (bucket < 0)
		return;

	/* Remove from the source memory cgroup */
	kstaled_atomic_sub(&stats[0]->count[type][bucket], size);
	if (pgdat->node_idle_scan_pfn >= pfn)
		kstaled_atomic_sub(&stats[1]->count[type][bucket], size);

	/* Charge to the target memory cgroup */
	if (!to)
		return;

	bucket = kstaled_get_bucket(stats[3]->buckets, age);
	if (bucket < 0)
		return;

	kstaled_atomic_add(&stats[2]->count[type][bucket], size);
	if (pgdat->node_idle_scan_pfn >= pfn)
		kstaled_atomic_add(&stats[3]->count[type][bucket], size);
}
EXPORT_SYMBOL_GPL(kstaled_mem_cgroup_move_stats);

static void kstaled_mem_cgroup_uncharge(struct folio *folio, struct mem_cgroup *memcg)
{
	pg_data_t *pgdat;
	unsigned long pfn;
	struct idle_page_stats *stats[2] = { NULL, };
	int type, bucket, age;
	unsigned long size;

	if (mem_cgroup_disabled() || !is_kstaled_enabled() || !memcg)
		return;

	pgdat = folio_pgdat(folio);
	pfn = folio_pfn(folio);
	age = kstaled_get_folio_age(pgdat, pfn);
	if (age <= 0)
		return;

	type = kstaled_get_idle_type(folio);

	size = folio_nr_pages(folio) << PAGE_SHIFT;

	stats[0] = mem_cgroup_get_stable_idle_stats(memcg);
	stats[1] = mem_cgroup_get_unstable_idle_stats(memcg);

	bucket = kstaled_get_bucket(stats[1]->buckets, age);
	if (bucket < 0)
		return;

	/* Remove from the memory cgroup */
	kstaled_atomic_sub(&stats[0]->count[type][bucket], size);

	if (pgdat->node_idle_scan_pfn >= pfn)
		kstaled_atomic_sub(&stats[1]->count[type][bucket], size);
}

void kstaled_mem_cgroup_uncharge_list(struct folio_batch *folios,
				      bool shrink)
{
	unsigned int i;
	struct {
		int age;
		int count;
	} age_map[SWAP_CLUSTER_MAX + 1] = {
		[0 ... SWAP_CLUSTER_MAX] = {
			.age = -1,
			.count = 0,
		}
	};
	int age_idx[KSTALED_MAX_IDLE_AGE + 1] = {[0 ... KSTALED_MAX_IDLE_AGE] = -1};
	int nr_seen = 0;

	if (mem_cgroup_disabled() || !is_kstaled_enabled())
		return;

	for (i = 0; i < folios->nr; i++) {
		struct folio *folio = folios->folios[i];
		struct mem_cgroup *memcg = folio_memcg(folio);

		if (memcg) {
			pg_data_t *pgdat = folio_pgdat(folio);
			unsigned long pfn = folio_pfn(folio);
			int age = kstaled_get_folio_age(pgdat, pfn);

			if (unlikely(!pgdat->node_page_age))
				return;

			if (shrink && nr_seen < SWAP_CLUSTER_MAX &&
			    age >= 0 && age <= KSTALED_MAX_IDLE_AGE) {
				int idx = age_idx[age];

				if (idx == -1) {
					age_idx[age] = nr_seen;
					age_map[nr_seen].age = age;
					age_map[nr_seen].count++;
					nr_seen++;
				} else
					age_map[idx].count++;
			}
			kstaled_mem_cgroup_uncharge(folio, memcg);
		}
	}

	if (shrink && trace_kstaled_folio_age_enabled()
	    && nr_seen > 0) {
		int i;
		char buf[128] = {'\0'};
		int len = 0;

		for (i = 0; i < nr_seen; i++) {
			int age = age_map[i].age;

			if (len >= 100) {
				trace_kstaled_folio_age(buf);
				len = 0;
				memset(buf, '\0', sizeof(buf));
			}
			len += snprintf(buf + len, sizeof(buf) - len, "[%d:%u] ",
					age, age_map[i].count);
		}

		if (len > 0)
			trace_kstaled_folio_age(buf);
	}
}
EXPORT_SYMBOL_GPL(kstaled_mem_cgroup_uncharge_list);

static inline void
kstaled_mem_cgroup_scan_done(struct kstaled_scan_control scan_control)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stable_stats, *unstable_stats;

	for (memcg = mem_cgroup_iter(NULL, NULL, NULL);
	     memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {

		down_write(&memcg->idle_stats_rwsem);
		stable_stats = mem_cgroup_get_stable_idle_stats(memcg);
		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);

		/* Switch stable buckets when scanning buckets is valid. */
		if (!KSTALED_IS_BUCKET_INVALID(unstable_stats->buckets)) {
			memcg->idle_stable_idx = !memcg->idle_stable_idx;
			if (kstaled_has_page_target(&scan_control))
				memcg->idle_page_scans++;
		} else {
			memcpy(unstable_stats->buckets, stable_stats->buckets,
			       sizeof(unstable_stats->buckets));
		}

		memcg->scan_control = scan_control;
		up_write(&memcg->idle_stats_rwsem);

		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);
		memset(&unstable_stats->count, 0,
		       sizeof(unstable_stats->count));
	}
}

/*
 * Reset the specified statistics by scan_type when users want to
 * change the scan target. For example, we should clear the slab
 * statistics when we only want to scan the page and vice versa.
 * Otherwise it will mislead the user about the statistics.
 */
static inline void
kstaled_mem_cgroup_reset(enum kstaled_scan_type scan_type)
{
	struct mem_cgroup *memcg;
	struct idle_page_stats *stable_stats, *unstable_stats;

	if (scan_type != SCAN_TARGET_PAGE)
		pr_warn_once("kstaled: not support scan type: %d", scan_type);

	for (memcg = mem_cgroup_iter(NULL, NULL, NULL); memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {
		int i;

		down_write(&memcg->idle_stats_rwsem);
		stable_stats = mem_cgroup_get_stable_idle_stats(memcg);
		unstable_stats = mem_cgroup_get_unstable_idle_stats(memcg);

		for (i = 0; i < KSTALE_NR_TYPE - 1; i++)
			memset(&stable_stats->count[i], 0,
			       sizeof(stable_stats->count[i]));

		memcg->idle_page_scans = 0;
		memcg->scan_control.scan_target = kstaled_scan_target;
		up_write(&memcg->idle_stats_rwsem);
		for (i = 0; i < KSTALE_NR_TYPE - 1; i++)
			memset(&unstable_stats->count[i], 0,
			       sizeof(unstable_stats->count[i]));
	}
}
#else /* !CONFIG_MEMCG */
void kstaled_mem_cgroup_account(struct page *page,
		void *ptr, int age, unsigned long size)
{
}
static inline void kstaled_mem_cgroup_scan_done(struct kstaled_scan_control
					       scan_control)
{
}
static inline void kstaled_mem_cgroup_reset(enum kstaled_scan_type scan_type)
{
}
#endif /* CONFIG_MEMCG */

/*
 * An idle page with an older age is more likely idle, while a busy page is
 * more likely busy, so we can reduce the sampling frequency to save cpu
 * resource when meet these pages. And we will keep sampling each time when
 * an idle page is young. See tables below:
 *
 *  idle age |   down ratio
 * ----------+-------------
 * [0, 1)    |     1/2      # busy
 * [1, 4)    |      1       # young idle
 * [4, 8)    |     1/2      # idle
 * [8, 16)   |     1/4      # old idle
 * [16, +inf)|     1/8      # older idle
 */
static inline bool kstaled_need_check_idle(pg_data_t *pgdat, unsigned long pfn)
{
	struct page *page = pfn_to_page(pfn);
	int age = kstaled_get_folio_age(pgdat, pfn);
	unsigned long pseudo_random;

	if (age < 0)
		return false;

	/*
	 * kstaled will check different pages at each round when need
	 * reduce sampling frequency, this depends on current pfn and
	 * global scanning rounds. There exist some special pfns, for
	 * one huge page, we can only check the head page, while tail
	 * pages would be checked in low levels and will be skipped.
	 * Shifting HPAGE_PMD_ORDER bits is to achieve good load balance
	 * for each round when system has many huge pages, 1GB is not
	 * considered here.
	 */
	if (PageHead(page))
		pfn >>= compound_order(page);

	pseudo_random = pfn + kstaled_scan_rounds;
	if (age == 0)
		return pseudo_random & 0x1UL;
	else if (age < 4)
		return true;
	else if (age < 8)
		return pseudo_random & 0x1UL;
	else if (age < 16)
		return (pseudo_random & 0x3UL) == 0x3UL;
	else
		return (pseudo_random & 0x7UL) == 0x7UL;
}

static inline int kstaled_scan_folio(pg_data_t *pgdat, unsigned long pfn)
{
	struct folio *folio;
	int age, nr_pages = 1;
	bool idle = false;

	if (!pfn_valid(pfn))
		goto out;

	folio = pfn_folio(pfn);
	if (!folio || !folio_test_lru(folio)) {
		kstaled_set_folio_age(pgdat, pfn, 0);
		goto out;
	}

	/*
	 * Try to skip clear PTE references which is an expensive call.
	 * PG_idle should be cleared when free a page and we have checked
	 * PG_lru flag above, so the race is acceptable to us.
	 */
	if (folio_test_idle(folio)) {
		if (kstaled_need_check_idle(pgdat, pfn)) {
			if (!folio_try_get(folio)) {
				kstaled_set_folio_age(pgdat, pfn, 0);
				goto out;
			}

			/*
			 * Check again after get a reference count, while in
			 * page_idle_get_page() it gets zone_lru_lock at first,
			 * it seems useless.
			 *
			 * Also we can't hold LRU lock here as the consumed
			 * time to finish the scanning is fixed. Otherwise,
			 * the accumulated statistics will be cleared out
			 * and scan interval (@scan_period_in_seconds) will
			 * be doubled. However, this may incur race between
			 * kstaled and page reclaim. The page reclaim may dry
			 * run due to dumped refcount, but it's acceptable.
			 */
			if (unlikely(!folio_test_lru(folio))) {
				folio_put(folio);
				kstaled_set_folio_age(pgdat, pfn, 0);
				goto out;
			}

			page_idle_clear_pte_refs(folio);
			if (folio_test_idle(folio))
				idle = true;
			folio_put(folio);
		} else if (kstaled_get_folio_age(pgdat, pfn) > 0) {
			idle = true;
		}
	}

	nr_pages = folio_nr_pages(folio);

	if (idle) {
		age = kstaled_inc_folio_age(pgdat, pfn);
		if (age > 0)
			kstaled_mem_cgroup_account(folio, NULL,
					age, nr_pages << PAGE_SHIFT);
		else
			age = 0;
	} else {
		age = 0;
		kstaled_set_folio_age(pgdat, pfn, 0);
		if (folio_try_get(folio)) {
			if (likely(folio_test_lru(folio)))
				folio_set_idle(folio);
			folio_put(folio);
		}
	}

out:
	return nr_pages;
}

static int kstaled_range_populate_pte(unsigned long addr, int nid)
{
	struct page *page;
	struct page *pages[1];
	int err;

	if (vmalloc_to_page((void *)addr))
		return 0;

	page = alloc_pages_node(nid, GFP_KERNEL | __GFP_ZERO, 0);
	if (!page)
		return -ENOMEM;

	pages[0] = page;
	err = vmap_pages_range_noflush(addr, addr + PAGE_SIZE, PAGE_KERNEL,
				       pages, PAGE_SHIFT);
	if (err < 0) {
		__free_page(page);
		return err;
	}

	return 0;
}

static int kstaled_populate_folio_age(pg_data_t *pgdat, unsigned long start,
				    unsigned long end)
{
	unsigned long addr;

	/* TODO try pmd-mapping first */
	for (addr = start; addr < end; addr += PAGE_SIZE) {
		if (kstaled_range_populate_pte(addr, pgdat->node_id))
			return -ENOMEM;
	}
	return 0;
}

static int kstaled_free_pte(pte_t *pte, unsigned long addr, void *ctx)
{
	if (!pte_none(*pte)) {
		struct page *page = pte_page(*pte);

		/*
		 * Just free the physical page, the mapping will be
		 * removed by free_vm_area().
		 */
		__free_page(page);
	}
	return 0;
}

static int kstaled_reset_pte(pte_t *pte, unsigned long addr, void *ctx)
{
	if (!pte_none(*pte))
		clear_page(page_address(pte_page(*pte)));
	return 0;
}

static int kstale_init_age_sparse_vmemmap(pg_data_t *pgdat)
{
	struct vm_struct *area;
	unsigned long start, end;
	unsigned long prev_end_pfn = 0;
	int i;

	area = get_vm_area(PAGE_ALIGN(pgdat->node_spanned_pages) + PAGE_SIZE,
			   VM_ALLOC);
	if (!area)
		return false;

	/* Populate physical pages for valid PFN ranges */
#if defined(CONFIG_ARCH_KEEP_MEMBLOCK) || defined(CONFIG_MEMORY_HOTPLUG)
	for_each_mem_pfn_range(i, pgdat->node_id, &start, &end, NULL) {
		unsigned long virt_start, virt_end;

		start = max(start, pgdat->node_start_pfn);
		end = min(end, pgdat_end_pfn(pgdat));
		if (start >= end)
			continue;

		if (end <= prev_end_pfn)
			continue;

		if (prev_end_pfn >= start && prev_end_pfn < end)
			start = prev_end_pfn + 1;

		virt_start = (unsigned long)area->addr +
			     (start - ALIGN_DOWN(pgdat->node_start_pfn, BITS_PER_BYTE));
		virt_end = (unsigned long)area->addr +
			   (end - ALIGN_DOWN(pgdat->node_start_pfn, BITS_PER_BYTE));
		/*
		 * Align to PAGE_SIZE to ensure we cover all bytes.
		 * virt_start should mask down, virt_end align up (conceptually),
		 * but since we iterate by PAGE_SIZE, we just need to ensure
		 * we start at the page containing the first byte and end > last byte.
		 */
		virt_start &= PAGE_MASK;
		virt_end = PAGE_ALIGN(virt_end);

		if (kstaled_populate_folio_age(pgdat, virt_start, virt_end))
			goto free;

		prev_end_pfn = PAGE_ALIGN(end) - 1;
	}
#else
	/*
	 * Fallback: Iterate PFNs and populate
	 * This might be slow but it's one-time init.
	 * To optimize, we check validity per PAGE (4096 PFNs).
	 */
	{
		unsigned long pfn;
		unsigned long virt_addr = (unsigned long)area->addr;

		for (pfn = pgdat->node_start_pfn;
		     pfn < pgdat_end_pfn(pgdat);
		     pfn += PAGE_SIZE) {
			/* Check if any PFN in this page-sized block is valid */
			unsigned long check_pfn;
			bool valid = false;

			for (check_pfn = pfn;
			     check_pfn < pfn + PAGE_SIZE && check_pfn < pgdat_end_pfn(pgdat);
			     check_pfn++) {
				if (pfn_valid(check_pfn)) {
					valid = true;
					break;
				}
			}

			if (valid) {
				if (kstaled_range_populate_pte(virt_addr, pgdat->node_id))
					goto free;
			}
			virt_addr += PAGE_SIZE;
		}
	}
#endif
	rcu_assign_pointer(pgdat->node_page_age, area->addr);
	return true;

free:
	/*
	 * Cleanup is handled by kstaled_reset(true) or similar?
	 * No, we should clean up here on failure.
	 */
	apply_to_existing_page_range(&init_mm, (unsigned long)area->addr,
				     PAGE_ALIGN(pgdat->node_spanned_pages),
				     kstaled_free_pte, NULL);
	free_vm_area(area);
	rcu_assign_pointer(pgdat->node_page_age, NULL);
	return false;
}

static bool kstaled_scan_node(pg_data_t *pgdat,
			     struct kstaled_scan_control scan_control,
			     unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn = start_pfn;
	unsigned long node_end = pgdat_end_pfn(pgdat);
#if !defined(CONFIG_ARCH_KEEP_MEMBLOCK) && !defined(CONFIG_MEMORY_HOTPLUG)
	unsigned long sequent_invalid_pfns = 0;
	int nr_nodes = num_online_nodes();
#endif

	if (pgdat->node_idle_scan_pfn >= node_end)
		return true;

	if (unlikely(!pgdat->node_page_age)) {
		int ret;

		/* This node has none memory, skip it. */
		if (!pgdat->node_spanned_pages)
			return true;

		ret = kstale_init_age_sparse_vmemmap(pgdat);
		if (!ret) {
			pr_warn_once("fail to init sparse age memmap (node %d)",
				     pgdat->node_id);
			return false;
		}
	}

	while (pfn < end_pfn) {
		/* Restart new scanning when user updates the period */
		if (unlikely(!kstaled_is_scan_period_equal(&scan_control)))
			break;

#if !defined(CONFIG_ARCH_KEEP_MEMBLOCK) && !defined(CONFIG_MEMORY_HOTPLUG)
		if (nr_nodes == 1) {
			if (!pfn_valid(pfn)) {
				sequent_invalid_pfns++;
				if (sequent_invalid_pfns % (2 * HPAGE_PMD_NR) == 0)
					cond_resched();
				pfn++;
				continue;
			}
			sequent_invalid_pfns = 0;
		}
#endif
		cond_resched();
		pfn += kstaled_scan_folio(pgdat, pfn);
	}

	pgdat->node_idle_scan_pfn = pfn;
	return pfn >= node_end;
}

/*
 * Here for_each_mem_pfn_range() only used when either CONFIG_ARCH_KEEP_MEMBLOCK
 * or CONFIG_MEMORY_HOTPLUG is turned on. That because these functions with
 * __init_memblock would been discarded after system running, then crash would
 * happen if caller executes to them.
 */
#if defined(CONFIG_ARCH_KEEP_MEMBLOCK) || defined(CONFIG_MEMORY_HOTPLUG)
static __kstaled_ref bool kstaled_scan_nodes(struct kstaled_scan_control scan_control,
				    bool restart)
{
	int i, nid;
	unsigned long start_pfn, end_pfn;
	bool scan_done = true;

	for_each_online_node(nid) {
		pg_data_t *pgdat = NODE_DATA(nid);
		unsigned long pages_to_scan = DIV_ROUND_UP(pgdat->node_present_pages,
							   scan_control.duration);
		bool init = !restart;

		if (restart)
			pgdat->node_idle_scan_pfn = pgdat->node_start_pfn;

		for_each_mem_pfn_range(i, nid, &start_pfn, &end_pfn, NULL) {
			if (init) {
				/* Start scanning from the previous range */
				if (end_pfn < pgdat->node_idle_scan_pfn)
					continue;

				/*
				 * There are two cases should been noticed:
				 *
				 * 1) end_pfn = node_idle_scan_pfn: only one pfn
				 * will be scanned, and we must increasing end_pfn
				 * to avoid 'start_pfn = end_pfn';
				 *
				 * 2) start_pfn > node_idle_scan_pfn: this indicates
				 * node_idle_scan_pfn locates in an invalid range.
				 * We should update it to next valid range before
				 * scanning.
				 */
				if (end_pfn == pgdat->node_idle_scan_pfn) {
					end_pfn += 1;
					start_pfn = pgdat->node_idle_scan_pfn;
				} else if (start_pfn > pgdat->node_idle_scan_pfn) {
					pgdat->node_idle_scan_pfn = start_pfn;
				} else
					start_pfn = pgdat->node_idle_scan_pfn;
				init = false;
			}

			if ((end_pfn - start_pfn) > pages_to_scan)
				end_pfn = start_pfn + pages_to_scan;
			scan_done &= kstaled_scan_node(pgdat, scan_control,
						      start_pfn, end_pfn);
			/*
			 * That empirical value mainly to ensure that
			 * sufficient PFNs will be scanned in current
			 * period.
			 */
			if ((end_pfn - start_pfn) >= pages_to_scan / 16)
				break; /* Let kstaled scans next node */
		}
	}

	return scan_done;
}
#else
static bool kstaled_scan_nodes(struct kstaled_scan_control scan_control,
			      bool restart)
{
	unsigned long start_pfn, end_pfn;
	pg_data_t *pgdat;
	bool scan_done = true;

	/*
	 * TODO: Perhaps there are massive holes when NUMA disabled.
	 * And this scene only find in arm64.
	 */
	for_each_online_pgdat(pgdat) {
		unsigned long node_end = pgdat_end_pfn(pgdat);

		if (restart)
			pgdat->node_idle_scan_pfn = pgdat->node_start_pfn;

		start_pfn = pgdat->node_idle_scan_pfn;
		end_pfn = min(start_pfn + DIV_ROUND_UP(pgdat->node_spanned_pages,
						 scan_control.duration), node_end);
		scan_done &= kstaled_scan_node(pgdat, scan_control, start_pfn,
					      end_pfn);
	}

	return scan_done;
}
#endif

void kstaled_free_folio_age(pg_data_t *pgdat)
{
	u8 *age;

	age = rcu_access_pointer(pgdat->node_page_age);
	if (age) {
		rcu_assign_pointer(pgdat->node_page_age, NULL);
		synchronize_rcu();
		apply_to_existing_page_range(&init_mm, (unsigned long)age,
					     PAGE_ALIGN(pgdat->node_spanned_pages),
					     kstaled_free_pte, NULL);
		free_vm_area(find_vm_area(age));
	}
}

static inline void kstaled_scan_done(struct kstaled_scan_control scan_control)
{
	kstaled_mem_cgroup_scan_done(scan_control);
	kstaled_scan_rounds++;
}

static void kstaled_reset(bool free)
{
	pg_data_t *pgdat;

	kstaled_mem_cgroup_reset(SCAN_TARGET_PAGE);

	get_online_mems();
	for_each_online_pgdat(pgdat) {
		if (!pgdat->node_page_age)
			continue;

		if (free)
			kstaled_free_folio_age(pgdat);
		else {
			apply_to_existing_page_range(&init_mm,
					    (unsigned long)pgdat->node_page_age,
					    PAGE_ALIGN(pgdat->node_spanned_pages),
					    kstaled_reset_pte, NULL);
		}

		cond_resched();
	}
	put_online_mems();
}

static inline bool kstaled_should_run(struct kstaled_scan_control *p, bool *new)
{
	if (unlikely(!kstaled_is_scan_period_equal(p))) {
		struct kstaled_scan_control scan_control;

		scan_control  = kstaled_get_current_scan_control();
		if (p->duration)
			kstaled_reset(!scan_control.duration);

		if (!scan_control.duration)
			static_branch_disable(&kstaled_enabled_key);

		*p = scan_control;
		*new = true;
	} else
		*new = false;

	if (p->duration > 0)
		return true;

	return false;
}

static inline bool is_kstaled_scan_done(bool scan_done,
				       struct kstaled_scan_control scan_control)
{
	if (kstaled_has_page_target_only(&scan_control))
		return scan_done;

	return 0;
}

static int kstaled(void *dummy)
{
	int busy_loop = 0;
	bool restart = true;
	struct kstaled_scan_control scan_control;

	kstaled_reset_scan_control(&scan_control);

	while (!kthread_should_stop()) {
		u64 start_jiffies, elapsed;
		bool new, scan_done = true;

		wait_event_interruptible(kstaled_wait,
					 kstaled_should_run(&scan_control, &new));
		if (unlikely(new)) {
			restart = true;
			busy_loop = 0;
		}

		if (unlikely(scan_control.duration == 0))
			continue;

		start_jiffies = jiffies_64;
		get_online_mems();
		scan_done = kstaled_scan_nodes(scan_control, restart);
		put_online_mems();

		if (is_kstaled_scan_done(scan_done, scan_control)) {
			kstaled_scan_done(scan_control);
			restart = true;
		} else {
			restart = false;
		}

		/*
		 * This design of emergency throttle was borrowed from
		 * Michel Lespinasse and Alibaba's patch. And we also
		 * set the scheduler policy of kstaled as SCHED_IDLE.
		 *
		 * Ideally, kstaled can scan a fixed number of pages which
		 * calculated by scan_control in each slice, likes:
		 *
		 *	pages_to_scan = total_pages / scan_duration
		 *	for_each_slice() {
		 *		start_jiffies = jiffies_64;
		 *		scan_pages(pages_to_scan);
		 *		elapsed = jiffies_64 - start_jiffies;
		 *		sleep(HZ - elapsed);
		 *	}
		 *
		 * Moreover, kstaled defines the BUSY when elapsed >= (HZ / 2)
		 * and track busy loops using a token bucket, such as
		 * If elapsed >= (HZ / 2), increment the busy_loop counter.
		 * If elapsed < (HZ / 2), down to 0.
		 * If the counter reaches KSTALED_BUSY_LOOP_THRESHOLD, we
		 * scale up the scan duration to relax the CPU and back off
		 * for a fixed HZ (1 second).
		 *
		 * Literally, kstaled is the lowest priority, and it can be
		 * scheduled easily when other task want to run in it's cpu.
		 */
#define KSTALED_BUSY_RUNNING		(HZ / 2)
#define KSTALED_BUSY_LOOP_THRESHOLD	10
		elapsed = jiffies_64 - start_jiffies;
		if (elapsed < KSTALED_BUSY_RUNNING) {
			busy_loop = 0;
			schedule_timeout_interruptible(HZ - elapsed);
		} else if (++busy_loop >= KSTALED_BUSY_LOOP_THRESHOLD) {
			busy_loop = 0;
			if (kstaled_try_double_scan_control(scan_control)) {
				pr_warn_ratelimited("%s: period -> %u\n",
					__func__,
					kstaled_get_current_scan_duration());
			}

			/*
			 * sleep fixed time to relax cpu and avoid unstable
			 * extreme backoff.
			 */
			schedule_timeout_interruptible(HZ);
		} else {
			elapsed = (elapsed < HZ) ? (HZ - elapsed) : HZ;
			schedule_timeout_interruptible(elapsed);
		}
	}

	return 0;
}

static ssize_t kstaled_scan_period_show(struct kobject *kobj,
				       struct kobj_attribute *attr,
				       char *buf)
{
	return sprintf(buf, "%u\n", kstaled_get_current_scan_duration());
}

/*
 * We will update the real scan period and do reset asynchronously,
 * avoid stall when kstaled is busy waiting for other resources.
 */
static ssize_t kstaled_scan_period_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long secs;
	int ret;

	ret = kstrtoul(buf, 10, &secs);
	if (ret || secs > KSTALED_MAX_SCAN_DURATION)
		return -EINVAL;

	/*
	 * To avoid situation like lru_gen >= 0 && kstaled disabled, disable
	 * enabled_key after reset.
	 */
	if (secs)
		static_branch_enable(&kstaled_enabled_key);

	kstaled_set_scan_duration(secs);
	wake_up_interruptible(&kstaled_wait);
	return count;
}

static struct kobj_attribute kstaled_scan_period_attr =
	__ATTR(scan_period_in_seconds, 0644,
	       kstaled_scan_period_show, kstaled_scan_period_store);

static struct attribute *kstaled_attrs[] = {
	&kstaled_scan_period_attr.attr,
	NULL
};
static struct attribute_group kstaled_attr_group = {
	.name = "kstaled",
	.attrs = kstaled_attrs,
};

static int __init kstaled_init(void)
{
	struct task_struct *thread;
	struct sched_param param = { .sched_priority = 0 };
	int ret;

	ret = sysfs_create_group(mm_kobj, &kstaled_attr_group);
	if (ret) {
		pr_warn("%s: Error %d on creating sysfs files\n",
		       __func__, ret);
		return ret;
	}

	thread = kthread_run(kstaled, NULL, "kstaled");
	if (IS_ERR(thread)) {
		sysfs_remove_group(mm_kobj, &kstaled_attr_group);
		pr_warn("%s: Failed to start kthread\n", __func__);
		return PTR_ERR(thread);
	}

	/* Make kstaled as nice as possible. */
	sched_setscheduler(thread, SCHED_IDLE, &param);

	return 0;
}

module_init(kstaled_init);
