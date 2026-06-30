// SPDX-License-Identifier: GPL-2.0
/*
 * Cgroup-level NUMA Memory Balancing
 *
 * This implements proactive, periodic scanning of processes within a
 * memory cgroup to detect NUMA page placement mismatches and batch-migrate
 * misplaced pages to the correct NUMA nodes.
 *
 * Controlled via:
 *   - Global: sysctl kernel.numa_balancing (value 4 = cgroup mode)
 *   - Per-cgroup: memory.numa_balance.scan_ctrl / memory.numa_balance.stat
 *
 * Copyright (C) 2026
 */

#include <linux/mm.h>
#include <linux/huge_mm.h>
#include <linux/mm_types.h>
#include <linux/mm_inline.h>
#include <linux/migrate.h>
#include <linux/sched/mm.h>
#include <linux/sched/cputime.h>
#include <linux/mempolicy.h>
#include <linux/nodemask.h>
#include <linux/cpuset.h>
#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/hugetlb.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/vmstat.h>
#include <linux/page-isolation.h>
#include <asm/tlbflush.h>

#include "cgroup_numa_balance.h"
#include "internal.h"

#define CREATE_TRACE_POINTS
#include <trace/events/cgroup_numa_balance.h>

/* Forward declarations from kernel/sched/sched.h */
#include <linux/jump_label.h>
extern struct static_key_false cgroup_numa_balance_enabled;
extern int sysctl_numa_balancing_mode;

/*
 * Determine the expected NUMA node(s) for a given task.
 *
 * Priority:
 *   1. cpuset allowed CPUs → corresponding NUMA nodes
 *   2. mempolicy (MPOL_BIND/MPOL_PREFERRED)
 *   3. Current CPU's NUMA node
 */
static void get_expected_numa_nodes(struct task_struct *task,
				    struct mempolicy *pol,
				    nodemask_t *nodes)
{
	nodemask_t mems_allowed, cpu_nodes;

	nodes_clear(*nodes);
	nodes_clear(cpu_nodes);

	/* Physical hard limit: task's allowed memory nodes (e.g. cpuset.mems) */
	mems_allowed = task->mems_allowed;

	/* Optimal nodes from allowed CPUs */
	if (task->cpus_ptr) {
		int node;

		for_each_node_state(node, N_MEMORY) {
			if (cpumask_intersects(task->cpus_ptr, cpumask_of_node(node)))
				node_set(node, cpu_nodes);
		}
	}

	/* Intersect soft and hard limits */
	nodes_and(*nodes, cpu_nodes, mems_allowed);
	if (nodes_empty(*nodes))
		*nodes = mems_allowed;

	/*
	 * Apply user-defined memory policy to further narrow down
	 * the expected nodes.
	 */
	if (pol) {
		nodemask_t pol_nodes;

		nodes_clear(pol_nodes);
		if (pol->mode == MPOL_BIND || pol->mode == MPOL_INTERLEAVE ||
		    pol->mode == MPOL_PREFERRED_MANY || pol->mode == MPOL_PREFERRED) {
			pol_nodes = pol->nodes;
		} else if (pol->mode == MPOL_LOCAL) {
			node_set(cpu_to_node(task_cpu(task)), pol_nodes);
		}

		if (!nodes_empty(pol_nodes)) {
			nodemask_t intersect;

			nodes_and(intersect, *nodes, pol_nodes);
			if (!nodes_empty(intersect))
				*nodes = intersect;
		}
	}
}

static void task_expected_numa_nodes(struct task_struct *task,
				     nodemask_t *nodes)
{
	task_lock(task);
	get_expected_numa_nodes(task, get_task_policy(task), nodes);
	task_unlock(task);
}

static void vma_expected_numa_nodes(struct vm_area_struct *vma,
				    struct task_struct *task,
				    nodemask_t *nodes)
{
	struct mempolicy *pol = vma_policy(vma);

	if (pol)
		get_expected_numa_nodes(task, pol, nodes);
	else
		task_expected_numa_nodes(task, nodes);
}

/*
 * Select the best target node from the allowed set.
 * Currently picks the first node in mask; can be extended
 * to consider node load/free memory.
 */
