// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/memcontrol.h>
#include "memcontrol-v1.h"

#ifdef CONFIG_MEMCG_KMEM
void memcg1_account_kmem(struct mem_cgroup *memcg, int nr_pages)
{
	if (!cgroup_subsys_on_dfl(memory_cgrp_subsys)) {
		if (nr_pages > 0)
			page_counter_charge(&memcg->kmem, nr_pages);
		else
			page_counter_uncharge(&memcg->kmem, -nr_pages);
	}
}
#endif /* CONFIG_MEMCG_KMEM */
