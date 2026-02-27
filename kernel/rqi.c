// SPDX-License-Identifier: GPL-2.0-only
/*
 * RQI (Resource Quality Indicator) - QoS monitor
 */

#include "linux/printk.h"
#include "sched/sched.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/stacktrace.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/bitmap.h>
#include <linux/cpumask.h>
#include <linux/rqi.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/jump_label.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>
#ifdef CONFIG_NUMA
#include <linux/mmzone.h>
#include <linux/nodemask.h>
#endif

#define RQI_MAX_STACK_DEPTH 16
#define RQI_STACK_RECORD_MAX 32

/* RQI enabled static key */
DEFINE_STATIC_KEY_FALSE(rqi_enabled);

struct rqi_stack_record {
	unsigned long backtrace[RQI_MAX_STACK_DEPTH];
	unsigned int nr_entries;
	atomic64_t max_latency_ns;
	atomic64_t count;
	pid_t pid;
	char comm[TASK_COMM_LEN];
	enum rqi_event_id event_id;
};

/* Latency histogram buckets */
enum rqi_lat_count {
	RQI_LAT_0_1,		/* [0,1ms) */
	RQI_LAT_1_4,		/* [1,4ms) */
	RQI_LAT_4_8,		/* [4,8ms) */
	RQI_LAT_8_16,		/* [8,16ms) */
	RQI_LAT_16_32,		/* [16,32ms) */
	RQI_LAT_32_64,		/* [32,64ms) */
	RQI_LAT_64_128,		/* [64,128ms) */
	RQI_LAT_128_INF,	/* [>=128ms) */
	RQI_LAT_COUNT_NR,
};

/* Histogram */
struct rqi_hist {
	u64 buckets[RQI_LAT_COUNT_NR];
};

struct rqi_pcpu_stat {
	/* Per-CPU stat counters - only for percpu stat events */
	u64 stat[RQI_PERCPU_STAT_COUNT];

	/* Duration event counters */
	u64 duration_max[RQI_DURATION_COUNT];
	u64 duration_over_threshold[RQI_DURATION_COUNT];

	/* duration histogram */
	struct rqi_hist duration_hist[RQI_DURATION_COUNT];
};

struct rqi_global_stat {
	struct rqi_stack_record stack_records[RQI_STACK_RECORD_MAX];
	unsigned int stack_count;
};

/* Per-CPU counters for global stat events; read side aggregates in sqe_file show. */
struct rqi_global_stat_pcp {
	u64 stat[RQI_GLOBAL_STAT_COUNT];
};

/* Event descriptor: const metadata (set at compile time), mutable state at runtime. */
struct rqi_event {
	const char * const name;
	const enum rqi_event_type type;
	const u32 id;
	const u32 attr;
	bool enabled;
	u64 threshold;
	cpumask_t cpu_mask;
};
static struct ctl_table_header *rqi_sysctl_header;

static DEFINE_PER_CPU(struct rqi_pcpu_stat, rqi_pcpu_data);
static DEFINE_PER_CPU(struct rqi_global_stat_pcp, rqi_global_stat_pcp);
static struct rqi_global_stat rqi_global_data;

static struct rqi_event rqi_events[RQI_EVENT_ID_MAX] = {
	[RQI_MEM_SHRINK_PAGES] = {
		.name = "mem_shrink_pages",
		.type = RQI_EVENT_MEM,
		.id   = RQI_MEM_SHRINK_PAGES,
		.attr = RQI_ATTR_GLOBAL | RQI_ATTR_STAT,
	},
	[RQI_MEM_LRUADD_FLUSH] = {
		.name = "mem_lruadd_flush",
		.type = RQI_EVENT_CPU,
		.id   = RQI_MEM_LRUADD_FLUSH,
		.attr = RQI_ATTR_PERCPU | RQI_ATTR_STAT,
	},
	[RQI_NUMA_LOCAL] = {
		.name = "numa_local",
		.type = RQI_EVENT_MEM,
		.id   = RQI_NUMA_LOCAL,
		.attr = RQI_ATTR_PERDEV | RQI_ATTR_STAT,
	},
	[RQI_NUMA_OTHER] = {
		.name = "numa_other",
		.type = RQI_EVENT_MEM,
		.id   = RQI_NUMA_OTHER,
		.attr = RQI_ATTR_PERDEV | RQI_ATTR_STAT,
	},
	[RQI_CPU_RUNDELAY] = {
		.name = "cpu_rundelay",
		.type = RQI_EVENT_CPU,
		.id   = RQI_CPU_RUNDELAY,
		.attr = RQI_ATTR_DURATION | RQI_ATTR_END_IS_DELTA,
	},
	[RQI_CPU_LONGSYS] = {
		.name = "cpu_longsys",
		.type = RQI_EVENT_CPU,
		.id   = RQI_CPU_LONGSYS,
		.attr = RQI_ATTR_DURATION | RQI_ATTR_END_IS_DELTA,
	},
	[RQI_MEM_SWAP_FAULT] = {
		.name = "mem_swap_fault",
		.type = RQI_EVENT_CPU,
		.id   = RQI_MEM_SWAP_FAULT,
		.attr = RQI_ATTR_DURATION,
	},
	[RQI_MEM_DIRECT_RECLAIM] = {
		.name = "mem_direct_reclaim",
		.type = RQI_EVENT_CPU,
		.id   = RQI_MEM_DIRECT_RECLAIM,
		.attr = RQI_ATTR_DURATION,
	},
	[RQI_MEM_KSWAPD_RECLAIM] = {
		.name = "mem_kswapd_reclaim",
		.type = RQI_EVENT_CPU,
		.id   = RQI_MEM_KSWAPD_RECLAIM,
		.attr = RQI_ATTR_DURATION,
	},
};

static DEFINE_RWLOCK(rqi_events_lock);

static void rqi_global_init(void)
{
	int cpu;

	memset(&rqi_global_data, 0, sizeof(rqi_global_data));
	for_each_possible_cpu(cpu)
		memset(per_cpu_ptr(&rqi_global_stat_pcp, cpu), 0,
		       sizeof(struct rqi_global_stat_pcp));
}

/* Runtime init: only set mutable fields for registered events (name != NULL). */
static void __init rqi_events_runtime_init(void)
{
	unsigned int i;

	for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
		struct rqi_event *e = &rqi_events[i];

		if (!e->name)
			continue;

		e->enabled = false;
		e->threshold = 0;
		cpumask_setall(&e->cpu_mask);
	}
}

