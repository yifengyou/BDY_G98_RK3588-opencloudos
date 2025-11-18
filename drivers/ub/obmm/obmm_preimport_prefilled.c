// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2025. All rights reserved.
 */

#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/memory.h>

#include "obmm_preimport.h"
#include "obmm_addr_check.h"
#include "obmm_resource.h"

struct prefilled_preimport_range {
	struct preimport_range pr;
	spinlock_t bitmap_lock;
	unsigned long nbits;
	unsigned long *bitmap;
	struct ubmem_resource *ubmem_res;
};
static DEFINE_MUTEX(preimport_mutex);

static int create_prefilled_preimport_range(const struct obmm_cmd_preimport *cmd,
					    struct prefilled_preimport_range **p_ppr)
{
	struct prefilled_preimport_range *ppr;

	ppr = kzalloc(sizeof(struct prefilled_preimport_range), GFP_KERNEL);
	if (ppr == NULL)
		return -ENOMEM;

	ppr->pr.numa_id = cmd->numa_id;
	ppr->pr.start = cmd->pa;
	ppr->pr.end = cmd->pa + cmd->length - 1;
	ppr->pr.scna = cmd->scna;
	ppr->pr.dcna = cmd->dcna;
	memcpy(ppr->pr.deid, cmd->deid, sizeof(cmd->deid));
	memcpy(ppr->pr.seid, cmd->seid, sizeof(cmd->seid));
	ppr->pr.use_count = 0;

	spin_lock_init(&ppr->bitmap_lock);
	ppr->nbits = cmd->length / memory_block_size_bytes();
	ppr->bitmap = bitmap_zalloc(ppr->nbits, GFP_KERNEL);
	if (!ppr->bitmap) {
		pr_err("failed to allocate preimport range bitmap.\n");
		kfree(ppr);
		return -ENOMEM;
	}

	ppr->ubmem_res = setup_ubmem_resource(cmd->pa, cmd->length, true);
	if (IS_ERR(ppr->ubmem_res)) {
		pr_err("failed to setup ubmem resource on preimport. pa=%pa, size=%#llx, ret=%pe\n",
		       &cmd->pa, cmd->length, ppr->ubmem_res);
		kfree(ppr->bitmap);
		kfree(ppr);
		return PTR_ERR(ppr->ubmem_res);
	}

	*p_ppr = ppr;
	return 0;
}

static void destroy_prefilled_preimport_range(const struct prefilled_preimport_range *ppr)
{
	release_ubmem_resource(ppr->ubmem_res);
	kfree(ppr->bitmap);
	kfree(ppr);
}

static int get_pa_mapping(phys_addr_t addr, struct prefilled_preimport_range **p_ppr)
{
	int ret;
	struct obmm_addr_info info;

	ret = query_pa_range(addr, &info);
	if (ret) {
		pr_err("No information found with PA=%pa.\n", &addr);
		return ret;
	}
	if (info.user != OBMM_ADDR_USER_PREIMPORT) {
		pr_err("PA=%pa is not a preimport address.\n", &addr);
		return -EINVAL;
	}
	if (info.data == not_ready_ptr) {
		pr_err("Preimport process not finished. Try later.\n");
		return -EAGAIN;
	}
	*p_ppr = (struct prefilled_preimport_range *)info.data;

	pr_debug("prefilled preimport range found with PA %pa.\n", &addr);
	return 0;
}

static int check_preimport_cmd(const struct obmm_cmd_preimport *cmd)
{
	int ret;

	ret = check_preimport_cmd_common(cmd);
	if (ret)
		return ret;

	if (cmd->pa == 0) {
		pr_err("invalid preimport PA base addr 0.\n");
		return -EINVAL;
	}
	return 0;
}

