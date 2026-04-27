// SPDX-License-Identifier: GPL-2.0

#include <linux/mm.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <asm/pgalloc.h>
#include <linux/hugetlb.h>

DEFINE_STATIC_KEY_FALSE(async_fork_enabled_key);

/*
 * async fork in staging, 0 means the module is not loaded yet.
 */
atomic_t async_fork_staging;
EXPORT_SYMBOL_GPL(async_fork_staging);

struct async_fork_ops dummy_async_fork_ops = {
	.async_fork_prepare = NULL,
	.async_fork_mm_bind = NULL,
	.async_fork_fast = NULL,
	.async_fork_fast_done = NULL,
	.async_fork_rest = NULL,
	.async_fork_fixup_pmd = NULL,
	.async_fork_fixup_vma = NULL,
	.async_fork_fixup_vmas = NULL,
	.async_fork_madvise_vma = NULL,
};
EXPORT_SYMBOL_GPL(dummy_async_fork_ops);

struct async_fork_ops *async_fork_ops = &dummy_async_fork_ops;
EXPORT_SYMBOL_GPL(async_fork_ops);

int is_async_fork_task(struct task_struct *p)
{
	struct mem_cgroup *task_memcg;
	int async_fork = 0;

	if (likely(!async_fork_enabled()) || mem_cgroup_disabled())
		return 0;

	rcu_read_lock();
	task_memcg = mem_cgroup_from_task(p);
	if (task_memcg)
		async_fork = task_memcg->async_fork;
	rcu_read_unlock();

	if (!async_fork)
		return 0;

	return atomic_inc_not_zero(&async_fork_staging);
}

EXPORT_SYMBOL_GPL(__pte_alloc);
EXPORT_SYMBOL_GPL(__pmd_alloc);
EXPORT_SYMBOL_GPL(__pud_alloc);
EXPORT_SYMBOL_GPL(__p4d_alloc);
EXPORT_SYMBOL_GPL(pte_alloc_one);
EXPORT_SYMBOL_GPL(pmd_mkwrite);
EXPORT_SYMBOL_GPL(copy_hugetlb_page_range);
EXPORT_SYMBOL_GPL(__mmu_notifier_invalidate_range_start);
EXPORT_SYMBOL_GPL(__mmu_notifier_invalidate_range_end);
EXPORT_SYMBOL_GPL(track_pfn_copy);
EXPORT_SYMBOL_GPL(untrack_pfn_copy);
