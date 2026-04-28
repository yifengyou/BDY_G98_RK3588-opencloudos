#undef TRACE_SYSTEM
#define TRACE_SYSTEM kstaled

#if !defined(_TRACE_KSTALED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_KSTALED_H

#include <linux/tracepoint.h>
#include <linux/types.h>
#include <linux/mm.h>

TRACE_EVENT(kstaled_folio_age,
	TP_PROTO(const char *stats),
	TP_ARGS(stats),

	TP_STRUCT__entry(
		__array(char, stats, 128)
	),

	TP_fast_assign(
		strscpy(__entry->stats, stats, 128);
	),

	TP_printk("age_stats=%s", __entry->stats)
);
#endif /* _TRACE_KSTALED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
