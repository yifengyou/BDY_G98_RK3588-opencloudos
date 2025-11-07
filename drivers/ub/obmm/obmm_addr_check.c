// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#define pr_fmt(fmt) "OBMM: addr_check:" fmt

#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/xarray.h>
#include <linux/spinlock.h>
#include <linux/maple_tree.h>

#include "obmm_addr_check.h"

struct pa_checker {
	spinlock_t lock;
	struct maple_tree pa_ranges;
};
static struct pa_checker g_pa_checker;

static bool is_same_pa_range(const struct obmm_pa_range *l, const struct obmm_pa_range *r)
{
	bool same = l->start == r->start && l->end == r->end;

	if (!same)
		pr_err("unmatched pa range: [%pa, %pa] vs. [%pa, %pa]\n", &l->start, &l->end,
		       &r->start, &r->end);
	return same;
}

int occupy_pa_range(const struct obmm_pa_range *pa_range)
{
	int ret;
	void *persist_info;
	unsigned long flags;

	persist_info = kmemdup(pa_range, sizeof(*pa_range), GFP_KERNEL);
	if (persist_info == NULL)
		return -ENOMEM;

	spin_lock_irqsave(&g_pa_checker.lock, flags);
	ret = mtree_insert_range(&g_pa_checker.pa_ranges, (unsigned long)pa_range->start,
				 (unsigned long)pa_range->end, persist_info, GFP_ATOMIC);
	spin_unlock_irqrestore(&g_pa_checker.lock, flags);

	if (ret != 0) {
		kfree(persist_info);
		pr_err("failed to occupy PA range [%pa, %pa]: ret=%pe\n", &pa_range->start,
		       &pa_range->end, ERR_PTR(ret));
		return ret;
	}
	pr_debug("pa_check: add [%pa,%pa]->{user=%s,data=%p}\n", &pa_range->start, &pa_range->end,
		 "direct_import", pa_range->info.data);
	return 0;
}

int free_pa_range(const struct obmm_pa_range *pa_range)
{
	int ret;
	const char *user;
	void *entry;
	unsigned long flags;

	spin_lock_irqsave(&g_pa_checker.lock, flags);
	entry = mtree_erase(&g_pa_checker.pa_ranges, (unsigned long)pa_range->start);
	spin_unlock_irqrestore(&g_pa_checker.lock, flags);
	if (!entry) {
		pr_err("PA range [%pa, %pa], not found.\n", &pa_range->start, &pa_range->end);
		return -EFAULT;
	}
	ret = 0;
	if (!is_same_pa_range((const struct obmm_pa_range *)entry, pa_range)) {
		/* expected to be UNREACHABLE */
		pr_err("BUG: PA range does not fully match.\n");
		ret = -ENOTRECOVERABLE;
	}
	user = "import";
	pr_debug("pa_check: del [%pa,?]->{user=%s,data=%p}\n", &pa_range->start, user,
		 ((struct obmm_pa_range *)entry)->info.data);
	kfree(entry);
	return ret;
}

int query_pa_range(phys_addr_t addr, struct obmm_addr_info *info)
{
	unsigned long index, flags;
	const struct obmm_pa_range *retrieved;

	if (info == NULL)
		return -EINVAL;

	index = (unsigned long)addr;
	spin_lock_irqsave(&g_pa_checker.lock, flags);
	retrieved = (const struct obmm_pa_range *)mt_find(&g_pa_checker.pa_ranges, &index, index);
	if (retrieved) {
		info->user = retrieved->info.user;
		info->data = retrieved->info.data;
	}
	spin_unlock_irqrestore(&g_pa_checker.lock, flags);

	if (!retrieved)
		return -EFAULT;
	return 0;
}

int update_pa_range(phys_addr_t addr, const struct obmm_addr_info *info)
{
	unsigned long index, flags;
	struct obmm_pa_range *retrieved;

	if (info == NULL)
		return -EINVAL;

	index = (unsigned long)addr;
	spin_lock_irqsave(&g_pa_checker.lock, flags);
	retrieved = (struct obmm_pa_range *)mt_find(&g_pa_checker.pa_ranges, &index, index);
	if (retrieved) {
		retrieved->info.user = info->user;
		retrieved->info.data = info->data;
	}
	spin_unlock_irqrestore(&g_pa_checker.lock, flags);

	if (!retrieved)
		return -EFAULT;
	pr_debug("pa_check: update [%pa,?]->{user=%s,data=%p}\n", &addr, "direct_import",
		 info->data);
	return 0;
}

void module_addr_check_init(void)
{
	mt_init(&g_pa_checker.pa_ranges);
	spin_lock_init(&g_pa_checker.lock);
}
void module_addr_check_exit(void)
{
	mtree_destroy(&g_pa_checker.pa_ranges);
}
