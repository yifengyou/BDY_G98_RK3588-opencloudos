/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM lockup

#if !defined(_TRACE_LOCKUP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_LOCKUP_H

#include <linux/sched.h>
#include <linux/tracepoint.h>

/*
 * Lockup tracepoints
 * ------------------
 *
 * These events expose the softlockup / hardlockup detector state to
 * the standard ftrace event framework, so that filtering, aggregation,
 * and stack capture can be done with native filter / hist trigger /
 * stacktrace trigger primitives -- no custom user-space agent needed.
 *
 * The three events differ in trigger context and semantics:
 *
 *   softlockup_sample  - emitted from watchdog_timer_fn() on every
 *                        per-CPU hrtimer tick (soft IRQ context).
 *                        Carries stuck_ns = nanoseconds since the last
 *                        successful reschedule on this CPU. Default-off.
 *                        Pair with a filter such as
 *                        "stuck_ns >= 200000000" to record only long
 *                        windows.
 *
 *   softlockup_warn    - emitted from watchdog_timer_fn() when the
 *                        report threshold (2 * effective_thresh) has
 *                        been exceeded, immediately before the existing
 *                        pr_emerg() / show_regs() flow.
 *
 *   hardlockup_warn    - emitted from watchdog_hardlockup_check() in
 *                        NMI context when is_hardlockup() fires.
 *                        Detection is based on a missing hrtimer
 *                        interrupt, NOT on a duration measurement, so
 *                        no stuck_ns / thresh_ms fields are present.
 *                        The reported `cpu` is the locked-up CPU and
 *                        may differ from smp_processor_id() at the
 *                        trace site (the buddy/perf detector can fire
 *                        on a remote CPU). When the reported CPU is
 *                        not the local CPU, comm/ip are best-effort
 *                        and may be slightly stale.
 */
DECLARE_EVENT_CLASS(lockup_sample_template,

	TP_PROTO(int cpu, u64 stuck_ns, u32 thresh_ms,
		 pid_t pid, const char *comm, unsigned long ip),

	TP_ARGS(cpu, stuck_ns, thresh_ms, pid, comm, ip),

	TP_STRUCT__entry(
		__field(	int,		cpu		)
		__field(	u64,		stuck_ns	)
		__field(	u32,		thresh_ms	)
		__field(	pid_t,		pid		)
		__array(	char,		comm,	TASK_COMM_LEN)
		__field(	unsigned long,	ip		)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->stuck_ns	= stuck_ns;
		__entry->thresh_ms	= thresh_ms;
		__entry->pid		= pid;
		memcpy(__entry->comm, comm, TASK_COMM_LEN);
		__entry->ip		= ip;
	),

	TP_printk("cpu=%d stuck=%llu ns thresh=%u ms task=%s(%d) ip=%pS",
		__entry->cpu,
		(unsigned long long)__entry->stuck_ns,
		__entry->thresh_ms,
		__entry->comm, __entry->pid, (void *)__entry->ip)
);

DEFINE_EVENT(lockup_sample_template, softlockup_sample,
	TP_PROTO(int cpu, u64 stuck_ns, u32 thresh_ms,
		 pid_t pid, const char *comm, unsigned long ip),
	TP_ARGS(cpu, stuck_ns, thresh_ms, pid, comm, ip));

DEFINE_EVENT(lockup_sample_template, softlockup_warn,
	TP_PROTO(int cpu, u64 stuck_ns, u32 thresh_ms,
		 pid_t pid, const char *comm, unsigned long ip),
	TP_ARGS(cpu, stuck_ns, thresh_ms, pid, comm, ip));

TRACE_EVENT(hardlockup_warn,

	TP_PROTO(int cpu, pid_t pid, const char *comm, unsigned long ip),

	TP_ARGS(cpu, pid, comm, ip),

	TP_STRUCT__entry(
		__field(	int,		cpu	)
		__field(	pid_t,		pid	)
		__array(	char,		comm,	TASK_COMM_LEN)
		__field(	unsigned long,	ip	)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->pid = pid;
		memcpy(__entry->comm, comm, TASK_COMM_LEN);
		__entry->ip  = ip;
	),

	TP_printk("cpu=%d task=%s(%d) ip=%pS",
		__entry->cpu, __entry->comm, __entry->pid,
		(void *)__entry->ip)
);

/*
 * long_oncpu_sample - @current has been continuously on the same CPU.
 * Unlike softlockup_sample, only resets on a real context switch, so
 * cond_resched() callers that always win the rebid still surface.
 * Default off; gated by kernel.long_oncpu_thresh_ms.
 */
TRACE_EVENT(long_oncpu_sample,

	TP_PROTO(int cpu, u64 oncpu_ns, u32 thresh_ms,
		 pid_t pid, const char *comm, unsigned long ip),

	TP_ARGS(cpu, oncpu_ns, thresh_ms, pid, comm, ip),

	TP_STRUCT__entry(
		__field(	int,		cpu		)
		__field(	u64,		oncpu_ns	)
		__field(	u32,		thresh_ms	)
		__field(	pid_t,		pid		)
		__array(	char,		comm,	TASK_COMM_LEN)
		__field(	unsigned long,	ip		)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->oncpu_ns	= oncpu_ns;
		__entry->thresh_ms	= thresh_ms;
		__entry->pid		= pid;
		memcpy(__entry->comm, comm, TASK_COMM_LEN);
		__entry->ip		= ip;
	),

	TP_printk("cpu=%d oncpu=%llu ns thresh=%u ms task=%s(%d) ip=%pS",
		__entry->cpu,
		(unsigned long long)__entry->oncpu_ns,
		__entry->thresh_ms,
		__entry->comm, __entry->pid, (void *)__entry->ip)
);

#endif /* _TRACE_LOCKUP_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