static int select_target_nid(const nodemask_t *nodes)
{
	int nid;

	for_each_node_mask(nid, *nodes) {
		if (node_state(nid, N_MEMORY))
			return nid;
	}
	return NUMA_NO_NODE;
}

int cgroup_numa_misplaced(struct folio *folio, struct vm_area_struct *vma,
			  unsigned long addr)
{
	nodemask_t expected_nodes;
	int curnid = folio_nid(folio);

	vma_expected_numa_nodes(vma, current, &expected_nodes);
	if (node_isset(curnid, expected_nodes))
		return NUMA_NO_NODE;

	return select_target_nid(&expected_nodes);
}

#define CG_NUMA_BUSY_RUNNING		(HZ / 2)

#ifdef CONFIG_LRU_GEN
/*
 * Scan MGLRU folios for a memcg and isolate unmapped file pages
 * that are on nodes not present in expected_nodes.
 */
static void cgroup_numa_balance_scan_file_mglru(struct mem_cgroup *memcg,
						nodemask_t *expected_nodes)
{
	unsigned long batch_pages = memcg->nb_scan_batch;
	unsigned long pages_scanned = 0;
	int nid;
	LIST_HEAD(isolate_list);
	u64 start_jiffies, elapsed;
	unsigned long nr_reclaimed = 0;

	if (nodes_empty(*expected_nodes))
		return;

	start_jiffies = jiffies_64;
	for_each_node_state(nid, N_MEMORY) {
		struct lruvec *lruvec;
		struct folio *folio, *next;
		struct lru_gen_folio *lrugen;
		int old_gen, zone;
		int type = LRU_GEN_FILE;

		/* Skip nodes that are in the allowed expected_nodes */
		if (node_isset(nid, *expected_nodes))
			continue;

		lruvec = mem_cgroup_lruvec(memcg, NODE_DATA(nid));
		lrugen = &lruvec->lrugen;

		spin_lock_irq(&lruvec->lru_lock);

		old_gen = lru_gen_from_seq(READ_ONCE(lrugen->min_seq[type]));
		for (zone = 0; zone < MAX_NR_ZONES; zone++) {
			list_for_each_entry_safe_reverse(folio, next,
					&lrugen->folios[old_gen][type][zone], lru) {
				unsigned long nr_pages;

				if (batch_pages == 0)
					break;

				nr_pages = folio_nr_pages(folio);

				/*
				 * Only process unmapped file pages.
				 * If mapped or unable to get, skip it
				 * without changing its position.
				 */
				if (folio_mapped(folio) || !folio_try_get(folio)) {
					pages_scanned += nr_pages;
					batch_pages -= min(batch_pages, nr_pages);
					continue;
				}

				if (!folio_test_clear_lru(folio)) {
					folio_put(folio);
					pages_scanned += nr_pages;
					batch_pages -= min(batch_pages, nr_pages);
					continue;
				}

				lruvec_del_folio(lruvec, folio);
				list_add(&folio->lru, &isolate_list);

				pages_scanned += nr_pages;
				batch_pages -= min(batch_pages, nr_pages);
			}
			if (batch_pages == 0)
				break;
		}
		spin_unlock_irq(&lruvec->lru_lock);

		elapsed = jiffies_64 - start_jiffies;
		if (batch_pages == 0 || elapsed >= CG_NUMA_BUSY_RUNNING)
			break;
	}

	if (!list_empty(&isolate_list)) {
		/*
		 * The un-reclaimed pages are put back to the LRU by
		 * reclaim_pages() automatically.
		 */
		nr_reclaimed = reclaim_pages(&isolate_list, false);
		atomic64_add(nr_reclaimed, &memcg->nb_pages_migrated);
	}

	atomic64_add(pages_scanned, &memcg->nb_pages_scanned);
	trace_cgroup_numa_balance_scan_file_mglru(pages_scanned, nr_reclaimed);
}
#else
static inline void cgroup_numa_balance_scan_file_mglru(struct mem_cgroup *memcg,
						nodemask_t *expected_nodes)
{
}
#endif

/*
 * Scan LRU_INACTIVE_FILE for a memcg and isolate unmapped file pages
 * that are on nodes not present in expected_nodes.
 */