static inline bool rqi_is_duration_event(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return (rqi_events[event_id].attr & RQI_ATTR_DURATION) != 0;
}

static inline bool rqi_event_enabled(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return READ_ONCE(rqi_events[event_id].enabled);
}

static inline bool rqi_cpu_in_mask(enum rqi_event_id event_id, int cpu)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return cpumask_test_cpu(cpu, &rqi_events[event_id].cpu_mask);
}

static inline bool rqi_is_stat_event(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return (rqi_events[event_id].attr & RQI_ATTR_STAT) != 0;
}

static inline bool rqi_is_global_event(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return (rqi_events[event_id].attr & RQI_ATTR_GLOBAL) != 0;
}

/* Event appears in /proc/rqi/percpu_stat (per-CPU counter, not duration) */
static inline bool rqi_is_percpu_stat_event(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return rqi_events[event_id].name &&
			(rqi_events[event_id].attr & RQI_ATTR_STAT) != 0 &&
			(rqi_events[event_id].attr & RQI_ATTR_GLOBAL) == 0 &&
			(rqi_events[event_id].attr & RQI_ATTR_PERCPU) != 0;
}

/* Check if event is per-node: MEM type + PERDEV attribute */
static inline bool rqi_is_pernode_event(enum rqi_event_id event_id)
{
	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return false;
	return (rqi_events[event_id].type == RQI_EVENT_MEM &&
		(rqi_events[event_id].attr & RQI_ATTR_PERDEV) != 0);
}

/* Get the index of a per-node event in the rqi_numa_stat array */
static inline unsigned int rqi_numa_stat_index(enum rqi_event_id event_id)
{
	if (event_id < RQI_NUMA_STAT_START || event_id >= RQI_NUMA_STAT_END)
		return RQI_NUMA_STAT_COUNT; /* Invalid index */
	return event_id - RQI_NUMA_STAT_START;
}

/* Get the index of a global stat event in the global_stat array */
static inline unsigned int rqi_global_stat_index(enum rqi_event_id event_id)
{
	if (event_id < RQI_GLOBAL_STAT_START || event_id >= RQI_GLOBAL_STAT_END)
		return RQI_GLOBAL_STAT_COUNT; /* Invalid index */
	return event_id - RQI_GLOBAL_STAT_START;
}

/* Get the index of a per-CPU stat event in the percpu stat array */
static inline unsigned int rqi_percpu_stat_index(enum rqi_event_id event_id)
{
	if (event_id < RQI_PERCPU_STAT_START || event_id >= RQI_PERCPU_STAT_END)
		return RQI_PERCPU_STAT_COUNT; /* Invalid index */
	return event_id - RQI_PERCPU_STAT_START;
}

static inline unsigned int rqi_duration_index(enum rqi_event_id event_id)
{
	if (event_id < RQI_DURATION_STAT_START || event_id >= RQI_DURATION_STAT_END)
		return RQI_DURATION_COUNT; /* Invalid */
	return event_id - RQI_DURATION_STAT_START;
}

/* cpulist str: "0", "0-7", "0,2-4,6" */
static int rqi_parse_cpu_mask(const char *str, struct cpumask *mask)
{
	if (!str || !mask)
		return -EINVAL;
	cpumask_clear(mask);
	return cpulist_parse(str, mask);
}

static inline enum rqi_lat_count rqi_get_latency_bucket(u64 ns)
{
	u64 ms = ns / 1000000; /* ns -> ms */

	if (ms < 1)
		return RQI_LAT_0_1;
	else if (ms < 4)
		return RQI_LAT_1_4;
	else if (ms < 8)
		return RQI_LAT_4_8;
	else if (ms < 16)
		return RQI_LAT_8_16;
	else if (ms < 32)
		return RQI_LAT_16_32;
	else if (ms < 64)
		return RQI_LAT_32_64;
	else if (ms < 128)
		return RQI_LAT_64_128;
	else
		return RQI_LAT_128_INF;
}
#ifdef CONFIG_NUMA
static int rqi_ensure_pgdat_numa_stats(void)
{
	struct pglist_data *pgdat;
	int cpu;
	int ret = 0;

	cpus_read_lock();
	for_each_online_pgdat(pgdat) {
		if (!pgdat->rqi_pcpu_numa_stats) {
			pgdat->rqi_pcpu_numa_stats =
				alloc_percpu(struct rqi_percpu_numa_stat);
			if (!pgdat->rqi_pcpu_numa_stats) {
				pr_warn("RQI: alloc rqi_pcpu_numa_stats failed for nid %d\n",
					pgdat->node_id);
				ret = -ENOMEM;

				continue;
			}
		}

		for_each_online_cpu(cpu)
			memset(per_cpu_ptr(pgdat->rqi_pcpu_numa_stats, cpu), 0,
					sizeof(struct rqi_percpu_numa_stat));
	}
	cpus_read_unlock();

	return ret;
}
#endif

/* Clear all RQI data when disabled
 */
static void rqi_clear_all_data(void)
{
	unsigned int i, cpu;
	unsigned long flags;
	struct rqi_pcpu_stat *pcpu;

	for_each_possible_cpu(cpu) {
		pcpu = per_cpu_ptr(&rqi_pcpu_data, cpu);
		memset(pcpu, 0, sizeof(*pcpu));
		memset(per_cpu_ptr(&rqi_global_stat_pcp, cpu), 0,
			sizeof(struct rqi_global_stat_pcp));
	}

#ifdef CONFIG_NUMA
#ifdef CONFIG_RQI
	{
		struct pglist_data *pgdat;
		int cpu;

		for_each_online_pgdat(pgdat) {
			if (!pgdat->rqi_pcpu_numa_stats)
				continue;

			for_each_online_cpu(cpu) {
				memset(per_cpu_ptr(pgdat->rqi_pcpu_numa_stats, cpu),
				       0, sizeof(struct rqi_percpu_numa_stat));
			}
		}
	}
#endif /* CONFIG_RQI */
#endif /* CONFIG_NUMA */

	write_lock_irqsave(&rqi_events_lock, flags);
	memset(&rqi_global_data, 0, sizeof(rqi_global_data));
	for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
		if (rqi_events[i].name) {
			WRITE_ONCE(rqi_events[i].enabled, false);
			WRITE_ONCE(rqi_events[i].threshold, 0);
			cpumask_setall(&rqi_events[i].cpu_mask);
		}
	}
	write_unlock_irqrestore(&rqi_events_lock, flags);
}
static int rqi_enable __read_mostly;
static int sys_rqi_save_stack __read_mostly;

