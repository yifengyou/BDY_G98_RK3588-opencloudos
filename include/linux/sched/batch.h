#ifndef _SCHED_BATCH_H
#define _SCHED_BATCH_H

/*
 * SCHED_BT tasks has 140~179 priorities, reflecting
 * the fact that any of them has slower prio than RT and
 * NORMAL/BATCH tasks.
 */

#define MAX_CFS_PRIO	139
#define MIN_BT_PRIO	140
#define MAX_BT_PRIO	179
#define BT_PRIO_INTER	40

static inline int cfs_prio(int prio)
{
	if (prio >= MAX_RT_PRIO && prio < MIN_BT_PRIO)
		return 1;
	return 0;
}

static inline int bt_prio(int prio)
{
	if (prio > MAX_CFS_PRIO && prio < MAX_PRIO)
		return 1;
	return 0;
}

static inline int bt_task(struct task_struct *p)
{
	return bt_prio(p->prio);
}

static inline void bt_prio_adjust_pos(int *prio)
{
	int priority = *prio;

	if (cfs_prio(priority))
		*prio = priority + BT_PRIO_INTER;
}

static inline void bt_prio_adjust_neg(int *prio)
{
	int priority = *prio;

	if (bt_prio(priority))
		*prio = priority - BT_PRIO_INTER;
}
#endif /* _SCHED_BATCH_H */