static void cgroup_numa_balance_scan_file_lru(struct mem_cgroup *memcg,
					      nodemask_t *expected_nodes)
{
	unsigned long batch_pages = memcg->nb_scan_batch;
	unsigned long pages_scanned = 0;
	int nid;
	LIST_HEAD(isolate_list);
	u64 elapsed;
	unsigned long nr_reclaimed = 0;
	u64 start_jiffies;

	if (nodes_empty(*expected_nodes))
		return;

	start_jiffies = jiffies_64;
	for_each_node_state(nid, N_MEMORY) {
		struct lruvec *lruvec;
		struct folio *folio, *next;

		/* Skip nodes that are in the allowed expected_nodes */
		if (node_isset(nid, *expected_nodes))
			continue;

		lruvec = mem_cgroup_lruvec(memcg, NODE_DATA(nid));

		spin_lock_irq(&lruvec->lru_lock);
		list_for_each_entry_safe_reverse(folio, next, &lruvec->lists[LRU_INACTIVE_FILE], lru) {
			unsigned long nr_pages;

			if (batch_pages == 0)
				break;

			nr_pages = folio_nr_pages(folio);
			/*
			 * Only process unmapped file pages.
			 * If mapped or unable to get, skip it without changing its position.
			 */
			if (folio_mapped(folio) || !folio_try_get(folio)) {
				pages_scanned += nr_pages;
				batch_pages -= min(batch_pages, nr_pages);
				continue;
			}

			if (!folio_test_clear_lru(folio)) {
				folio_put(folio);
				pages_scanned += nr_pages;
				batch_pages -= min(batch_pages, nr_pages);
				continue;
			}

			lruvec_del_folio(lruvec, folio);
			list_add(&folio->lru, &isolate_list);

			pages_scanned += nr_pages;
			batch_pages -= min(batch_pages, nr_pages);
		}
		spin_unlock_irq(&lruvec->lru_lock);

		elapsed = jiffies_64 - start_jiffies;
		if (batch_pages == 0 || elapsed >= CG_NUMA_BUSY_RUNNING)
			break;
	}

	if (!list_empty(&isolate_list)) {
		/*
		 * The un-reclaimed pages are put back to the LRU by
		 * reclaim_pages() automatically.
		 */
		nr_reclaimed = reclaim_pages(&isolate_list, false);
		atomic64_add(nr_reclaimed, &memcg->nb_pages_migrated);
	}

	atomic64_add(pages_scanned, &memcg->nb_pages_scanned);
	trace_cgroup_numa_balance_scan_file_lru(pages_scanned, nr_reclaimed);
}

static bool cgroup_numa_balance_scan_vma(struct vm_area_struct *vma,
		struct cg_numa_scan_ctx *ctx, unsigned long *mm_scan_offset,
		u64 start_jiffies)
{
	unsigned long start, end = 0;
	unsigned long nr_updated, nr_scanned, one_shot_size;
	u64 elapsed;
	bool timeout = false;

	if (ctx->batch_pages >= HPAGE_PMD_NR)
		one_shot_size = HPAGE_PMD_SIZE;
	else
		one_shot_size = ctx->batch_pages << PAGE_SHIFT;

	start = max(*mm_scan_offset, vma->vm_start);

	for (; start < vma->vm_end; start += one_shot_size) {
		end = min(vma->vm_end, start + one_shot_size);
		nr_scanned = (end - start) >> PAGE_SHIFT;
		nr_updated = change_prot_numa(vma, start, end);
		ctx->pages_scanned += nr_updated;
		if (ctx->batch_pages > nr_scanned)
			ctx->batch_pages -= nr_scanned;
		else
			ctx->batch_pages = 0;

		elapsed = jiffies_64 - start_jiffies;
		if (elapsed >= CG_NUMA_BUSY_RUNNING) {
			timeout = true;
			break;
		}
		if (ctx->batch_pages == 0)
			break;
	}
	*mm_scan_offset = end;
	return timeout;
}