static int rqi_enable_handler(struct ctl_table *table, int write,
				void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;
	int old_val = rqi_enable;

	ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
	if (!write || ret)
		return ret;

	if (old_val == rqi_enable)
		return 0;

	if (rqi_enable) {
#ifdef CONFIG_NUMA
		ret = rqi_ensure_pgdat_numa_stats();
		if (ret) {
			rqi_enable = old_val;
			pr_warn("RQI: enable failed (NUMA stats alloc incomplete)\n");
			return ret;
		}
#endif
		rqi_clear_all_data();
		if (!static_key_enabled(&rqi_enabled))
			static_branch_enable(&rqi_enabled);
		pr_info("RQI enabled\n");
	} else {
		if (static_key_enabled(&rqi_enabled))
			static_branch_disable(&rqi_enabled);
		pr_info("RQI disabled\n");
	}

	return 0;
}

static int rqi_save_stack_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;
	int old_val = sys_rqi_save_stack;

	ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);

	if (write && ret == 0) {
		if (old_val != sys_rqi_save_stack) {
			if (sys_rqi_save_stack) {
				pr_info("RQI save_stack enabled\n");
			} else {
				pr_info("RQI save_stack disabled\n");
				if (rqi_global_data.stack_count > 0) {
					unsigned long flags;

					write_lock_irqsave(&rqi_events_lock, flags);
					rqi_global_data.stack_count = 0;
					write_unlock_irqrestore(&rqi_events_lock, flags);
				}
			}
		}
	}

	return ret;
}

static struct ctl_table rqi_table[] = {
	{
		.procname   = "rqi_enable",
		.data       = &rqi_enable,
		.maxlen     = sizeof(int),
		.mode       = 0644,
		.proc_handler = rqi_enable_handler,
		.extra1     = SYSCTL_ZERO,
		.extra2     = SYSCTL_ONE,
	},
	{
		.procname   = "rqi_save_stack",
		.data       = &sys_rqi_save_stack,
		.maxlen     = sizeof(int),
		.mode       = 0644,
		.proc_handler = rqi_save_stack_handler,
		.extra1     = SYSCTL_ZERO,
		.extra2     = SYSCTL_ONE,
	},
	{ }
};

static bool rqi_stack_same(const struct rqi_stack_record *r1,
			const struct rqi_stack_record *r2)
{
	int i;

	/* Compare event_id first */
	if (r1->event_id != r2->event_id)
		return false;

	if (r1->nr_entries != r2->nr_entries)
		return false;

	for (i = 0; i < r1->nr_entries; i++) {
		if (r1->backtrace[i] != r2->backtrace[i])
			return false;
		if (r1->backtrace[i] == 0)
			break;
	}

	return true;
}

static __always_inline void
rqi_stack_record_copy(struct rqi_stack_record *dst,
			const struct rqi_stack_record *src)
{
	memcpy(dst->backtrace, src->backtrace, sizeof(dst->backtrace));
	dst->nr_entries = src->nr_entries;
	dst->pid = src->pid;
	memcpy(dst->comm, src->comm, TASK_COMM_LEN);
	dst->event_id = src->event_id;

	atomic64_set(&dst->max_latency_ns, atomic64_read(&src->max_latency_ns));
	atomic64_set(&dst->count,          atomic64_read(&src->count));
}

static __always_inline bool
rqi_stack_try_update(struct rqi_stack_record *records, unsigned int limit,
			const struct rqi_stack_record *key, u64 latency_ns)
{
	unsigned int i;

	for (i = 0; i < limit; i++) {
		if (records[i].backtrace[0] == 0)
			continue;

		if (!rqi_stack_same(&records[i], key))
			continue;

		atomic64_inc(&records[i].count);

		for (;;) {
			u64 old = atomic64_read(&records[i].max_latency_ns);

			if (latency_ns <= old)
				break;

			if (atomic64_cmpxchg(&records[i].max_latency_ns, old, latency_ns) == old)
				break;
		}
		return true;
	}
	return false;
}

static void rqi_save_stack(enum rqi_event_id event_id, u64 latency_ns,
		struct task_struct *task)
{
	struct rqi_stack_record new_record;
	struct rqi_stack_record *records;
	unsigned long flags;
	bool found = false;
	unsigned int write_pos, search_limit;

	if (event_id >= RQI_EVENT_ID_MAX || !rqi_is_duration_event(event_id))
		return;
	memset(&new_record, 0, sizeof(new_record));

	if (task) {
		new_record.nr_entries = stack_trace_save_tsk(task, new_record.backtrace,
							     RQI_MAX_STACK_DEPTH, 0);
		new_record.pid = task->pid;
		memcpy(new_record.comm, task->comm, TASK_COMM_LEN);
	} else {
		new_record.nr_entries = stack_trace_save(new_record.backtrace,
							  RQI_MAX_STACK_DEPTH, 0);
		new_record.pid = current->pid;
		memcpy(new_record.comm, current->comm, TASK_COMM_LEN);
	}
	atomic64_set(&new_record.max_latency_ns, latency_ns);
	atomic64_set(&new_record.count, 1);
	new_record.event_id = event_id;

	read_lock_irqsave(&rqi_events_lock, flags);
	records = rqi_global_data.stack_records;
	/* try to find if the same stack already exists */
	search_limit = min_t(unsigned int, rqi_global_data.stack_count, RQI_STACK_RECORD_MAX);
	found = rqi_stack_try_update(records, search_limit, &new_record, latency_ns);
	read_unlock_irqrestore(&rqi_events_lock, flags);

	/* If not found, add new record. */
	if (!found) {
		write_lock_irqsave(&rqi_events_lock, flags);
		records = rqi_global_data.stack_records;
		search_limit = min_t(unsigned int, rqi_global_data.stack_count,
			 RQI_STACK_RECORD_MAX);
		/* double-check */
		if (rqi_stack_try_update(records, search_limit, &new_record, latency_ns)) {
			write_unlock_irqrestore(&rqi_events_lock, flags);
			return;
		}
		if (rqi_global_data.stack_count < RQI_STACK_RECORD_MAX) {
			write_pos = rqi_global_data.stack_count++;
			rqi_stack_record_copy(&records[write_pos], &new_record);
		}
		/* else: full, discard new event */
		write_unlock_irqrestore(&rqi_events_lock, flags);
	}
}

