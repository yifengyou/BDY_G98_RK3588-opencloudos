#ifndef _SCHED_FAIR_H
#define _SCHED_FAIR_H

extern unsigned int sched_nr_latency;

u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw);

static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - max_vruntime);
	if (delta > 0)
		max_vruntime = vruntime;

	return max_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

u64 __sched_period(unsigned long nr_running);

#ifdef CONFIG_BT_SCHED

#define BT_SCHED_BULD_FIXED_1

#define BT_WEIGHT 10
#define CFS_WEIGHT 1000
static inline u64 get_balance_load(struct rq *rq)
{
	return rq->cfs.h_nr_running * CFS_WEIGHT + rq->bt.h_nr_running * BT_WEIGHT;
}

static inline u64 get_balance_load_dequeue_bt(struct rq *rq, int nr)
{
	unsigned int bt_h_nr_running;

	bt_h_nr_running = rq->bt.h_nr_running >= nr ? rq->bt.h_nr_running - nr : 0;
	return rq->cfs.h_nr_running * CFS_WEIGHT + bt_h_nr_running * BT_WEIGHT;
}

static inline u64 get_balance_load_enqueue_bt(struct rq *rq, int nr)
{
	return rq->cfs.h_nr_running * CFS_WEIGHT + (rq->bt.h_nr_running + nr) * BT_WEIGHT;
}

#endif

#endif /* _SCHED_FAIR_H */
