/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_STAT_H
#define _LINUX_SCHED_STAT_H

#include <linux/percpu.h>
#include <linux/kconfig.h>
#include <linux/sched/sysctl.h>

/*
 * Various counters maintained by the scheduler and fork(),
 * exposed via /proc, sys.c or used by drivers via these APIs.
 *
 * ( Note that all these values are acquired without locking,
 *   so they can only be relied on in narrow circumstances. )
 */

DECLARE_PER_CPU(unsigned long, total_forks);
extern int nr_threads;
DECLARE_PER_CPU(unsigned long, process_counts);
extern unsigned long nr_forks(void);
extern int nr_processes(void);
extern unsigned int nr_running(void);
extern unsigned long nr_running_cpu(int cpu);
extern bool single_task_running(void);
extern unsigned int nr_iowait(void);
extern unsigned int nr_iowait_cpu(int cpu);

#ifdef CONFIG_BT_SCHED
static inline int sched_bt_enabled(void) { return sysctl_cpu_qos; }
extern unsigned long nr_iowait_bt(void);
extern unsigned long nr_iowait_bt_cpu(int cpu);
extern void sched_bt_killall(void);
static inline bool should_account_iowait_bt(int cpu)
{
	return nr_iowait_bt_cpu(cpu) > 0;
}
static inline u64 iowait_bt_cputime(int cpu, u64 cputime)
{
	u64 nr_iowait = nr_iowait_cpu(cpu);

	return nr_iowait ? div_u64(cputime *
			nr_iowait_bt_cpu(cpu), nr_iowait) : 0;
}
#else
static inline int sched_bt_enabled(void) { return 0; }
static inline void sched_bt_killall(void) { }
#endif

static inline int sched_info_on(void)
{
	return IS_ENABLED(CONFIG_SCHED_INFO);
}

#ifdef CONFIG_SCHEDSTATS
void force_schedstat_enabled(void);
#endif

#endif /* _LINUX_SCHED_STAT_H */