#ifdef CONFIG_NUMA
static __always_inline void rqi_numa_stat_add(pg_data_t *pgdat, unsigned int idx, u64 delta)
{
	struct rqi_percpu_numa_stat *pcpu;

	if (unlikely(!pgdat->rqi_pcpu_numa_stats))
		return;

	pcpu = this_cpu_ptr(pgdat->rqi_pcpu_numa_stats);
	pcpu->stats[idx] += delta;
}
#endif

void rqi_stat(enum rqi_event_id event_id, u64 delta, void *arg)
{
	struct rqi_event *event;
	unsigned int stat_idx;
	int cpu;
#ifdef CONFIG_NUMA
	int node_id;
	pg_data_t *pgdat;
	unsigned int numa_stat_idx;
#endif

	if (!static_branch_likely(&rqi_enabled))
		return;

	if (!delta || event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return;

	event = &rqi_events[event_id];
	if (!READ_ONCE(event->enabled) || !event->name)
		return;

	if (!rqi_is_stat_event(event_id))
		return;
	cpu = smp_processor_id();
	if (!rqi_cpu_in_mask(event_id, cpu))
		return;

	/* Global events: update per-CPU counter */
	if (rqi_is_global_event(event_id)) {
		stat_idx = rqi_global_stat_index(event_id);
		if (stat_idx >= RQI_GLOBAL_STAT_COUNT)
			return;
		this_cpu_add(rqi_global_stat_pcp.stat[stat_idx], delta);
		return;
	}

#ifdef CONFIG_NUMA
	/* Per-node events: update per-node counter */
	if (rqi_is_pernode_event(event_id)) {
		/* Get node id from arg if provided, otherwise use current CPU's node */
		if (arg)
			node_id = *(int *)arg;
		else
			node_id = cpu_to_node(cpu);

		if (node_id < 0 || node_id >= MAX_NUMNODES || !node_online(node_id))
			return;
		pgdat = NODE_DATA(node_id);
		if (!pgdat)
			return;

		/* Use elegant index mapping: event_id - RQI_NUMA_STAT_START */
		numa_stat_idx = rqi_numa_stat_index(event_id);
		if (numa_stat_idx >= RQI_NUMA_STAT_COUNT)
			return;

		/* Update per-node statistics */
		rqi_numa_stat_add(pgdat, numa_stat_idx, delta);
		return;
	}
#endif

	/* Per-CPU events: update per-CPU counter */
	cpu = smp_processor_id();
	if (!rqi_cpu_in_mask(event_id, cpu))
		return;

	stat_idx = rqi_percpu_stat_index(event_id);
	if (stat_idx >= RQI_PERCPU_STAT_COUNT)
		return;

	this_cpu_add(rqi_pcpu_data.stat[stat_idx], delta);
}
EXPORT_SYMBOL_GPL(rqi_stat);

/* rqi_start used to record start timestamps for duration events */
u64 rqi_start(enum rqi_event_id event_id)
{
	struct rqi_event *event;

	if (!static_branch_likely(&rqi_enabled))
		return 0;

	if (event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return 0;

	event = &rqi_events[event_id];
	if (!READ_ONCE(event->enabled) || !event->name)
		return 0;

	if (!(event->attr & RQI_ATTR_DURATION))
		return 0;

	if (!rqi_cpu_in_mask(event_id, smp_processor_id()))
		return 0;

	return get_jiffies_64();
}
EXPORT_SYMBOL_GPL(rqi_start);

static __always_inline void
__rqi_end_common(enum rqi_event_id event_id, u64 value,
		 struct task_struct *target_task)
{
	struct rqi_event *event;
	u64 delta_ns, delta_jiffies;
	unsigned int duration_idx;
	int cpu;
	bool is_delta;
	enum rqi_lat_count bucket;
	bool over_threshold;

	if (!static_branch_likely(&rqi_enabled))
		return;

	if (!value || event_id >= RQI_EVENT_ID_MAX || (int)event_id < 0)
		return;

	event = &rqi_events[event_id];
	if (!READ_ONCE(event->enabled) || !event->name)
		return;

	if (!rqi_is_duration_event(event_id))
		return;

	cpu = smp_processor_id();
	if (!rqi_cpu_in_mask(event_id, cpu))
		return;

	is_delta = (event->attr & RQI_ATTR_END_IS_DELTA) != 0;
	if (is_delta) {
		delta_ns = value;
	} else {
		delta_jiffies = get_jiffies_64() - value;
		delta_ns = jiffies64_to_nsecs(delta_jiffies);
	}

	duration_idx = rqi_duration_index(event_id);

	if (duration_idx >= RQI_DURATION_COUNT)
		return;

	bucket = rqi_get_latency_bucket(delta_ns);
	u64 thr = READ_ONCE(event->threshold);

	over_threshold = (thr > 0 && delta_ns >= thr);
	this_cpu_inc(rqi_pcpu_data.duration_hist[duration_idx].buckets[bucket]);
	if (delta_ns > this_cpu_read(rqi_pcpu_data.duration_max[duration_idx]))
		this_cpu_write(rqi_pcpu_data.duration_max[duration_idx], delta_ns);

	if (over_threshold && sys_rqi_save_stack && bucket >= RQI_LAT_16_32) {
		this_cpu_inc(rqi_pcpu_data.duration_over_threshold[duration_idx]);
		rqi_save_stack(event_id, delta_ns, target_task);
	}
}

void rqi_end(enum rqi_event_id event_id, u64 value)
{
	__rqi_end_common(event_id, value, current);
}
EXPORT_SYMBOL_GPL(rqi_end);

/* The caller must guarantee that @task remains valid */
void rqi_end_task(enum rqi_event_id event_id, u64 value, struct task_struct *task)
{
	if (!task)
		task = current;
	__rqi_end_common(event_id, value, task);
}
EXPORT_SYMBOL_GPL(rqi_end_task);


/* RQI rundelay */
void rqi_rundelay(struct task_struct *prev, u64 delta_ns)
{
	struct rqi_event *event;
	unsigned int duration_idx;
	int cpu;
	enum rqi_lat_count bucket;
	bool over_threshold;

	if (!static_branch_likely(&rqi_enabled))
		return;

	if (!delta_ns)
		return;

	event = &rqi_events[RQI_CPU_RUNDELAY];
	if (!READ_ONCE(event->enabled) || !event->name)
		return;
	if (!rqi_is_duration_event(RQI_CPU_RUNDELAY))
		return;

	cpu = smp_processor_id();
	if (!rqi_cpu_in_mask(RQI_CPU_RUNDELAY, cpu))
		return;

	duration_idx = rqi_duration_index(RQI_CPU_RUNDELAY);
	if (duration_idx >= RQI_DURATION_COUNT)
		return;

	bucket = rqi_get_latency_bucket(delta_ns);
	u64 thr = READ_ONCE(event->threshold);

	over_threshold = (thr > 0 && delta_ns >= thr);
	if (delta_ns > this_cpu_read(rqi_pcpu_data.duration_max[duration_idx]))
		this_cpu_write(rqi_pcpu_data.duration_max[duration_idx], delta_ns);
	this_cpu_inc(rqi_pcpu_data.duration_hist[duration_idx].buckets[bucket]);
	if (over_threshold)
		this_cpu_inc(rqi_pcpu_data.duration_over_threshold[duration_idx]);

	if (over_threshold && prev)
		rqi_save_stack(RQI_CPU_RUNDELAY, delta_ns, prev);
}
EXPORT_SYMBOL_GPL(rqi_rundelay);

void rqi_check_longsys(struct task_struct *tsk)
{
	long delta;

	if (!static_branch_likely(&rqi_enabled))
		return;

	if (tsk->sched_class != &fair_sched_class)
		return;

	/* Longsys is performed only when TIF_RESCHED is set */
	if (!test_tsk_need_resched(tsk))
		return;

	/* Kthread is not belong to any cgroup */
	if (tsk->flags & PF_KTHREAD)
		return;

	if (!tsk->sched_info.kernel_exec_start ||
		tsk->sched_info.task_switch != (tsk->nvcsw + tsk->nivcsw) ||
		tsk->utime != tsk->sched_info.utime) {
		tsk->sched_info.utime = tsk->utime;
		tsk->sched_info.kernel_exec_start = rq_clock(task_rq(tsk));
		tsk->sched_info.task_switch = tsk->nvcsw + tsk->nivcsw;
		return;

	}
	delta = rq_clock(task_rq(tsk)) - tsk->sched_info.kernel_exec_start;
	rqi_end_task(RQI_CPU_LONGSYS, delta, tsk);
}

static int __rqi_event_set_threshold(enum rqi_event_id event_id, u64 threshold)
{
	lockdep_assert_held_write(&rqi_events_lock);

	if (event_id >= RQI_EVENT_ID_MAX)
		return -EINVAL;

	if (!rqi_events[event_id].name)
		return -ENOENT;
	if (!(rqi_events[event_id].attr & RQI_ATTR_DURATION))
		return -EINVAL;
	WRITE_ONCE(rqi_events[event_id].threshold, threshold);

	return 0;
}

/* /proc/rqi/stat - seq_operations: show global stat events; */
static void *rqi_stat_seq_start(struct seq_file *m, loff_t *pos)
{
	/* 0 is reserved for SEQ_START_TOKEN (header or disabled check) */
	if (*pos == 0)
		return SEQ_START_TOKEN;

	/* If disabled, stop iteration immediately for data elements */
	if (!static_key_enabled(&rqi_enabled))
		return NULL;

	/* Check bounds (pos is 1-based index for events here) */
	if (*pos > RQI_EVENT_ID_MAX)
		return NULL;

	return &rqi_events[*pos - 1];
}

static void *rqi_stat_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	if (!static_key_enabled(&rqi_enabled))
		return NULL;

	if (*pos > RQI_EVENT_ID_MAX)
		return NULL;

	return &rqi_events[*pos - 1];
}

static void rqi_stat_seq_stop(struct seq_file *m, void *v)
{
}

static int rqi_stat_seq_show(struct seq_file *m, void *v)
{
	struct rqi_event *event;
	unsigned int event_id;
	unsigned int stat_idx;
	u64 sum;
	int cpu;

	/* Handle the start token: check if disabled */
	if (v == SEQ_START_TOKEN) {
		if (!static_key_enabled(&rqi_enabled))
			seq_puts(m, "RQI is disabled.\n");
		return 0;
	}

	event = v;
	event_id = event->id;
	if (!event->name || event_id >= RQI_EVENT_ID_MAX)
		return 0;
	if (!rqi_is_stat_event(event_id) || !rqi_is_global_event(event_id))
		return 0;

	stat_idx = rqi_global_stat_index(event_id);
	if (stat_idx >= RQI_GLOBAL_STAT_COUNT)
		return 0;

	sum = 0;
	for_each_online_cpu(cpu)
		sum += per_cpu_ptr(&rqi_global_stat_pcp, cpu)->stat[stat_idx];
	seq_printf(m, "%s %llu\n", event->name, sum);

	return 0;
}

static const struct seq_operations rqi_stat_seq_ops = {
	.start	= rqi_stat_seq_start,
	.next	= rqi_stat_seq_next,
	.stop	= rqi_stat_seq_stop,
	.show	= rqi_stat_seq_show,
};

static int rqi_stat_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rqi_stat_seq_ops);
}

