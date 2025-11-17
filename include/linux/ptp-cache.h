/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTP_CACHE_H
#define _LINUX_PTP_CACHE_H
#include <linux/types.h>
union freelist_aba_t {
	struct {
		void **freelist;
		unsigned long counter;
	};
	u128 full;
};

struct pg_cache {
	union {
		struct {
			void **freelist;
			unsigned long tid;
		};
		union freelist_aba_t freelist_tid;
	};
	unsigned long reserve_order;
	unsigned long reserve_start_addr;
	unsigned long reserve_end_addr;
	unsigned long object_order;
	int levels;
	const char *name;
	atomic_t count;
	atomic_t fail_count;
};

extern struct pg_cache pg_cache;
extern void ptp_pg_cache_init(struct pg_cache *cache, unsigned long object_order,
								int levels, const char *name);
extern void ptp_set_iee_reserved(struct pg_cache *cache);
extern void *ptp_pg_alloc(struct pg_cache *cache, gfp_t gfp);
extern void ptp_pg_free(struct pg_cache *cache, void *object);
#endif
