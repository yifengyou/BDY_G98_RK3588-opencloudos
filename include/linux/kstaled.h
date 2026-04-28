/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_MM_KSTALED_H
#define _LINUX_MM_KSTALED_H

#include "linux/bits.h"
#ifdef CONFIG_KSTALED

#include <linux/types.h>
#include <linux/mm.h>

#define KSTALED_VERSION			"1.0"
struct mem_cgroup;

/*
 * kstaled_scan_type define the scan type that kstaled will
 * work at. The default option is to scan page only, but
 * it can be modified by a specified interface at any time.
 */
enum kstaled_scan_type {
	SCAN_TARGET_PAGE = 0,
	SCAN_TARGET_ALL
};

#define KSTALED_SCAN_PAGE	(1 << SCAN_TARGET_PAGE)

/*
 * We want to get more info about a specified idle page, whether it's
 * a page cache or in active LRU list and so on. We use KSTALE_<flag>
 * to mark these different page attributes, we support 4 flags:
 *
 * KSTALE_DIRTY  : page is dirty or not;
 * KSTALE_FILE   : page is a page cache or not;
 * KSTALE_UNEVIT : page is unevictable or evictable;
 * KSTALE_ACTIVE : page is in active LRU list or not.
 *
 * Each KSTALE_<flag> occupies one bit position in a specified idle type.
 * There exist total 2^4=16 idle types.
 */
#define KSTALE_BASE			0
#define KSTALE_DIRTY			(1 << 0)
#define KSTALE_FILE			(1 << 1)
#define KSTALE_UNEVICT			(1 << 2)
#define KSTALE_ACTIVE			(1 << 3)

#define KSTALE_NR_TYPE			16

/*
 * Each page has an idle age which means how long the page is keeping
 * in idle state, the age's unit is in one scan period. Each page's
 * idle age will consume one byte, and the max age is 255.
 *
 * Buckets are used to stat histogram if all pages's age, e.g.:
 *
 *     bucket [1,2) ---\             bucket [5,15) ---\
 *                      \                              \
 * # ||||              [1,2)          [2,5)         [5,15)        [15,30)        [30,60)       [60,120)      [120,240)     [240,+inf)
 *   csei           51535872      166055936      250150912      119083008      482594816      113512448              0              0
 *   dsei             520192         274432         692224         110592          94208       26714112              0              0
 *
 * the bucket [5,15) means: 5 <= pages's age < 15 scan_periods.
 */
#define KSTALED_MAX_IDLE_AGE		U8_MAX
#define NUM_KSTALED_BUCKETS		8

/*
 * Literally, it's unnecessary to get an runtime age stats, the better way
 * is to show the stats when one scanning round completed. Here designing
 * two buffers, one is used to store the stable age stats which called
 * 'stable buffer'. Another is used to store the scanning stats which
 * called 'unstable buffer'. Switch them when one scanning round is finished.
 */
#define KSTALED_STATS_NR_TYPE		2
#define KSTALED_INVALID_BUCKET		(KSTALED_MAX_IDLE_AGE + 1)

#define KSTALED_MARK_BUCKET_INVALID(buckets)	\
	(buckets[0] = KSTALED_INVALID_BUCKET)
#define KSTALED_IS_BUCKET_INVALID(buckets)	\
	(buckets[0] == KSTALED_INVALID_BUCKET)

DECLARE_STATIC_KEY_FALSE(kstaled_enabled_key);

/*
 * We account number of idle pages depending on idle type and buckets
 * for a specified instance (e.g. one memory cgroup or one process...)
 */
struct idle_page_stats {
	int			buckets[NUM_KSTALED_BUCKETS];
	unsigned long		count[KSTALE_NR_TYPE][NUM_KSTALED_BUCKETS];
};

#define KSTALED_MAX_SCAN_DURATION	U16_MAX		/* max 65536 seconds */
struct kstaled_scan_control {
	union {
		atomic_t		val;
		struct {
			u16		seq;		/* inc when update */
			u16		duration;	/* in seconds */
		};
	};
	unsigned int scan_target;	/* decide how kstaled to scan */
};
extern struct kstaled_scan_control kstaled_scan_control;
extern unsigned int kstaled_scan_target;
extern unsigned long kstaled_scan_rounds;

#define KSTALED_OP_SET_DURATION		(1 << 0)
#define KSTALED_OP_INC_SEQ		(1 << 1)

static inline struct kstaled_scan_control kstaled_get_current_scan_control(void)
{
	struct kstaled_scan_control scan_control;

	atomic_set(&scan_control.val, atomic_read(&kstaled_scan_control.val));
	scan_control.scan_target = kstaled_scan_target;
	return scan_control;
}

static inline unsigned int kstaled_get_current_scan_duration(void)
{
	struct kstaled_scan_control scan_control =
		kstaled_get_current_scan_control();

	return scan_control.duration;
}

static inline void kstaled_reset_scan_control(struct kstaled_scan_control *p)
{
	atomic_set(&p->val, 0);
	p->scan_target = KSTALED_SCAN_PAGE;
}

/*
 * Compare with global kstaled_scan_control, return true if equals.
 */
static inline bool kstaled_is_scan_period_equal(struct kstaled_scan_control *p)
{
	return atomic_read(&p->val) == atomic_read(&kstaled_scan_control.val);
}

static inline bool kstaled_has_page_target(struct kstaled_scan_control *p)
{
	return p->scan_target & KSTALED_SCAN_PAGE;
}