static const struct proc_ops rqi_stat_proc_ops = {
	.proc_open = rqi_stat_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};

/* /proc/rqi/percpu_stat */
static void *rqi_percpu_stat_seq_start(struct seq_file *m, loff_t *pos)
{
	unsigned long n = *pos;
	int cpu;

	if (n == 0)
		return SEQ_START_TOKEN;

	if (!static_key_enabled(&rqi_enabled))
		return NULL;

	n--;
	if (n > 0)
		cpu = cpumask_next(n - 1, cpu_online_mask);
	else
		cpu = cpumask_first(cpu_online_mask);

	*pos = (loff_t)cpu + 1;

	if (cpu < nr_cpu_ids)
		return (void *)(uintptr_t)(cpu + 2);
	return NULL;
}

static void *rqi_percpu_stat_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	return rqi_percpu_stat_seq_start(m, pos);
}

static void rqi_percpu_stat_seq_stop(struct seq_file *m, void *v) {}

static int rqi_percpu_stat_seq_show(struct seq_file *m, void *v)
{
	unsigned int i, cpu;
	struct rqi_pcpu_stat *pcpu;
	unsigned int stat_idx;
	char devname[16];

	if (v == SEQ_START_TOKEN) {
		if (!static_key_enabled(&rqi_enabled)) {
			seq_puts(m, "RQI is disabled.\n");
			return 0;
		}

		seq_printf(m, "%-12s", "cpu");
		for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
			if (rqi_is_percpu_stat_event(i))
				seq_printf(m, " %s", rqi_events[i].name);
		}
		seq_putc(m, '\n');
		return 0;
	}

	if (!static_key_enabled(&rqi_enabled))
		return 0;

	cpu = (unsigned int)((uintptr_t)v - 2);
	if (cpu >= nr_cpu_ids || !cpu_online(cpu))
		return 0;

	pcpu = per_cpu_ptr(&rqi_pcpu_data, cpu);
	snprintf(devname, sizeof(devname), "cpu%u", cpu);
	seq_printf(m, "%-12s", devname);
	for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
		if (!rqi_is_percpu_stat_event(i))
			continue;
		stat_idx = rqi_percpu_stat_index(i);
		if (stat_idx < RQI_PERCPU_STAT_COUNT)
			seq_printf(m, " %llu", (u64)pcpu->stat[stat_idx]);
	}
	seq_putc(m, '\n');
	return 0;
}