int preimport_prepare_prefilled(struct obmm_cmd_preimport *cmd)
{
	int ret;
	struct prefilled_preimport_range *ppr;
	struct obmm_pa_range pa_range;

	ret = check_preimport_cmd(cmd);
	if (ret)
		return ret;

	pa_range.start = cmd->pa;
	pa_range.end = cmd->pa + cmd->length - 1;
	pa_range.info.user = OBMM_ADDR_USER_PREIMPORT;
	pa_range.info.data = not_ready_ptr;
	ret = occupy_pa_range(&pa_range);
	if (ret)
		return ret;

	ret = create_prefilled_preimport_range(cmd, &ppr);
	if (ret)
		goto err_free_pa_range;

	ret = preimport_prepare_common(&ppr->pr, cmd->base_dist);
	if (ret)
		goto err_destroy_ppr;
	cmd->numa_id = ppr->pr.numa_id;

	/* make ppr accessible to others, no more access! (ppr might be freed by racers.) */
	pa_range.info.data = (void *)ppr;
	ret = update_pa_range(pa_range.start, &pa_range.info);
	if (ret) {
		cmd->numa_id = NUMA_NO_NODE;
		goto err_unprepare_common;
	}

	return 0;

err_unprepare_common:
	WARN_ON(preimport_release_common(&ppr->pr, true));
err_destroy_ppr:
	destroy_prefilled_preimport_range(ppr);
err_free_pa_range:
	WARN_ON(free_pa_range(&pa_range));
	return ret;
}

int preimport_release_prefilled(phys_addr_t start, phys_addr_t end)
{
	int ret;
	struct obmm_pa_range pa_range;
	struct prefilled_preimport_range *ppr;

	mutex_lock(&preimport_mutex);
	ret = get_pa_mapping(start, &ppr);
	if (ret) {
		pr_err("failed to identify preimport range during unpreimport.\n");
		goto err_unlock;
	}
	/* must be an exact match */
	if (ppr->pr.start != start || ppr->pr.end != end) {
		pr_err("requested range touches ppr<%pa> but is not an exact match.\n",
		       &ppr->pr.start);
		ret = -EINVAL;
		goto err_unlock;
	}
	if (ppr->pr.use_count != 0) {
		pr_err("ppr<%pa> cannot be released: %u active users found.\n", &ppr->pr.start,
		       ppr->pr.use_count);
		ret = -EBUSY;
		goto err_unlock;
	}
	ret = preimport_release_common(&ppr->pr, false);
	if (ret) {
		pr_err("failed to release ppr<%pa>.\n", &ppr->pr.start);
		goto err_unlock;
	}
	/* roll back is not possible from this point */

	pa_range.start = ppr->pr.start;
	pa_range.end = ppr->pr.end;
	pa_range.info.user = OBMM_ADDR_USER_PREIMPORT;
	pa_range.info.data = (void *)ppr;
	WARN_ON(free_pa_range(&pa_range));

	mutex_unlock(&preimport_mutex);

	destroy_prefilled_preimport_range(ppr);
	pr_debug("ppr<%pa> released.\n", &start);
	return ret;

err_unlock:
	mutex_unlock(&preimport_mutex);
	return ret;
}

static int get_ppr(phys_addr_t pa, struct prefilled_preimport_range **p_ppr)
{
	int ret;
	struct prefilled_preimport_range *ppr;

	mutex_lock(&preimport_mutex);
	ret = get_pa_mapping(pa, &ppr);
	if (ret)
		goto out_unlock;
	if (ppr == not_ready_ptr) {
		pr_err("ppr <%pa> not ready yet.\n", &pa);
		ret = -EAGAIN;
		goto out_unlock;
	}
	pr_debug("ppr <%pa> refcount: %u -> %u.\n", &pa, ppr->pr.use_count, ppr->pr.use_count + 1);
	ppr->pr.use_count += 1;
	*p_ppr = ppr;
out_unlock:
	mutex_unlock(&preimport_mutex);
	return ret;
}

static void put_ppr(struct prefilled_preimport_range *ppr)
{
	mutex_lock(&preimport_mutex);
	WARN_ON(ppr->pr.use_count == 0);
	pr_debug("ppr <%pa> refcount: %u -> %u.\n", &ppr->pr.start, ppr->pr.use_count,
		 ppr->pr.use_count - 1);
	ppr->pr.use_count -= 1;
	mutex_unlock(&preimport_mutex);
}

