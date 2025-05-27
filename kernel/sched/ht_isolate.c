#include <linux/cpumask.h>
#include "ht_isolate.h"
#include "sched.h"

int sysctl_ht_isolate;

bool may_kick_sibling(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	return rq->ht_sensi_type == HT_SENSITIV_TYPE ? true : false;
}

static inline bool online_running_oncpu(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	return rq->core_curr_stat == HT_ONLINE_RUN ? true : false;
}

static inline bool idle_running_oncpu(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	return rq->core_curr_stat == HT_IDLE_RUN ? true : false;
}

static inline bool offline_running_oncpu(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	return rq->core_curr_stat == HT_OFFLINE_RUN ? true : false;
}

void kick_sibling_offline_task(void)
{
	int i, cpu = smp_processor_id();
	const struct cpumask *smt_mask = cpu_smt_mask(cpu);
	struct rq *rq_i;

	if (!sysctl_cpu_qos || !sysctl_ht_isolate)
		return;

	if (!smt_mask)
		return;

	rcu_read_lock();
	for_each_cpu(i, smt_mask) {
		if (i == cpu)
			continue;
		rq_i = cpu_rq(i);
		if (offline_running_oncpu(i) && may_kick_sibling(cpu))
			resched_curr(rq_i);
	}
	rcu_read_unlock();
	return;
}

void kick_sibling_offline_run(void)
{
	int i, cpu = smp_processor_id();
	const struct cpumask *smt_mask = cpu_smt_mask(cpu);
	struct rq *rq_i;

	if (!sysctl_cpu_qos || !sysctl_ht_isolate)
		return;

	if (!smt_mask)
		return;

	rcu_read_lock();
	for_each_cpu(i, smt_mask) {
		if (i == cpu)
			continue;
		rq_i = cpu_rq(i);
		if (idle_running_oncpu(i) && rq_i->bt_nr_running)
			resched_curr(rq_i);
	}
	rcu_read_unlock();
	return;
}

bool sensitive_running_on_sibling(int cpu)
{
	int i;
	const struct cpumask *smt_mask = cpu_smt_mask(cpu);

	if (!smt_mask)
		return false;

	if (!sysctl_cpu_qos || !sysctl_ht_isolate)
		return false;

	for_each_cpu(i, smt_mask) {
		if (i == cpu)
			continue;
		if (online_running_oncpu(i) && may_kick_sibling(i))
			return true;
	}

	return false;
}