static const struct seq_operations rqi_percpu_stat_seq_ops = {
	.start	= rqi_percpu_stat_seq_start,
	.next	= rqi_percpu_stat_seq_next,
	.stop	= rqi_percpu_stat_seq_stop,
	.show	= rqi_percpu_stat_seq_show,
};

static int rqi_percpu_stat_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rqi_percpu_stat_seq_ops);
}

static const struct proc_ops rqi_percpu_stat_proc_ops = {
	.proc_open = rqi_percpu_stat_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};

static inline bool rqi_is_duration_event_id(unsigned int i)
{
	if (i >= RQI_EVENT_ID_MAX)
		return 0;
	return rqi_events[i].name && rqi_is_duration_event(i) &&
		rqi_duration_index(i) < RQI_DURATION_COUNT;
}

static void *rqi_lat_seq_start(struct seq_file *m, loff_t *pos)
{
	if (*pos == 0)
		return SEQ_START_TOKEN;

	if (!static_key_enabled(&rqi_enabled))
		return NULL;

	if (*pos > RQI_EVENT_ID_MAX)
		return NULL;

	return &rqi_events[*pos - 1];
}

static void *rqi_lat_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;
	if (!static_key_enabled(&rqi_enabled))
		return NULL;

	if (*pos == 0)
		return SEQ_START_TOKEN;

	if (*pos > RQI_EVENT_ID_MAX)
		return NULL;

	return &rqi_events[*pos - 1];
}

static void rqi_lat_seq_stop(struct seq_file *m, void *v)
{
}

struct rqi_lat_cpu_snap {
	u64 buckets[RQI_LAT_COUNT_NR];
	u64 max_latency;
	u64 over_threshold;
};

static int rqi_lat_seq_show(struct seq_file *m, void *v)
{
	struct rqi_event *event;
	const char *event_name;
	unsigned int event_id, duration_idx;
	unsigned int cpu, b, idx, nr_cpus;

	struct rqi_lat_cpu_snap *snap;
	int *cpu_ids;

	if (v == SEQ_START_TOKEN) {
		if (!static_key_enabled(&rqi_enabled)) {
			seq_puts(m, "RQI is disabled.\n");
			return 0;
		}
		seq_puts(m, "cpu_event duration_max(ms) over_threshold_count ");
		seq_puts(m, "[0,1ms) [1,4ms) [4,8ms) [8,16ms) [16,32ms) [32,64ms) ");
		seq_puts(m, "[64,128ms) [>=128ms]\n");
		return 0;
	}

	event = v;
	event_id = event->id;
	event_name = event->name;

	if (!event_name || event_id >= RQI_EVENT_ID_MAX)
		return 0;

	if (!rqi_is_duration_event(event_id))
		return 0;

	duration_idx = rqi_duration_index(event_id);
	if (duration_idx >= RQI_DURATION_COUNT)
		return 0;

	cpus_read_lock();
	nr_cpus = num_online_cpus();
	if (!nr_cpus) {
		cpus_read_unlock();
		return 0;
	}

	snap = kcalloc(nr_cpus, sizeof(*snap), GFP_KERNEL);
	cpu_ids = kcalloc(nr_cpus, sizeof(*cpu_ids), GFP_KERNEL);
	if (!snap || !cpu_ids) {
		kfree(snap);
		kfree(cpu_ids);
		cpus_read_unlock();
		return -ENOMEM;
	}

	idx = 0;
	for_each_online_cpu(cpu) {
		struct rqi_pcpu_stat *pcpu = per_cpu_ptr(&rqi_pcpu_data, cpu);

		cpu_ids[idx] = cpu;
		snap[idx].max_latency = pcpu->duration_max[duration_idx];
		snap[idx].over_threshold = pcpu->duration_over_threshold[duration_idx];

		for (b = 0; b < RQI_LAT_COUNT_NR; b++)
			snap[idx].buckets[b] =
				pcpu->duration_hist[duration_idx].buckets[b];

		idx++;
	}
	cpus_read_unlock();

	for (idx = 0; idx < nr_cpus; idx++) {
		seq_printf(m, "cpu%d_%s %llu %llu",
			   cpu_ids[idx], event_name,
			   (u64)(snap[idx].max_latency / 1000000),
			   (u64)snap[idx].over_threshold);
		for (b = 0; b < RQI_LAT_COUNT_NR; b++)
			seq_printf(m, " %llu", (u64)snap[idx].buckets[b]);
		seq_putc(m, '\n');
	}

	kfree(snap);
	kfree(cpu_ids);
	return 0;
}

static const struct seq_operations rqi_lat_seq_ops = {
	.start	= rqi_lat_seq_start,
	.next	= rqi_lat_seq_next,
	.stop	= rqi_lat_seq_stop,
	.show	= rqi_lat_seq_show,
};

static int rqi_rqilat_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rqi_lat_seq_ops);
}

static const struct proc_ops rqi_rqilat_proc_ops = {
	.proc_open = rqi_rqilat_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release,
};

struct rqi_event_snap {
	const char *name;
	bool enabled;
	u64 threshold;
	bool is_duration;
	cpumask_t cpu_mask;
};

