/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PSI_H
#define _LINUX_PSI_H

#include <linux/jump_label.h>
#include <linux/psi_types.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/cgroup-defs.h>
#include <linux/cgroup.h>
#include <linux/sched/clock.h>

struct seq_file;
struct css_set;

#ifdef CONFIG_PSI

extern struct static_key_false psi_disabled;
extern struct static_key_true psi_cgroups_enabled;
extern struct psi_group psi_system;
#ifdef CONFIG_PSI_DYN_SWITCH
extern unsigned int sysctl_psi_dyn_stat_types;
#ifdef CONFIG_CGROUPS
extern unsigned int sysctl_psi_cgroup_default_enabled;
#endif
extern struct static_key_false dyn_cpu_enabled;

static inline bool psi_dyn_stat_cpu(void)
{
	return static_branch_unlikely(&dyn_cpu_enabled);
}
int psi_dyn_stat_handler(struct ctl_table *table, int write, void *buffer,
		size_t *lenp, loff_t *ppos);
#else
static inline bool psi_dyn_stat_cpu(void)
{
	return true;
}
#endif

/*
 * There is no need to use sched_clock() that is very expensive to
 * record time for psi, because of two reasons: First, psi only needs to
 * record the time interval between two events, which makes the usage of
 * INITIAL_JIFFIES meaning less. Second, if only we use same units for
 * all the values we recorded, we can do the time interval calculation
 * without any conversion. And we can convert the jiffies to nanoseconds
 * when we are about to print out the time in nanosecond. This approach
 * minimizes the overhead of time conversion in the critical path of psi
 * accounting.
 */
static inline unsigned long psi_gettime(int cpu)
{
#ifdef CONFIG_PSI_USE_JIFFIES
	return jiffies;
#else
	return cpu_clock(cpu);
#endif
}

void psi_init(void);

void psi_memstall_enter(unsigned long *flags);
void psi_memstall_leave(unsigned long *flags);

int psi_show(struct seq_file *s, struct psi_group *group, enum psi_res res);
struct psi_trigger *psi_trigger_create(struct psi_group *group, char *buf,
				       enum psi_res res, struct file *file,
				       struct kernfs_open_file *of);
void psi_trigger_destroy(struct psi_trigger *t);

__poll_t psi_trigger_poll(void **trigger_ptr, struct file *file,
			poll_table *wait);

#ifdef CONFIG_CGROUPS
static inline struct psi_group *cgroup_psi(struct cgroup *cgrp)
{
	return cgrp->psi;
}

int psi_cgroup_alloc(struct cgroup *cgrp);
void psi_cgroup_free(struct cgroup *cgrp);
void cgroup_move_task(struct task_struct *p, struct css_set *to);
void psi_cgroup_restart(struct psi_group *group);

int cgroup_io_pressure_show(struct seq_file *seq, void *v);
int cgroup_memory_pressure_show(struct seq_file *seq, void *v);
int cgroup_cpu_pressure_show(struct seq_file *seq, void *v);
ssize_t cgroup_io_pressure_write(struct kernfs_open_file *of, char *buf, size_t nbytes, loff_t off);
ssize_t cgroup_memory_pressure_write(struct kernfs_open_file *of, char *buf, size_t nbytes, loff_t off);
ssize_t cgroup_cpu_pressure_write(struct kernfs_open_file *of, char *buf, size_t nbytes, loff_t off);
void cgroup_pressure_release(struct kernfs_open_file *of);
__poll_t cgroup_pressure_poll(struct kernfs_open_file *of, struct poll_table_struct *pt);
#endif

#else /* CONFIG_PSI */

static inline void psi_init(void) {}

static inline void psi_memstall_enter(unsigned long *flags) {}
static inline void psi_memstall_leave(unsigned long *flags) {}

#ifdef CONFIG_CGROUPS
static inline int psi_cgroup_alloc(struct cgroup *cgrp)
{
	return 0;
}
static inline void psi_cgroup_free(struct cgroup *cgrp)
{
}
static inline void cgroup_move_task(struct task_struct *p, struct css_set *to)
{
	rcu_assign_pointer(p->cgroups, to);
}
static inline void psi_cgroup_restart(struct psi_group *group) {}
#endif

#endif /* CONFIG_PSI */

#endif /* _LINUX_PSI_H */