static int occupy_ppr_blocks(struct prefilled_preimport_range *ppr, phys_addr_t start,
			     phys_addr_t end)
{
	int ret = 0;
	unsigned long bit, init_bit, end_bit, flags;

	spin_lock_irqsave(&ppr->bitmap_lock, flags);
	if (start < ppr->pr.start || end > ppr->pr.end) {
		pr_err("requested range [%pa, %pa] is not managed by ppr [%pa, %pa].\n", &start,
		       &end, &ppr->pr.start, &ppr->pr.end);
		ret = -EINVAL;
		goto out_unlock;
	}
	init_bit = (start - ppr->pr.start) / memory_block_size_bytes();
	end_bit = (end - ppr->pr.start) / memory_block_size_bytes();

	for (bit = init_bit; bit <= end_bit; bit++) {
		if (test_bit(bit, ppr->bitmap)) {
			ret = -EEXIST;
			pr_err("conflicts on preimport block %lu of ppr<%pa>.\n", bit,
			       &ppr->pr.start);
			goto out_unlock;
		}
	}

	for (bit = init_bit; bit <= end_bit; bit++)
		set_bit(bit, ppr->bitmap);
	pr_debug("ppr<%pa>: bitmap[%lu, %lu] set.\n", &ppr->pr.start, init_bit, end_bit);

out_unlock:
	spin_unlock_irqrestore(&ppr->bitmap_lock, flags);
	return ret;
}

static int free_ppr_blocks(struct prefilled_preimport_range *ppr, phys_addr_t start,
			   phys_addr_t end)
{
	int ret = 0;
	unsigned long bit, init_bit, end_bit, flags;

	spin_lock_irqsave(&ppr->bitmap_lock, flags);
	if (start < ppr->pr.start || end > ppr->pr.end) {
		pr_err("requested range [%pa, %pa] is not managed by ppr [%pa, %pa].\n", &start,
		       &end, &ppr->pr.start, &ppr->pr.end);
		ret = -EINVAL;
		goto out_unlock;
	}
	init_bit = (start - ppr->pr.start) / memory_block_size_bytes();
	end_bit = (end - ppr->pr.start) / memory_block_size_bytes();

	for (bit = init_bit; bit <= end_bit; bit++) {
		if (!test_bit(bit, ppr->bitmap)) {
			ret = -EINVAL;
			pr_err("preimport block %lu of ppr<%pa> never used.\n", bit,
			       &ppr->pr.start);
			goto out_unlock;
		}
	}

	for (bit = init_bit; bit <= end_bit; bit++)
		clear_bit(bit, ppr->bitmap);
	pr_debug("ppr<%pa>: bitmap[%lu, %lu] cleared.\n", &ppr->pr.start, init_bit, end_bit);

out_unlock:
	spin_unlock_irqrestore(&ppr->bitmap_lock, flags);
	return ret;
}

/* alignment checked by callers */
int preimport_commit_prefilled(phys_addr_t start, phys_addr_t end,
			       const struct obmm_datapath *datapath, int *p_numa_id,
			       void **p_handle)
{
	int ret;
	struct prefilled_preimport_range *ppr;

	ret = get_ppr(start, &ppr);
	if (ret)
		return ret;

	/* TODO: move to out */
	ret = check_preimport_datapath_common(&ppr->pr, datapath);
	if (ret)
		goto err_put_ppr;

	ret = occupy_ppr_blocks(ppr, start, end);
	if (ret)
		goto err_put_ppr;

	*p_numa_id = ppr->pr.numa_id;
	*p_handle = (void *)ppr;
	return 0;

err_put_ppr:
	put_ppr(ppr);
	return ret;
}

int preimport_uncommit_prefilled(void *handle, phys_addr_t start, phys_addr_t end)
{
	int ret;
	struct prefilled_preimport_range *ppr;

	ppr = (struct prefilled_preimport_range *)handle;
	ret = free_ppr_blocks(handle, start, end);
	if (ret)
		return ret;

	put_ppr(ppr);
	return ret;
}

struct ubmem_resource *preimport_get_resource_prefilled(void *handle)
{
	return ((struct prefilled_preimport_range *)handle)->ubmem_res;
}

void preimport_init_prefilled(void)
{
}

void preimport_exit_prefilled(void)
{
}