static bool cgroup_numa_balance_scan_vmas(struct mem_cgroup *memcg,
			struct task_struct *task, struct mm_struct *mm,
			struct cg_numa_scan_ctx *ctx, u64 start_jiffies)
{
	struct vm_area_struct *vma = NULL;
	struct vma_iterator vmi;
	unsigned long mm_scan_offset;
	bool timeout = false;

	mmap_read_lock(mm);
	mm_scan_offset = mm->numa_scan_offset;
	vma_iter_init(&vmi, mm, mm_scan_offset);
	vma = vma_next(&vmi);
	if (!vma) {
		/*
		 * If vma is NULL here, it means the process was fully
		 * scanned in the previous invocation. Reset offset to
		 * 0 for the next round and switch to the next task.
		 */
		mm->numa_scan_offset = 0;
		mmap_read_unlock(mm);
		memcg->nb_last_scanned_pid = 0;
		return false;
	}

	for (; vma; vma = vma_next(&vmi)) {
		nodemask_t vma_nodes;

		if (ctx->batch_pages == 0)
			break;

		/* Skip non-migratable VMAs */
		if (!vma_migratable(vma) ||
		    is_vm_hugetlb_page(vma) ||
		    (vma->vm_flags & VM_MIXEDMAP)) {
			mm_scan_offset = vma->vm_end;
			continue;
		}

		/* Skip read-only file-backed mappings */
		if (vma->vm_file &&
		    (vma->vm_flags & (VM_READ | VM_WRITE)) == VM_READ) {
			mm_scan_offset = vma->vm_end;
			continue;
		}

		vma_expected_numa_nodes(vma, task, &vma_nodes);
		if (nodes_equal(vma_nodes, node_states[N_MEMORY])) {
			mm_scan_offset = vma->vm_end;
			continue;
		}

		timeout = cgroup_numa_balance_scan_vma(vma, ctx,
				&mm_scan_offset, start_jiffies);
		if (timeout)
			break;
	}

	/* Update the scan offset for the next round */
	if (vma || (ctx->batch_pages == 0 || timeout))
		/* Still keep task as last scanned if ran out of quota */
		mm->numa_scan_offset = mm_scan_offset;
	else {
		/* Switch to next task in the next iteration */
		mm->numa_scan_offset = 0;
		memcg->nb_last_scanned_pid = 0;
	}
	mmap_read_unlock(mm);

	if (ctx->batch_pages == 0)
		timeout = true;

	return timeout;

}

/*
 * Main scan function: iterate all tasks in the memcg and scan their VMAs.
 */
