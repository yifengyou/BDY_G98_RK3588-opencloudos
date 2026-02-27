/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * RQI (Resource Quality Indicator) - QoS monitor
 */
#ifndef _LINUX_RQI_H
#define _LINUX_RQI_H

#include <linux/types.h>

struct task_struct;
enum rqi_event_type {
	RQI_EVENT_CPU = 0,
	RQI_EVENT_MEM,
	RQI_EVENT_MAX,
};

enum rqi_event_id {
	/* Global stats - must be consecutive */
	RQI_GLOBAL_STAT_START = 0,               /* 0 */
	RQI_MEM_SHRINK_PAGES = RQI_GLOBAL_STAT_START, /* 0 */
	RQI_GLOBAL_STAT_END,					/* 1 */

	/* Per-CPU stats - must be consecutive */
	RQI_PERCPU_STAT_START = RQI_GLOBAL_STAT_END, /* 1 */
	RQI_MEM_LRUADD_FLUSH = RQI_PERCPU_STAT_START, /* 1 */
	RQI_PERCPU_STAT_END,					/* 2 */

	/* Per-node NUMA stats: local = alloc from this node by local CPU, other = by remote CPU */
	RQI_NUMA_STAT_START = RQI_PERCPU_STAT_END, /* 2 */
	RQI_NUMA_LOCAL = RQI_NUMA_STAT_START,  /* 2 */
	RQI_NUMA_OTHER,                        /* 3 */
	RQI_NUMA_STAT_END,                     /* 4 */

	/* DURATION type events - must be consecutive */
	RQI_DURATION_STAT_START = RQI_NUMA_STAT_END, /* 4 */

	/* CPU */
	RQI_CPU_RUNDELAY = RQI_DURATION_STAT_START, /* 4 */
	RQI_CPU_LONGSYS, /* 5 */

	/* MEM */
	RQI_MEM_SWAP_FAULT, /* 6 */
	RQI_MEM_DIRECT_RECLAIM, /* 7 */
	RQI_MEM_KSWAPD_RECLAIM, /* 8 */

	RQI_EVENT_ID_MAX,	/* 9 */
};

enum rqi_event_attr {
	RQI_ATTR_PERCPU = (1 << 0),	/* per-CPU event */
	RQI_ATTR_GLOBAL = (1 << 1),	/* global event */
	RQI_ATTR_PERDEV = (1 << 5),	/* per-device event (e.g. per-node for MEM) */
	RQI_ATTR_STAT = (1 << 2),	/* STAT event (using stat interface) */
	RQI_ATTR_DURATION = (1 << 3),	/* DURATION event (using start&end interface) */
	RQI_ATTR_END_IS_DELTA = (1 << 4),	/* rqi_end's second param is delta, not start */
};

/* Per-segment counts: all use (END - START). Event id is array index;
 * array size is RQI_EVENT_ID_MAX.
 */
#define RQI_GLOBAL_STAT_COUNT	(RQI_GLOBAL_STAT_END - RQI_GLOBAL_STAT_START) /* 1 */
#define RQI_PERCPU_STAT_COUNT	(RQI_PERCPU_STAT_END - RQI_PERCPU_STAT_START) /* 1 */
#define RQI_NUMA_STAT_COUNT	(RQI_NUMA_STAT_END - RQI_NUMA_STAT_START) /* 2 */
/* DURATION events - same pattern as NUMA_STAT */
#define RQI_DURATION_STAT_END	RQI_EVENT_ID_MAX
#define RQI_DURATION_COUNT	(RQI_DURATION_STAT_END - RQI_DURATION_STAT_START) /* 5 */

void rqi_stat(enum rqi_event_id event_id, u64 delta, void *arg);
u64 rqi_start(enum rqi_event_id event_id);
void rqi_end(enum rqi_event_id event_id, u64 value);
void rqi_end_task(enum rqi_event_id event_id, u64 value, struct task_struct *task);
/* RQI rundelay */
void rqi_rundelay(struct task_struct *prev, u64 delta_ns);
void rqi_check_longsys(struct task_struct *tsk);

struct rqi_percpu_numa_stat {
	u64 stats[RQI_NUMA_STAT_COUNT];
};

#endif /* _LINUX_RQI_H */

