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

struct async_fork_ops dummy_async_fork_ops = {0};
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