static bool cgroup_numa_balance_scan(struct mem_cgroup *memcg)
{
	struct cg_numa_scan_ctx ctx;
	struct task_struct *task = NULL;
	struct css_task_iter it;
	struct mm_struct *mm;
	nodemask_t file_expected_nodes;
	bool has_tasks = false;
	bool timeout = false;
	bool round_completed = false;
	bool resume_found = (memcg->nb_last_scanned_pid == 0);

	nodes_clear(file_expected_nodes);

	memset(&ctx, 0, sizeof(ctx));
	ctx.memcg = memcg;
	ctx.batch_pages = memcg->nb_scan_batch;

	if (memcg->nb_scan_mapped_pages) {
		u64 start_jiffies = jiffies_64;

		css_task_iter_start(&memcg->css, CSS_TASK_ITER_PROCS, &it);
		while ((task = css_task_iter_next(&it))) {
			if (!resume_found) {
				if (task->pid != memcg->nb_last_scanned_pid)
					continue;
				resume_found = true;
			}

			/* Skip kernel threads and exiting tasks */
			if (task->flags & (PF_KTHREAD | PF_EXITING))
				continue;

			if (memcg->nb_last_scanned_pid == 0 ||
			    memcg->nb_last_scanned_pid != task->pid)
				memcg->nb_last_scanned_pid = task->pid;

			mm = get_task_mm(task);
			if (!mm)
				continue;
			timeout = cgroup_numa_balance_scan_vmas(memcg, task, mm,
						&ctx, start_jiffies);
			if (timeout) {
				mmput(mm);
				break;
			}
			mmput(mm);
		}
		css_task_iter_end(&it);
		trace_cgroup_numa_balance_scan_vma(ctx.pages_scanned);

		if (!task)
			memcg->nb_last_scanned_pid = 0;
	} else {
		struct task_struct *iter_task;

		css_task_iter_start(&memcg->css, CSS_TASK_ITER_PROCS, &it);
		while ((iter_task = css_task_iter_next(&it))) {
			nodemask_t task_nodes;

			has_tasks = true;
			task_expected_numa_nodes(iter_task, &task_nodes);
			nodes_or(file_expected_nodes, file_expected_nodes, task_nodes);
			if (nodes_equal(file_expected_nodes, node_states[N_MEMORY]))
				break;
		}
		css_task_iter_end(&it);

		if (!has_tasks && cgroup_subsys_on_dfl(memory_cgrp_subsys))
			cpuset_cgroup_expected_nodes(memcg->css.cgroup,
						     &file_expected_nodes);

		if (!nodes_empty(file_expected_nodes)) {
			if (lru_gen_enabled())
				cgroup_numa_balance_scan_file_mglru(memcg,
						&file_expected_nodes);
			else
				cgroup_numa_balance_scan_file_lru(memcg,
						&file_expected_nodes);
		}
	}

	/*
	 * A round is considered complete when the VMA list is fully scanned
	 * (nb_last_scanned_pid == 0) AND we have just finished a File LRU
	 * scan (mapped_pages toggled back to true). This guarantees that
	 * every round includes exactly 1 full VMA scan and at least 1 File
	 * LRU scan.
	 */
	if (!memcg->nb_scan_mapped_pages &&
	    memcg->nb_last_scanned_pid == 0) {
		atomic64_inc(&memcg->nb_scan_rounds);
		round_completed = true;
	}
	memcg->nb_scan_mapped_pages = !memcg->nb_scan_mapped_pages;

	atomic64_add(ctx.pages_scanned, &memcg->nb_pages_scanned);
	memcg->nb_last_scan_jiffies = jiffies;
	return round_completed;
}

/*
 * Delayed work callback for periodic scanning.
 */
void cgroup_numa_balance_work_fn(struct work_struct *work)
{
	struct mem_cgroup *memcg;
	struct delayed_work *dwork;
	int rounds_left;

	dwork = to_delayed_work(work);
	memcg = container_of(dwork, struct mem_cgroup, numa_balance_work);

	/*
	 * Check both global and per-cgroup switches.
	 * The global check uses the static key set by sysctl.
	 */
	if (!static_branch_likely(&cgroup_numa_balance_enabled))
		return;

	if (!READ_ONCE(memcg->numa_balance_enabled))
		return;

	/* Check if the cgroup is still online */
	if (!css_tryget_online(&memcg->css))
		return;

	rounds_left = READ_ONCE(memcg->nb_scan_rounds_left);

	if (rounds_left > 0 || rounds_left == -1) {
		bool round_completed = false;

		round_completed = cgroup_numa_balance_scan(memcg);
		/* Decrement the counter if it's not infinite (-1) */
		if (round_completed && rounds_left > 0) {
			rounds_left--;

			WRITE_ONCE(memcg->nb_scan_rounds_left, rounds_left);
		}
	}

	/* Reschedule for next period if we still have rounds left (or -1) */
	if (READ_ONCE(memcg->numa_balance_enabled) &&
	    static_branch_likely(&cgroup_numa_balance_enabled)) {
		if (rounds_left > 0 || rounds_left == -1) {
			unsigned long delay;

			delay = msecs_to_jiffies(memcg->nb_scan_period_ms);
			if (delay > 0)
				queue_delayed_work(system_unbound_wq,
						   &memcg->numa_balance_work, delay);
		}
	}

	css_put(&memcg->css);
}

/*
 * Enable cgroup NUMA balance for a memcg.
 * Schedules the first scan immediately.
 */
void cgroup_numa_balance_enable(struct mem_cgroup *memcg)
{
	if (!static_branch_likely(&cgroup_numa_balance_enabled))
		return;

	WRITE_ONCE(memcg->numa_balance_enabled, true);

	/* Schedule first scan immediately */
	queue_delayed_work(system_unbound_wq,
			   &memcg->numa_balance_work, 0);
}

/*
 * Disable cgroup NUMA balance for a memcg.
 * Cancels any pending scan work.
 */
