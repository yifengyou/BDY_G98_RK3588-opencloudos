/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sparse bitmap operations
 * References: https://lkml.org/lkml/2018/12/6/1245
 */

#ifndef __PER_LLC_CPU__H__
#define __PER_LLC_CPU__H__

#include <linux/kernel.h>
#include <linux/bitmap.h>
#include <linux/bug.h>

/*
 * struct per_llc_cpu is a dense bitmap which will restore cpu id.
 */
struct per_llc_cpu_array {
	unsigned long batch;
} ____cacheline_aligned_in_smp;

struct per_llc_cpu {
	short total_nr;		/* current number of total cpus*/
	short shift;		/* store 2^shift cpu id per array */
	short first_cpu;	/* the first cpu id in bitmap */
	short seg_len;		/* cpu continuous length */
	short seg_gap;		/* cpu gap within smt */
	struct per_llc_cpu_array arrays[];
};

#define _PER_LLC_INDEX(shift, id)	((id) >> (shift))
#define PER_LLC_INDEX(mask, id)		_PER_LLC_INDEX((mask)->shift, id)
#define PER_LLC_BIT(mask, id)		({int __delta = (id) - (mask)->first_cpu; \
						__delta - ((__delta >= (mask)->seg_len) * (mask)->seg_gap); })
#define PER_LLC_BATCH(mask, id)		(&(mask)->arrays[PER_LLC_INDEX((mask), PER_LLC_BIT(mask, id))].batch)
#define BIT_TO_CPU(mask, bit)		((bit)	>= pllc->seg_len \
						? pllc->first_cpu + (bit) + pllc->seg_gap - pllc->seg_len \
							: pllc->first_cpu + next_bit)

/* Just prevent repetitive settings */
#define IDLE_THRESHOLD_L	1
#define IDLE_THRESHOLD_H	2

/*
 * the max bit for store cpu id, limit it smaller than 64bit.
 */
#define PER_LLC_SHIFT_MAX	6

static inline size_t per_llc_cpu_size(int total_nr, int shift)
{
	int index = _PER_LLC_INDEX(shift, total_nr) + 1;

	return offsetof(struct per_llc_cpu, arrays[index]);
}

/*
 * per_llc_cpu_alloc_node() - Allocate, initialize, and return a per_llc_cpu.
 */
static inline struct per_llc_cpu *
per_llc_cpu_alloc_node(int total_nr, int shift, gfp_t flags, int node)
{
	int nbytes = per_llc_cpu_size(total_nr, shift);
	struct per_llc_cpu *pllc = kmalloc_node(nbytes, flags, node);

	if (pllc) {
		WARN_ON(shift < 0 || shift > PER_LLC_SHIFT_MAX || total_nr < 0);
		pllc->total_nr = total_nr;
		pllc->shift = shift;
	}

	return pllc;
}

static inline void per_llc_cpu_free(struct per_llc_cpu *pllc)
{
	if (pllc)
		kfree(pllc);
}

/*
 * per_llc_cpu_next() - Get next cpu in a bitmap
 */
static inline int
per_llc_cpu_next(const struct per_llc_cpu *pllc, int origin_cpu, int prev_cpu)
{
	int shift = pllc->shift;
	int bits_per_batch = 1U << shift;
	const struct per_llc_cpu_array *chunk;
	int prev_bit = -1;
	int total_bits = (pllc->total_nr >> pllc->shift) + 1 * bits_per_batch;
	int next_bit, bit, nbits, origin_bit;
	unsigned long batch;

	/* covert cpu id to bit */
	origin_bit = PER_LLC_BIT(pllc, origin_cpu);
	if (prev_cpu > -1)
		prev_bit = PER_LLC_BIT(pllc, prev_cpu);

	/* Calculate number of bits to be searched. */
	if (prev_bit == -1) {
		nbits = total_bits;
		next_bit = origin_bit;
	} else if (prev_bit < origin_bit) {
		nbits = origin_bit - prev_bit;
		next_bit = prev_bit + 1;
	} else {
		nbits = total_bits - prev_bit + origin_bit - 1;
		next_bit = prev_bit + 1;
	}

	if (unlikely(next_bit >= pllc->total_nr))
		/* return -2 to end loop */
		return -2;

	chunk = &pllc->arrays[next_bit >> shift];
	bit = next_bit % bits_per_batch;
	batch = chunk->batch & (~0UL << bit);
	next_bit -= bit;
	nbits += bit;

	while (!batch) {
		next_bit += bits_per_batch;
		nbits -= bits_per_batch;
		if (nbits <= 0)
			return -2;

		if (next_bit >= total_bits) {
			chunk = pllc->arrays;
			nbits -= (next_bit - total_bits);
			next_bit = 0;
		} else {
			chunk++;
		}
		batch = chunk->batch;
	}

	next_bit += __ffs(batch);
	if (next_bit >= origin_bit && prev_bit != -1)
		return -2;
	return  BIT_TO_CPU(pllc, next_bit);
}

static inline void per_llc_cpu_set_id(struct per_llc_cpu *dst, int id)
{
	set_bit(PER_LLC_BIT(dst, id), PER_LLC_BATCH(dst, id));
}

static inline void per_llc_cpu_clear_id(struct per_llc_cpu *dst, int id)
{
	clear_bit(PER_LLC_BIT(dst, id), PER_LLC_BATCH(dst, id));
}