static inline bool
kstaled_is_scan_target_equal(struct kstaled_scan_control *p)
{
	return p->scan_target == kstaled_scan_target;
}

static inline bool
kstaled_has_page_target_only(struct kstaled_scan_control *p)
{
	return p->scan_target == KSTALED_SCAN_PAGE;
}

static inline bool
kstaled_has_page_target_equal(struct kstaled_scan_control *p)
{
	if (!kstaled_has_page_target(p))
		return false;

	return kstaled_scan_target & KSTALED_SCAN_PAGE;
}

static inline void kstaled_get_reset_type(struct kstaled_scan_control *p,
					 bool *page_disabled)
{
	if (kstaled_has_page_target(p) && !kstaled_has_page_target_equal(p))
		*page_disabled = 1;
}

static inline bool kstaled_set_scan_control(int op, u16 duration,
					  struct kstaled_scan_control *orig)
{
	bool retry = false;

	/*
	 * atomic_cmpxchg() tries to update kstaled_scan_control, shouldn't
	 * retry to avoid endless loop when caller specify a period.
	 */
	if (!orig) {
		orig = &kstaled_scan_control;
		retry = true;
	}

	while (true) {
		int new_period_val, old_period_val;
		struct kstaled_scan_control new_period;

		old_period_val = atomic_read(&orig->val);
		atomic_set(&new_period.val, old_period_val);
		if (op & KSTALED_OP_INC_SEQ)
			new_period.seq++;
		if (op & KSTALED_OP_SET_DURATION)
			new_period.duration = duration;
		new_period_val = atomic_read(&new_period.val);

		if (atomic_cmpxchg(&kstaled_scan_control.val,
				   old_period_val,
				   new_period_val) == old_period_val)
			return true;

		if (!retry)
			return false;
	}
}

static inline void kstaled_set_scan_duration(u16 duration)
{
	kstaled_set_scan_control(KSTALED_OP_INC_SEQ |
			       KSTALED_OP_SET_DURATION,
			       duration, NULL);
}

static inline bool is_kstaled_enabled(void)
{
	return static_branch_unlikely(&kstaled_enabled_key);
}

/*
 * Caller must specify the original scan period, avoid the race between
 * the double operation and user's updates through sysfs interface.
 */
static inline bool
kstaled_try_double_scan_control(struct kstaled_scan_control orig)
{
	u16 duration = orig.duration;

	if (unlikely(duration == KSTALED_MAX_SCAN_DURATION))
		return false;

	duration <<= 1;
	if (duration < orig.duration)
		duration = KSTALED_MAX_SCAN_DURATION;
	return kstaled_set_scan_control(KSTALED_OP_INC_SEQ |
				      KSTALED_OP_SET_DURATION,
				      duration,
				      &orig);
}

/*
 * Increase the sequence number while keep duration the same, it's used
 * to start a new period immediately.
 */
static inline void kstaled_inc_scan_seq(void)
{
	kstaled_set_scan_control(KSTALED_OP_INC_SEQ, 0, NULL);
}

extern const int kstaled_default_buckets[NUM_KSTALED_BUCKETS];

bool kstaled_use_hierarchy(void);
#ifdef CONFIG_MEMCG
void kstaled_mem_cgroup_move_stats(struct mem_cgroup *from,
				  struct mem_cgroup *to,
				  struct folio *folio,
				  unsigned long size);
#endif /* CONFIG_MEMCG */

void kstaled_free_folio_age(pg_data_t *pgdat);

static inline int kstaled_get_folio_age(pg_data_t *pgdat, unsigned long pfn)
{
	u8 *age, age_val;

	rcu_read_lock();
	age = rcu_dereference(pgdat->node_page_age);
	if (unlikely(!age)) {
		rcu_read_unlock();
		return -EINVAL;
	}

	age += (pfn - ALIGN_DOWN(pgdat->node_start_pfn, BITS_PER_BYTE));
	age_val = *age;
	rcu_read_unlock();
	return age_val;
}

static inline int kstaled_inc_folio_age(pg_data_t *pgdat, unsigned long pfn)
{
	u8 *age, age_val;

	rcu_read_lock();
	age = rcu_dereference(pgdat->node_page_age);
	if (unlikely(!age)) {
		rcu_read_unlock();
		return -EINVAL;
	}

	age += (pfn - ALIGN_DOWN(pgdat->node_start_pfn, BITS_PER_BYTE));
	age_val = ++*age;
	rcu_read_unlock();

	return age_val;
}

static inline void kstaled_set_folio_age(pg_data_t *pgdat,
				       unsigned long pfn, int val)
{
	u8 *age;

	rcu_read_lock();
	age = rcu_dereference(pgdat->node_page_age);
	if (unlikely(!age)) {
		rcu_read_unlock();
		return;
	}

	age += (pfn - ALIGN_DOWN(pgdat->node_start_pfn, BITS_PER_BYTE));
	*age = val;
	rcu_read_unlock();
}

#else  /* !CONFIG_KSTALED */

static inline void kstaled_mem_cgroup_move_stats(struct mem_cgroup *from,
						struct mem_cgroup *to,
						struct folio *folio,
						unsigned long size)
{
}

static inline void kstaled_set_folio_age(pg_data_t *pgdat,
					 unsigned long pfn, int val)
{
}

static inline bool is_kstaled_enabled(void)
{
	return false;
}
#endif /* CONFIG_KSTALED */

#endif /* _LINUX_MM_KSTALED_H */