static int rqi_control_proc_show(struct seq_file *m, void *v)
{
	struct rqi_event_snap *snap;
	unsigned long flags;
	unsigned int i;

	if (!static_key_enabled(&rqi_enabled)) {
		seq_puts(m, "RQI is disabled.\n");
		return 0;
	}

	snap = kcalloc(RQI_EVENT_ID_MAX, sizeof(*snap), GFP_KERNEL);
	if (!snap)
		return -ENOMEM;


	read_lock_irqsave(&rqi_events_lock, flags);
	for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
		struct rqi_event *event = &rqi_events[i];

		if (!event->name)
			continue;


		snap[i].name = event->name;
		snap[i].enabled = event->enabled;
		snap[i].threshold = event->threshold;
		snap[i].is_duration = rqi_is_duration_event(i);
		cpumask_copy(&snap[i].cpu_mask, &event->cpu_mask);
	}
	read_unlock_irqrestore(&rqi_events_lock, flags);

	seq_puts(m, "event_name[,threshold=<value>],stat=<0|1>,cpu=<mask>\n");

	for (i = 0; i < RQI_EVENT_ID_MAX; i++) {
		const struct cpumask *cm = &snap[i].cpu_mask;

		if (!snap[i].name)
			continue;

		seq_printf(m, "%s", snap[i].name);

		if (snap[i].is_duration)
			seq_printf(m, ",threshold=%llu", (u64)snap[i].threshold);

		seq_printf(m, ",stat=%u,cpu=", snap[i].enabled ? 1 : 0);
		if (!cpumask_empty(cm))
			seq_printf(m, "%*pbl", cpumask_pr_args(cm));
		else
			seq_puts(m, "none");
		seq_putc(m, '\n');
	}

	kfree(snap);
	return 0;
}

static ssize_t rqi_control_proc_write(struct file *file,
				      const char __user *buffer,
				      size_t count, loff_t *ppos)
{
	char *buf, *orig_buf, *name, *p, *token;
	unsigned int event_id;
	u64 threshold = 0;
	bool threshold_set = false;
	int stat = -1;
	cpumask_var_t tmp_mask;
	unsigned long flags;
	ssize_t ret = count;
	bool found = false;
	bool cpu_mask_set = false;

	if (!static_key_enabled(&rqi_enabled)) {
		pr_warn("RQI is disabled.\n");
		return -EPERM;
	}

	if (count == 0 || count > PAGE_SIZE)
		return -EINVAL;

	if (!alloc_cpumask_var(&tmp_mask, GFP_KERNEL))
		return -ENOMEM;

	orig_buf = kzalloc(count + 1, GFP_KERNEL);
	if (!orig_buf) {
		free_cpumask_var(tmp_mask);
		return -ENOMEM;
	}

	if (copy_from_user(orig_buf, buffer, count)) {
		kfree(orig_buf);
		free_cpumask_var(tmp_mask);
		return -EFAULT;
	}
	orig_buf[count] = '\0';

	buf = strstrip(orig_buf);

	/* parse: eventname,threshold=1000,stat=1,cpu=0-1 */
	name = buf;
	p = strchr(name, ',');
	if (p)
		*p++ = '\0';

	for (event_id = 0; event_id < RQI_EVENT_ID_MAX; event_id++) {
		if (rqi_events[event_id].name &&
		    strcmp(rqi_events[event_id].name, name) == 0) {
			found = true;
			break;
		}
	}

	if (!found) {
		ret = -ENOENT;
		goto out;
	}

	while (p && *p) {
		token = p;
		p = strchr(p, ',');
		if (p)
			*p++ = '\0';

		if (strncmp(token, "threshold=", 10) == 0) {
			if (kstrtou64(token + 10, 0, &threshold)) {
				ret = -EINVAL;
				goto out;
			}
			threshold_set = true;
		} else if (strncmp(token, "stat=", 5) == 0) {
			int val;

			if (kstrtoint(token + 5, 0, &val)) {
				ret = -EINVAL;
				goto out;
			}
			if (val != 0 && val != 1) {
				ret = -EINVAL;
				goto out;
			}
			stat = val;
		} else if (strncmp(token, "cpu=", 4) == 0) {
			char *cpu_str = token + 4;

			if (rqi_parse_cpu_mask(cpu_str, tmp_mask) != 0) {
				ret = -EINVAL;
				goto out;
			}
			cpu_mask_set = true;
		} else {
			/* unknown key -> reject to avoid silent typos */
			ret = -EINVAL;
			goto out;
		}
	}

	write_lock_irqsave(&rqi_events_lock, flags);

	if (!threshold_set && stat < 0 && !cpu_mask_set) {
		write_unlock_irqrestore(&rqi_events_lock, flags);
		ret = -EINVAL;
		goto out;
	}
	if (threshold_set) {
		if (!rqi_is_duration_event(event_id)) {
			write_unlock_irqrestore(&rqi_events_lock, flags);
			pr_warn("RQI: threshold is only valid for duration events\n");
			ret = -EINVAL;
			goto out;
		}
		__rqi_event_set_threshold(event_id, threshold);
	}
	if (stat >= 0)
		WRITE_ONCE(rqi_events[event_id].enabled, stat);
	if (cpu_mask_set)
		cpumask_copy(&rqi_events[event_id].cpu_mask, tmp_mask);

	write_unlock_irqrestore(&rqi_events_lock, flags);

out:
	kfree(orig_buf);
	free_cpumask_var(tmp_mask);
	return ret;
}


static int rqi_control_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rqi_control_proc_show, NULL);
}

static const struct proc_ops rqi_control_proc_ops = {
	.proc_open = rqi_control_proc_open,
	.proc_read = seq_read,
	.proc_write = rqi_control_proc_write,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

/* /proc/rqi/stack */
static int rqi_stack_proc_show(struct seq_file *m, void *v)
{
	struct rqi_stack_record *snap = NULL;
	unsigned int i, k, n = 0;
	unsigned long flags;

	if (!static_key_enabled(&rqi_enabled)) {
		seq_puts(m, "RQI is disabled.\n");
		return 0;
	}

	snap = kcalloc(RQI_STACK_RECORD_MAX, sizeof(*snap), GFP_KERNEL);
	if (!snap) {
		return -ENOMEM;
	}

	write_lock_irqsave(&rqi_events_lock, flags);
	n = rqi_global_data.stack_count;
	if (n > RQI_STACK_RECORD_MAX)
		n = RQI_STACK_RECORD_MAX;
	for (i = 0; i < n; i++)
		rqi_stack_record_copy(&snap[i], &rqi_global_data.stack_records[i]);
	/* read-clear (steal semantics) */
	rqi_global_data.stack_count = 0;
	write_unlock_irqrestore(&rqi_events_lock, flags);

	/* Print outside lock */
	for (i = 0; i < n; i++) {
		struct rqi_stack_record *rec = &snap[i];
		const char *event_name = "unknown";

		/* Prefer nr_entries as validity; also handle empty backtrace */
		if (!rec->nr_entries || rec->backtrace[0] == 0)
			continue;

		if (rec->event_id < RQI_EVENT_ID_MAX) {
			const char *name = rqi_events[rec->event_id].name;

			if (name)
				event_name = name;
		}

		seq_printf(m, "Event: %s,", event_name);
		seq_printf(m, "  Max Latency: %llu us,",
			   (u64)(atomic64_read(&rec->max_latency_ns) / 1000));
		seq_printf(m, "  Count: %llu,",
			   (u64)atomic64_read(&rec->count));
		seq_printf(m, "  Triggered by: %s (pid %d)\n",
			   rec->comm, rec->pid);
		seq_puts(m, "  Call Trace:\n");

		for (k = 0; k < rec->nr_entries; k++) {
			if (rec->backtrace[k] == 0)
				break;
			seq_printf(m, "    %pB\n", (void *)(uintptr_t)rec->backtrace[k]);
		}
		seq_putc(m, '\n');
	}

	kfree(snap);
	return 0;
}

static int rqi_stack_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rqi_stack_proc_show, NULL);
}

