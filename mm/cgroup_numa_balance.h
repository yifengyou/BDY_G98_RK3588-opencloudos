/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _MM_CGROUP_NUMA_BALANCE_H
#define _MM_CGROUP_NUMA_BALANCE_H

#ifdef CONFIG_CGROUP_NUMA_BALANCE

#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/sched.h>
#include <linux/sched/sysctl.h>

/* Default scan period: 60 seconds */
#define CG_NUMA_BALANCE_SCAN_PERIOD_MS_DEF	60000
#define CG_NUMA_BALANCE_SCAN_PERIOD_MS_MIN	1000
#define CG_NUMA_BALANCE_SCAN_PERIOD_MS_MAX	3600000

/* Default scan batch: 256 MB worth of pages */
#define CG_NUMA_BALANCE_SCAN_BATCH_MB_DEF	256
#define CG_NUMA_BALANCE_SCAN_BATCH_MB_MIN	1
#define CG_NUMA_BALANCE_SCAN_BATCH_MB_MAX	4096

struct cg_numa_scan_ctx {
	struct mem_cgroup	*memcg;
	unsigned long		batch_pages;	/* remaining pages to scan this round */
	unsigned long		pages_scanned;
};

extern struct static_key_false cgroup_numa_balance_enabled;

static inline bool numa_balance_is_cgroup_mode(void)
{
	return !!(sysctl_numa_balancing_mode & NUMA_BALANCING_CGROUP);
}

int cgroup_numa_misplaced(struct folio *folio, struct vm_area_struct *vma, unsigned long addr);
void cgroup_numa_balance_work_fn(struct work_struct *work);
void cgroup_numa_balance_enable(struct mem_cgroup *memcg);
void cgroup_numa_balance_disable(struct mem_cgroup *memcg);
void cgroup_numa_balance_cancel_all(void);
void cgroup_numa_balance_resume_all(void);

extern bool cpuset_cgroup_expected_nodes(struct cgroup *cgrp, nodemask_t *nodes);

extern int cgroup_numa_balance_scan_ctrl_show(struct seq_file *m, void *v);
extern ssize_t cgroup_numa_balance_scan_ctrl_write(struct kernfs_open_file *of,
						   char *buf, size_t nbytes,
						   loff_t off);
extern int cgroup_numa_balance_stat_show(struct seq_file *m, void *v);

#else
static inline bool numa_balance_is_cgroup_mode(void)
{
	return false;
}
#endif /* CONFIG_CGROUP_NUMA_BALANCE */
#endif /* _MM_CGROUP_NUMA_BALANCE_H */