static inline int per_llc_cpu_test_id(const struct per_llc_cpu *mask, int id)
{
	return test_bit(PER_LLC_BIT(mask, id), PER_LLC_BATCH(mask, id));
}

/* The traversal time depends on the number of CPUs */
#define per_llc_cpu_for_each(mask, origin, id)			\
	for ((id) = -1;						\
	     (id) = per_llc_cpu_next((mask), (origin), (id)),	\
		 (id) > -2;)

static inline bool cpu_idle_revert_enabled(void)
{
	return sched_feat(IDLE_REVERT);
}

static inline bool cgroup_idle_revert_enable(struct sched_entity *se)
{
	if (!sysctl_tg_idle_revert_enabled)
		return false;

	return se->idle_revert_enabled ? true : false;
}

#ifdef CONFIG_SMP
static inline bool is_smt_supported(void)
{
#ifdef CONFIG_X86_64
	if (cpu_smt_num_threads > 1)
		return true;
#endif
	return false;
}

static inline int sd_llc_alloc(struct sched_domain *sd)
{
	struct sched_domain_shared *sds = sd->shared;
	struct cpumask *span = sched_domain_span(sd);
	int first_cpu = cpumask_first(span);
	int total_cpus = cpumask_weight(span);
	int nid = cpu_to_node(first_cpu);
	int flags = __GFP_ZERO | GFP_KERNEL;
	struct per_llc_cpu *pllc;

	if (!sds->llc_overload_cpus) {
		/*
		 * Use 32-bit bitmap to optimize indexing speed,
		 * so the shift is set 5.
		 */
		pllc = per_llc_cpu_alloc_node(total_cpus, 5, flags, nid);
		if (pllc) {
			if (is_smt_supported()) {
				int smt_last = cpumask_last(topology_sibling_cpumask(first_cpu));

				pllc->first_cpu = first_cpu;
				pllc->seg_len = smt_last - first_cpu == 1 ? total_cpus : total_cpus/2;
				pllc->seg_gap = smt_last - first_cpu == 1 ? 0 : smt_last - first_cpu;
			} else {
				pllc->first_cpu = first_cpu;
				pllc->seg_len = total_cpus;
				pllc->seg_gap = 0;
			}
		} else
			return 1;
		sds->llc_overload_cpus = pllc;
	}

	return 0;
}

static inline void sd_llc_free(struct sched_domain *sd)
{
	struct sched_domain_shared *sds = sd->shared;

	if (!sds)
		return;

	per_llc_cpu_free(sds->llc_overload_cpus);
	sds->llc_overload_cpus = NULL;
}

static inline bool llc_need_clear(struct rq *rq)
{
	bool need_clear = true;

	if (!cpu_idle_revert_enabled())
		return false;

	if (rq->cfs.h_nr_runnable >= IDLE_THRESHOLD_H)
		need_clear = false;

	if (sysctl_tg_idle_revert_enabled &&
	    rq->cfs.llc_h_nr_runnable < IDLE_THRESHOLD_L)
		need_clear = true;

	return need_clear;
}

static inline void llc_overload_clear(struct rq *rq)
{
	struct per_llc_cpu *overload_cpus;

	if (!cpu_idle_revert_enabled())
		return;

	rcu_read_lock();
	overload_cpus = rcu_dereference(rq->llc_overload_cpus);
	if (overload_cpus)
		per_llc_cpu_clear_id(overload_cpus, rq->cpu);
	rcu_read_unlock();
}

static inline bool llc_need_set(struct rq *rq)
{
	bool need_set = true;

	if (!cpu_idle_revert_enabled())
		return false;

	/* First support global tasks */
	if (rq->cfs.h_nr_runnable < IDLE_THRESHOLD_H)
		need_set = false;

	/* Second support task group */
	if (sysctl_tg_idle_revert_enabled &&
	    rq->cfs.llc_h_nr_runnable >= IDLE_THRESHOLD_L)
		need_set = true;

	return need_set;
}

static inline void llc_overload_set(struct rq *rq)
{
	struct per_llc_cpu *overload_cpus;

	if (!cpu_idle_revert_enabled())
		return;

	rcu_read_lock();
	overload_cpus = rcu_dereference(rq->llc_overload_cpus);
	if (overload_cpus)
		per_llc_cpu_set_id(overload_cpus, rq->cpu);
	rcu_read_unlock();
}
#else
static inline int sd_llc_alloc(struct sched_domain *sd)
{
	return 1;
}
static inline void sd_llc_free(struct sched_domain *sd)
{
}
static inline bool llc_need_clear(struct rq *rq)
{
	return false;
}
static inline void llc_overload_clear(struct rq *rq)
{
}
static inline bool llc_need_set(struct rq *rq)
{
	return false;
}
static inline void llc_overload_set(struct rq *rq)
{
}
#endif

static inline void llc_detach_task(struct task_struct *p, struct rq *src_rq, int dst_cpu)
{
	lockdep_assert_rq_held(src_rq);

	deactivate_task(src_rq, p, DEQUEUE_NOCLOCK);
	set_task_cpu(p, dst_cpu);
}

extern int llc_try_idle_revert(struct rq *dst_rq, struct rq_flags *dst_rf);

#endif /* __PER_LLC_CPU__H__ */