void cgroup_numa_balance_disable(struct mem_cgroup *memcg)
{
	WRITE_ONCE(memcg->numa_balance_enabled, false);
	cancel_delayed_work_sync(&memcg->numa_balance_work);
}

/*
 * Cancel all cgroup NUMA balance work across all memcgs.
 * Called when global sysctl switches away from cgroup mode.
 */
void cgroup_numa_balance_cancel_all(void)
{
	struct mem_cgroup *memcg;

	/* Iterate all memcgs and cancel their work */
	for (memcg = mem_cgroup_iter(NULL, NULL, NULL);
	     memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {
		cancel_delayed_work_sync(&memcg->numa_balance_work);
	}
}

/*
 * Resume all cgroup NUMA balance work across all enabled memcgs.
 * Called when global sysctl switches back to cgroup mode.
 */
void cgroup_numa_balance_resume_all(void)
{
	struct mem_cgroup *memcg;

	/* Iterate all memcgs and resume their work if enabled */
	for (memcg = mem_cgroup_iter(NULL, NULL, NULL);
	     memcg != NULL;
	     memcg = mem_cgroup_iter(NULL, memcg, NULL)) {
		if (READ_ONCE(memcg->numa_balance_enabled)) {
			int rounds_left = READ_ONCE(memcg->nb_scan_rounds_left);

			if (rounds_left != 0) {
				unsigned long delay;

				if (!css_tryget_online(&memcg->css))
					continue;

				delay = msecs_to_jiffies(memcg->nb_scan_period_ms);
				if (delay == 0)
					delay = 1;
				queue_delayed_work(system_unbound_wq,
						   &memcg->numa_balance_work, delay);
				css_put(&memcg->css);
			}
		}
	}
}

/*
 * Show callback for memory.numa_balance.scan_ctrl
 */
int cgroup_numa_balance_scan_ctrl_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_seq(m);
	int enabled;
	unsigned int period, batch;
	int rounds_left;

	if (!static_branch_likely(&cgroup_numa_balance_enabled)) {
		/*
		 * Cgroup mode is not globally active.
		 * If system-level NUMA balancing is on (sysctl=1/2/3),
		 * report 1 to indicate NUMA balancing is active system-wide.
		 */
		if (sysctl_numa_balancing_mode &
		    (NUMA_BALANCING_NORMAL | NUMA_BALANCING_MEMORY_TIERING))
			enabled = 1;
		else
			enabled = 0;
	} else {
		/* Cgroup mode active: return per-cgroup value */
		enabled = READ_ONCE(memcg->numa_balance_enabled) ? 1 : 0;
	}

	period = memcg->nb_scan_period_ms;
	batch = memcg->nb_scan_batch >> (20 - PAGE_SHIFT);
	rounds_left = READ_ONCE(memcg->nb_scan_rounds_left);

	seq_printf(m, "enabled=%d period_ms=%u batch_mb=%u rounds=%d\n",
		   enabled, period, batch, rounds_left);

	return 0;
}

/*
 * Write callback for memory.numa_balance.scan_ctrl
 *
 * Accepts a space/newline separated list of key=value pairs:
 *   enabled=1 period_ms=1000 batch_mb=256 rounds=-1
 */
