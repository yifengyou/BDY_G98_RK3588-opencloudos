#undef TRACE_SYSTEM
#define TRACE_SYSTEM cgroup_numa_balance

#if !defined(_TRACE_CGROUP_NUMA_BALANCE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CGROUP_NUMA_BALANCE_H

#include <linux/tracepoint.h>

TRACE_EVENT(cgroup_numa_balance_scan_file_lru,

	TP_PROTO(unsigned long pages_scanned, unsigned long nr_reclaimed),

	TP_ARGS(pages_scanned, nr_reclaimed),

	TP_STRUCT__entry(
		__field(unsigned long, pages_scanned)
		__field(unsigned long, nr_reclaimed)
	),

	TP_fast_assign(
		__entry->pages_scanned = pages_scanned;
		__entry->nr_reclaimed = nr_reclaimed;
	),

	TP_printk("pages_scanned=%lu nr_reclaimed=%lu",
		__entry->pages_scanned,
		__entry->nr_reclaimed)
);

TRACE_EVENT(cgroup_numa_balance_scan_file_mglru,

	TP_PROTO(unsigned long pages_scanned, unsigned long nr_reclaimed),

	TP_ARGS(pages_scanned, nr_reclaimed),

	TP_STRUCT__entry(
		__field(unsigned long, pages_scanned)
		__field(unsigned long, nr_reclaimed)
	),

	TP_fast_assign(
		__entry->pages_scanned = pages_scanned;
		__entry->nr_reclaimed = nr_reclaimed;
	),

	TP_printk("pages_scanned=%lu nr_reclaimed=%lu",
		__entry->pages_scanned,
		__entry->nr_reclaimed)
);

TRACE_EVENT(cgroup_numa_balance_scan_vma,

	TP_PROTO(unsigned long pages_scanned),

	TP_ARGS(pages_scanned),

	TP_STRUCT__entry(
		__field(unsigned long, pages_scanned)
	),

	TP_fast_assign(
		__entry->pages_scanned = pages_scanned;
	),

	TP_printk("pages_scanned=%lu",
		__entry->pages_scanned)
);

#endif /* _TRACE_CGROUP_NUMA_BALANCE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
