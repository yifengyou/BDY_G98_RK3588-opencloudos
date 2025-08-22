/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_SYSCTL_H
#define _LINUX_SCHED_SYSCTL_H

#include <linux/types.h>

#ifdef CONFIG_DETECT_HUNG_TASK
/* used for hung_task and block/ */
extern unsigned long sysctl_hung_task_timeout_secs;
#else
/* Avoid need for ifdefs elsewhere in the code */
enum { sysctl_hung_task_timeout_secs = 0 };
#endif

#ifdef CONFIG_PID_NS
extern unsigned int sysctl_watch_host_pid;
#endif

#ifdef CONFIG_IDLE_REVERT
extern unsigned int sysctl_sched_idle_revert_min;
extern unsigned int sysctl_tg_idle_revert_enabled;
extern unsigned int sysctl_tg_idle_revert_scan_count;
#endif

#ifdef CONFIG_SMP
extern int sysctl_smt_util_ratio;
#endif

enum sched_tunable_scaling {
	SCHED_TUNABLESCALING_NONE,
	SCHED_TUNABLESCALING_LOG,
	SCHED_TUNABLESCALING_LINEAR,
	SCHED_TUNABLESCALING_END,
};

#define NUMA_BALANCING_DISABLED		0x0
#define NUMA_BALANCING_NORMAL		0x1
#define NUMA_BALANCING_MEMORY_TIERING	0x2

#ifdef CONFIG_BT_SCHED
int bt_ignore_cpubind_handler(struct ctl_table *table, int write,
					  void *buffer, size_t *lenp,
					  loff_t *ppos);
extern unsigned int sysctl_sched_bt_nr_migrate;
extern unsigned int sysctl_idle_balance_bt_cost;
extern unsigned int sysctl_sched_bt_load_balance_interval_min_ms;
extern unsigned int sysctl_sched_bt_load_balance_interval_max_ms;
extern int sched_bt_disable_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos);
extern unsigned int sysctl_cpu_qos;
extern unsigned int sysctl_sched_bt_ignore_cpubind;
extern unsigned int sysctl_sched_bt_iowait;
#ifdef CONFIG_BT_BANDWIDTH
extern unsigned int sysctl_sched_bt_percpu_suppress_percent;
extern unsigned int sysctl_sched_bt_percpu_max_throttle_time_sec;
#endif
extern unsigned int sysctl_rue_reserved0;
extern unsigned int sysctl_rue_reserved1;
extern unsigned int sysctl_rue_reserved2;
extern unsigned int sysctl_rue_reserved3;
extern unsigned int sysctl_rue_reserved4;
extern unsigned int sysctl_rue_reserved5;
#endif

#ifdef CONFIG_BT_BANDWIDTH
extern unsigned int sysctl_sched_bt_period;
extern int sched_bt_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp,
		loff_t *ppos);
extern int sched_bt_percpu_suppress_percent_handler(struct ctl_table *table,
		int write, void __user *buffer, size_t *lenp,
		loff_t *ppos);
#endif

#ifdef CONFIG_NUMA_BALANCING
extern int sysctl_numa_balancing_mode;
#else
#define sysctl_numa_balancing_mode	0
#endif

#ifdef CONFIG_SCHED_CLUSTER
extern unsigned int sysctl_sched_cluster;
int sched_cluster_handler(struct ctl_table *table, int write,
			  void *buffer, size_t *lenp, loff_t *ppos);
#endif

#ifdef CONFIG_SCHED_CLASS_EXT
extern int sysctl_debug_scx_stall;
extern unsigned int sysctl_scx_ignore_cpubind;
extern int sysctl_scx_print_exit_kill;

extern int scx_ignore_cpubind_handler(struct ctl_table *table, int write,
		void *buffer, size_t *lenp, loff_t *ppos);
#endif

#endif /* _LINUX_SCHED_SYSCTL_H */