ssize_t cgroup_numa_balance_scan_ctrl_write(struct kernfs_open_file *of,
					    char *buf, size_t nbytes,
					    loff_t off)
{
	struct mem_cgroup *memcg = mem_cgroup_from_css(of_css(of));
	char *opt, *val;
	ssize_t ret = nbytes;
	bool restart_worker = false;
	bool update_enabled = false, update_period = false;
	bool update_batch = false, update_rounds = false;
	int new_enabled = 0;
	unsigned int new_period = 0;
	unsigned int new_batch = 0;
	int new_rounds = 0;

	/* Only allow writes when cgroup mode is globally active */
	if (!static_branch_likely(&cgroup_numa_balance_enabled))
		return -EPERM;

	if (!css_tryget_online(&memcg->css))
		return -ENODEV;

	buf = strstrip(buf);
	while ((opt = strsep(&buf, " \n")) != NULL) {
		if (!*opt)
			continue;

		val = strchr(opt, '=');
		if (!val) {
			ret = -EINVAL;
			goto out;
		}
		*val++ = '\0';

		if (!strcmp(opt, "enabled")) {
			long v;

			if (kstrtol(val, 0, &v) || (v != 0 && v != 1)) {
				ret = -EINVAL;
				goto out;
			}
			new_enabled = v;
			update_enabled = true;
		} else if (!strcmp(opt, "period_ms")) {
			unsigned long v;

			if (kstrtoul(val, 0, &v) ||
			    v < CG_NUMA_BALANCE_SCAN_PERIOD_MS_MIN ||
			    v > CG_NUMA_BALANCE_SCAN_PERIOD_MS_MAX) {
				ret = -EINVAL;
				goto out;
			}
			new_period = (unsigned int)v;
			update_period = true;
		} else if (!strcmp(opt, "batch_mb")) {
			unsigned long v;
			unsigned int pages;

			if (kstrtoul(val, 0, &v) ||
			    v < CG_NUMA_BALANCE_SCAN_BATCH_MB_MIN ||
			    v > CG_NUMA_BALANCE_SCAN_BATCH_MB_MAX) {
				ret = -EINVAL;
				goto out;
			}
			/* Convert MB to pages */
			pages = (unsigned int)(v << (20 - PAGE_SHIFT));
			if (pages == 0)
				pages = 1;
			new_batch = pages;
			update_batch = true;
		} else if (!strcmp(opt, "rounds")) {
			int v;

			if (kstrtoint(val, 0, &v) || v < -1) {
				ret = -EINVAL;
				goto out;
			}
			new_rounds = v;
			update_rounds = true;
		} else {
			ret = -EINVAL;
			goto out;
		}
	}

	if (update_period)
		memcg->nb_scan_period_ms = new_period;

	if (update_batch)
		memcg->nb_scan_batch = new_batch;

	if (update_rounds) {
		WRITE_ONCE(memcg->nb_scan_rounds_left, new_rounds);
		restart_worker = true;
	}

	if (update_enabled) {
		if (new_enabled == 1 && !READ_ONCE(memcg->numa_balance_enabled)) {
			cgroup_numa_balance_enable(memcg);
		} else if (new_enabled == 0 && READ_ONCE(memcg->numa_balance_enabled)) {
			cgroup_numa_balance_disable(memcg);
		}
	}

	/* If the limit was updated, we may need to restart the worker */
	if (restart_worker && READ_ONCE(memcg->numa_balance_enabled) &&
	    static_branch_likely(&cgroup_numa_balance_enabled)) {
		int rounds_left = READ_ONCE(memcg->nb_scan_rounds_left);

		if (rounds_left != 0) {
			unsigned long delay;

			delay = msecs_to_jiffies(memcg->nb_scan_period_ms);
			if (delay == 0)
				delay = 1;
			queue_delayed_work(system_unbound_wq,
					   &memcg->numa_balance_work, delay);
		}
	}

out:
	css_put(&memcg->css);
	return ret;
}

/*
 * Stat file handler for memory.numa_balance.stat
 */
int cgroup_numa_balance_stat_show(struct seq_file *m, void *v)
{
	struct mem_cgroup *memcg = mem_cgroup_from_seq(m);
	long last_scan_ms_ago = 0;

	if (memcg->nb_last_scan_jiffies) {
		last_scan_ms_ago = jiffies_to_msecs(
			jiffies - memcg->nb_last_scan_jiffies);
	}

	seq_printf(m, "pages_scanned %llu\n",
		   (unsigned long long)atomic64_read(&memcg->nb_pages_scanned));
	seq_printf(m, "pages_migrated %llu\n",
		   (unsigned long long)atomic64_read(&memcg->nb_pages_migrated));
	seq_printf(m, "scan_rounds_completed %llu\n",
		   (unsigned long long)atomic64_read(&memcg->nb_scan_rounds));
	seq_printf(m, "scan_rounds_left %d\n", READ_ONCE(memcg->nb_scan_rounds_left));
	seq_printf(m, "last_scan_ms_ago %ld\n", last_scan_ms_ago);

	return 0;
}