static const struct proc_ops rqi_stack_proc_ops = {
	.proc_open = rqi_stack_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static void *rqi_numa_stat_start(struct seq_file *m, loff_t *pos)
{
#ifdef CONFIG_NUMA
	pg_data_t *pgdat;
	loff_t n = *pos;

	/* pos==0: header/disabled line */
	if (n == 0)
		return SEQ_START_TOKEN;

	/* pos>=1: iterate pgdat (pos-1) */
	n--;
	for (pgdat = first_online_pgdat(); pgdat && n; pgdat = next_online_pgdat(pgdat))
		--n;

	return pgdat;
#else
	return (*pos == 0) ? SEQ_START_TOKEN : NULL;
#endif
}

static void *rqi_numa_stat_next(struct seq_file *m, void *v, loff_t *pos)
{
	(*pos)++;

#ifdef CONFIG_NUMA
	if (v == SEQ_START_TOKEN)
		return first_online_pgdat();

	return next_online_pgdat((pg_data_t *)v);
#else
	return NULL;
#endif
}

static void rqi_numa_stat_stop(struct seq_file *m, void *v)
{
}

static int rqi_numa_stat_show(struct seq_file *m, void *v)
{
	unsigned int id;

	/* Header / disabled line */
	if (v == SEQ_START_TOKEN) {
		if (!static_key_enabled(&rqi_enabled)) {
			seq_puts(m, "RQI is disabled.\n");
			return 0;
		}

		seq_printf(m, "%-12s", "node");
#ifdef CONFIG_NUMA
		for (id = RQI_NUMA_STAT_START; id < RQI_NUMA_STAT_END; id++) {
			if (!rqi_events[id].name)
				continue;
			const char *name = rqi_events[id].name;

			seq_printf(m, " %s", name);
		}
#endif
		seq_putc(m, '\n');
		return 0;
	}

#ifdef CONFIG_NUMA
	/* Node line */
	if (!static_key_enabled(&rqi_enabled))
		return 0;

	{
		pg_data_t *pgdat = (pg_data_t *)v;
		int node_id = pgdat->node_id;
		const struct cpumask *mask;
		struct rqi_percpu_numa_stat __percpu *pcp;
		int cpu;
		char devname[20];

		if (!node_online(node_id))
			return 0;

		pcp = READ_ONCE(pgdat->rqi_pcpu_numa_stats);
		mask = cpumask_of_node(node_id);

		snprintf(devname, sizeof(devname), "node%d", node_id);
		seq_printf(m, "%-12s", devname);

		for (id = RQI_NUMA_STAT_START; id < RQI_NUMA_STAT_END; id++) {
			u16 idx = id - RQI_NUMA_STAT_START;
			u64 sum = 0;

			if (pcp) {
				for_each_cpu(cpu, mask) {
					struct rqi_percpu_numa_stat *s =
						per_cpu_ptr(pcp, cpu);
					sum += READ_ONCE(s->stats[idx]);
				}
			}

			seq_printf(m, " %llu", sum);
		}

		seq_putc(m, '\n');
		return 0;
	}
#else
	return 0;
#endif
}

static const struct seq_operations rqi_numa_stat_seq_ops = {
	.start	= rqi_numa_stat_start,
	.next	= rqi_numa_stat_next,
	.stop	= rqi_numa_stat_stop,
	.show	= rqi_numa_stat_show,
};

static int rqi_numa_stat_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rqi_numa_stat_seq_ops);
}

static const struct proc_ops rqi_numa_stat_proc_ops = {
	.proc_open	= rqi_numa_stat_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= seq_release,
};



static int __init rqi_events_init(void)
{
	rqi_events_runtime_init();
	return 0;
}

static int __init rqi_proc_init(void)
{
	struct proc_dir_entry *rqi_dir = NULL;

	rqi_global_init();
	rqi_events_init();

	rqi_sysctl_header = register_sysctl("kernel", rqi_table);
	if (!rqi_sysctl_header) {
		pr_err("Failed to register rqi sysctl\n");
		goto err_cleanup;
	}

	rqi_dir = proc_mkdir("rqi", NULL);
	if (!rqi_dir)
		goto err_unregister_sysctl;

	/* /proc/rqi/stat */
	if (!proc_create("stat", 0444, rqi_dir, &rqi_stat_proc_ops))
		goto err_remove_proc;

	/* /proc/rqi/percpu_stat */
	if (!proc_create("percpu_stat", 0444, rqi_dir, &rqi_percpu_stat_proc_ops))
		goto err_remove_proc;

	/* /proc/rqi/numa_stat */
	if (!proc_create("numa_stat", 0444, rqi_dir, &rqi_numa_stat_proc_ops))
		goto err_remove_proc;

	/* /proc/rqi/lat */
	if (!proc_create("lat_stat", 0444, rqi_dir, &rqi_rqilat_proc_ops))
		goto err_remove_proc;

	/* /proc/rqi/control */
	if (!proc_create("control", 0644, rqi_dir, &rqi_control_proc_ops))
		goto err_remove_proc;

	/* /proc/rqi/stack */
	if (!proc_create("stack", 0444, rqi_dir, &rqi_stack_proc_ops))
		goto err_remove_proc;

	return 0;

err_remove_proc:
	if (rqi_dir)
		proc_remove(rqi_dir);
err_unregister_sysctl:
	if (rqi_sysctl_header)
		unregister_sysctl_table(rqi_sysctl_header);
err_cleanup:
	return -ENOMEM;
}
late_initcall(rqi_proc_init);
