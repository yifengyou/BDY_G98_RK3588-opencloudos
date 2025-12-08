/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __MM_MEMCONTROL_V1_H
#define __MM_MEMCONTROL_V1_H

#include <linux/cgroup-defs.h>
/* Cgroup v1-specific declarations */
#ifdef CONFIG_MEMCG_KMEM
void memcg1_account_kmem(struct mem_cgroup *memcg, int nr_pages);
#else	/* CONFIG_MEMCG_KMEM */
static inline void memcg1_account_kmem(struct mem_cgroup *memcg, int nr_pages) {}
#endif	/* CONFIG_MEMCG_KMEM */

#endif	/* __MM_MEMCONTROL_V1_H */
