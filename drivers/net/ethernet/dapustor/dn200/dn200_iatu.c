// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include "dn200.h"
#include "common.h"
#include "dn200_iatu.h"
#include "dn200_reg.h"

#define DN200_IATU_BAR_OFFSET 0x20000

static inline int dn200_iatu_cfg_get(struct dn200_priv *priv, u8 *iatu_base,
				     u8 *iatu_num)
{
	int idx = 0;
	u8 iatu_base_idx = 0, fun_iatu_num = 0;
	struct plat_dn200_data *plat_ex = priv->plat_ex;
	u8 pf_id;

	if (!priv->plat_ex)
		return -EINVAL;

	pf_id = priv->plat_ex->pf_id;
	/* add previous pfs iatu num to calculate current pf iatu base */
	for (idx = 0; idx < pf_id; idx++) {
		iatu_base_idx += priv->plat_ex->pf_max_iatu[idx];
		iatu_base_idx += priv->plat_ex->vf_total_iatu[idx];
	}

	if (PRIV_IS_VF(priv)) {
		struct dn200_vf_info info;

		dn200_get_vf_queue_info(plat_ex->pf.ioaddr, &info, pf_id,
					plat_ex->vf_offset);
		fun_iatu_num = info.iatu_num;
		/* add current pf iatu num to get vf iatu base */
		iatu_base_idx += priv->plat_ex->pf_max_iatu[pf_id];
		/* add previous vfs iatu num to calculate current vf iatu base */
		for (idx = 0; idx < plat_ex->vf_offset; idx++) {
			dn200_get_vf_queue_info(plat_ex->pf.ioaddr, &info,
						pf_id, idx);
			iatu_base_idx += info.iatu_num;
		}
		dev_dbg(priv->device,
			"%s, %d, pf_id %d, vf_offset:%d, iatu_base_idx:%d, fun_iatu_num:%d!!\n",
			__func__, __LINE__, pf_id, plat_ex->vf_offset,
			iatu_base_idx, fun_iatu_num);
	} else {
		fun_iatu_num = priv->plat_ex->pf_max_iatu[pf_id];
		dev_dbg(priv->device,
			"%s, %d, pf_id %d, vf_offset:%d, iatu_base_idx:%d, fun_iatu_num:%d!!\n",
			__func__, __LINE__, pf_id, plat_ex->vf_offset,
			iatu_base_idx, fun_iatu_num);
	}

	if (iatu_base_idx < 0 || iatu_base_idx >= DN200_MAX_IATU_TBL_SIZE) {
		dev_err(priv->device,
			"%s, %d, iatu_base:%d is invalid, pls chk pf_max_iatu & vf_total_iatu!!\n",
			__func__, __LINE__, iatu_base_idx);
		return -EINVAL;
	}

	if (fun_iatu_num < 1 || fun_iatu_num > DN200_MAX_IATU_TBL_SIZE) {
		dev_err(priv->device, "%s, %d, iatu num %d is invalid!!\n",
			__func__, __LINE__, fun_iatu_num);
		return -EINVAL;
	}
	*iatu_base = iatu_base_idx;
	*iatu_num = fun_iatu_num;

	return 0;
}

void dn200_axi_init_for_raid(struct dn200_priv *priv)
{
	u32 pf_axi_base_addr = 0;
	int i;

	/* to solve iatu base addr(32~40 bit:0xE0~0xFF) conflict with raid dma addr,
	 * should set 48th bit of 64bit dma address as 1 to deal with it,
	 * but xgmac just support 40 bit, so used axi bus register to
	 * set 48th bit dma addr as fixed 1 before route to pcie
	 * notes:
	 * 1. one xgmac have two axi addr (read & write) register,
	 *    every addr reg length is 0x4
	 * 2. xgmac 0 start from 0x20000344, other xgmac address is continuous from it
	 * 3. just append highest 24 bits to 40 bits dma address from xgamc to
	 *    compose 64 bit dma address
	 * 4. dma engine send out with 40 bits dma addr
	 *     -> axi regiter append highest 24bits to gen 64 bits addr
	 *     -> pcie receive 64 bits addr and iatu translate
	 *        highest 32 bit base address to target addr
	 */
	if (priv->plat_ex->raid_supported && !PRIV_IS_VF(priv)) {
		pf_axi_base_addr = DN200_AXI_HIGH_ADDR_REG_BASE +
					(priv->plat_ex->pf_id * 2) * 0x4;
		for (i = 0; i < 2; i++)
			fw_reg_write(&priv->plat_ex->ctrl,
				pf_axi_base_addr + (0x4 * i), DN200_AXI_HIGH_24BIT_VAL);
	}
}

void dn200_axi_uninit_for_raid(struct dn200_priv *priv)
{
	u32 pf_axi_base_addr = 0;
	int i;

	if (priv->plat_ex->raid_supported && !PRIV_IS_VF(priv)) {
		/* clear current pf axi high 24 bits address */
		pf_axi_base_addr = DN200_AXI_HIGH_ADDR_REG_BASE +
					(priv->plat_ex->pf_id * 2) * 0x4;
		for (i = 0; i < 2; i++)
			fw_reg_write(&priv->plat_ex->ctrl,
				pf_axi_base_addr + (0x4 * i), 0);
	}
}

static int dn200_iatu_tbl_init(struct dn200_priv *priv)
{
	int i;
	u8 pf_id = priv->plat_ex->pf_id;
	u8 max_iatu = 0;
	u64 iatu_base_addr = 0;

	struct dn200_func_iatu_basic_info *basic_info =
	    &priv->iatu_info.basic_info;
	struct dn200_priv_iatu_map *iatu_dma32 = &priv->iatu_info.dma32_info;
	struct dn200_iatu_tbl_entry *iatu_entry;
	u8 iatu_start_idx = 0;

	if (dn200_iatu_cfg_get(priv, &iatu_start_idx, &max_iatu) < 0)
		return -EINVAL;
	dev_dbg(priv->device,
		"%s, %d, pf:%d, iatu_start_idx:%d, max_iatu:%d.\n", __func__,
		__LINE__, pf_id, iatu_start_idx, max_iatu);

	/* iatu base addr used in two conditions for translating
	 *  desc dma addr ((base addr | iatu id) | (lowest 32bit addr))
	 *  to actual pcie target dma addr (target-dma addr high 32bit | lowest 32bit addr),
	 *  one iatu cover 4GB addr space
	 * 1. configured to pcie iatu table as region base
	 * 2. used as highest 8bit of dma addr of
	 *    ring descriptor(base addr | (dma addr & 0xffffffff)
	 */
	if (!priv->plat_ex->raid_supported)
		iatu_base_addr = DN200_BASE_IATU_ADDR;
	else
		iatu_base_addr = DN200_RAID_BASE_IATU_ADDR;


	for (i = iatu_start_idx; i < iatu_start_idx + max_iatu; i++) {
		if (i >= DN200_MAX_IATU_TBL_SIZE) {
			dev_err(priv->device,
				"%s, %d, iatu index:%d exceed max iatu tbl size:%d, iatu start:%d, max:%d.\n",
				__func__, __LINE__, i, DN200_MAX_IATU_TBL_SIZE,
				iatu_start_idx, max_iatu);
			break;
		}

		iatu_entry = &basic_info->tbl[i];
		iatu_entry->tgt_addr = 0;
		iatu_entry->base_addr =
		    ((iatu_base_addr >> MAX_LIMIT_RANGE_SHIFT) + i)
				<< MAX_LIMIT_RANGE_SHIFT;
		iatu_entry->iatu_offset = i;
		iatu_entry->limit_mask = LIMIT_MASK;
		/* dma32 iatu always use max limit mask */
		if (priv->dma32_iatu_used && i == iatu_dma32->iatu_index)
			iatu_entry->limit_mask = MAX_LIMIT_MASK;
		iatu_entry->pf_id = pf_id;

		if (PRIV_IS_VF(priv)) {
			iatu_entry->is_vf = true;
			iatu_entry->vf_offset = priv->plat_ex->vf_offset;
		} else {
			iatu_entry->is_vf = false;
			iatu_entry->vf_offset = 0;
		}
		dev_dbg(priv->device,
			"%s, %d, tgt:%#llx, base:%#llx, region:%d, limit:%#llx, pf:%d, is_vf:%d, vf id:%d.\n",
			__func__, __LINE__, iatu_entry->tgt_addr,
			iatu_entry->base_addr, iatu_entry->iatu_offset,
			iatu_entry->limit_mask, iatu_entry->pf_id,
			iatu_entry->is_vf, iatu_entry->vf_offset);

		dn200_iatu_tbl_entry_write(priv->ioaddr + DN200_IATU_BAR_OFFSET,
					   iatu_entry, i, true);
	}

	/* update dma32 target address that will not be changed */
	if (priv->dma32_iatu_used)
		dn200_iatu_tgt_addr_updt(priv->ioaddr + DN200_IATU_BAR_OFFSET,
					 iatu_dma32->iatu_index,
					 iatu_dma32->target_addr);
	return 0;
}

static void dn200_reclaim_queue_iatu(struct dn200_queue_iatu_info *queue_info)
{
	int i = 0;
	struct list_head *entry, *tmp;

	/*clean cached target to avoid reuse after freed */
	queue_info->cached.cached_target = 0;
	for (; i < DN200_MAX_IATU_MAP_PER_QUEUE; i++) {
		struct iatu_map_info *info = &queue_info->map_info[i];

		list_for_each_safe(entry, tmp, &info->list) {
			struct iatu_con_hashmap *con_info =
			    container_of(entry, struct iatu_con_hashmap, node);

			/* rx iatu exist forever and share with tx, no need to update and remove */
			if (con_info && !con_info->is_rx
			    && !atomic_read(&con_info->ref_count)) {
				atomic_sub(1, con_info->global_ref_ptr);
				list_del(&con_info->node);
				kfree(con_info);
			}
		}
	}
}

static void dn200_tx_iatu_reclaim(struct dn200_priv *priv, u32 queue_index)
{
	int i = 0;
	/* recycle iatu from all tx queues */
	for (; i < priv->plat->tx_queues_to_use; i++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[i];

		if (queue_index != i) {
			if (likely(atomic_wait_trysch(&tx_q->tx_scheduling))) {
				dn200_reclaim_queue_iatu(&tx_q->iatu_info);
				atomic_free_sch(&tx_q->tx_scheduling);
			}
		} else {
			dn200_reclaim_queue_iatu(&tx_q->iatu_info);
		}
	}
}

static inline int dn200_tx_iatu_alloc(struct dn200_priv *priv,
				      u64 tar_addr, u32 queue_index,
				      int *tx_map_index)
{
	struct dn200_priv_iatu_map *info;
	int i = 0;
	int unused_index = -1;
	bool need_update = false;
	bool retryed = false;
	u64 hw_tgt_addr;

	/*alloc iatu region from tx region */
	if (!spin_trylock_bh(&priv->iatu_info.tx_lock))
		return unused_index;

retry:
	for (i = 0; i < priv->iatu_info.basic_info.max_tx_iatu_num; i++) {
		info = &priv->iatu_info.tx_info[i];
		if (info->target_addr == tar_addr) {
			atomic_inc(&info->global_ref);
			need_update = false;
			unused_index = info->iatu_index;
			*tx_map_index = i;
			goto unlock;
		} else if (!atomic_read(&info->global_ref) && !need_update) {
			need_update = true;
			unused_index = info->iatu_index;
			*tx_map_index = i;
		}
	}
	if (need_update) {
		info = &priv->iatu_info.tx_info[*tx_map_index];
		info->target_addr = tar_addr;
		atomic_inc(&info->global_ref);
		hw_tgt_addr = tar_addr << CMP_ADDR_SHIFT;
		priv->swc.tx_iatu_updt_cnt++;
		dn200_iatu_tgt_addr_updt(priv->ioaddr + DN200_IATU_BAR_OFFSET,
					 unused_index, hw_tgt_addr);
	} else if (unused_index < 0 && !retryed) {
		/*no avalid iatu map, try reclaim iatu resources from all tx_queues */
		dn200_tx_iatu_reclaim(priv, queue_index);
		priv->swc.tx_iatu_recyc_cnt++;
		retryed = true;
		goto retry;
	}
unlock:
	spin_unlock_bh(&priv->iatu_info.tx_lock);
	return unused_index;
}

static inline int dn200_rx_iatu_match(struct dn200_priv *priv,
				      u64 tar_addr, int *rx_map_index)
{
	struct dn200_priv_iatu_map *info;
	int i = 0;
	int unused_index = -1;
	bool need_update = false;
	u64 hw_tgt_addr;

	for (i = 0; i < priv->iatu_info.basic_info.max_rx_iatu_num; i++) {
		info = &priv->iatu_info.rx_info[i];
		if (info->target_addr == tar_addr) {
			need_update = false;
			unused_index = info->iatu_index;
			*rx_map_index = i;
			goto unlock;
		} else if (!info->target_addr && !need_update) {
			need_update = true;
			unused_index = info->iatu_index;
			*rx_map_index = i;
		}
	}
	if (need_update) {
		info = &priv->iatu_info.rx_info[*rx_map_index];
		info->target_addr = tar_addr;
		info->is_rx = true;
		hw_tgt_addr = tar_addr << CMP_ADDR_SHIFT;
		dev_dbg(priv->device,
			"%s %d update tgt addr:%#llx, glob ref:%d, unused_index:%d, info:%p, ref:%p\n",
			__func__, __LINE__, hw_tgt_addr,
			atomic_read(&info->global_ref), unused_index, &info,
			&info->global_ref);
		dn200_iatu_tgt_addr_updt(priv->ioaddr + DN200_IATU_BAR_OFFSET,
					 unused_index, hw_tgt_addr);
	}
unlock:
	dev_dbg(priv->device, "%s, %d, tar_addr:%#llx, unused_index:%d.\n",
		__func__, __LINE__, tar_addr, unused_index);
	return unused_index;
}

static inline bool dn200_tx_iatu_match(struct dn200_tx_queue *tx_q,
				       u64 tar_addr, atomic_t **iatu_ref_ptr,
				       u64 *base_addr)
{
	u32 hash;
	struct list_head *entry, *tmp;
	struct iatu_map_info *hash_map;
	struct dn200_queue_iatu_info *queue_info = &tx_q->iatu_info;
	struct dn200_func_iatu_basic_info *basic_info =
	    &tx_q->priv_data->iatu_info.basic_info;

	hash = hash_32((u32) tar_addr, DN200_MAX_IATU_MAP_SHIFT);
	hash_map = &queue_info->map_info[hash];

	atomic_wait_sch(&tx_q->tx_scheduling);
	list_for_each_safe(entry, tmp, &hash_map->list) {
		struct iatu_con_hashmap *con_info =
		    container_of(entry, struct iatu_con_hashmap, node);

		if (con_info && con_info->target_addr == tar_addr) {
			if (!con_info->is_rx) {
				atomic_inc(&con_info->ref_count);
				*iatu_ref_ptr = &con_info->ref_count;
				queue_info->cached.ref_count_ptr =
				    &con_info->ref_count;
			}

			*base_addr =
			    basic_info->tbl[con_info->iatu_index].base_addr;
			atomic_free_sch(&tx_q->tx_scheduling);
			return true;
		}
	}
	atomic_free_sch(&tx_q->tx_scheduling);

	return false;
}

static inline bool dn200_tx_iatu_local_reuse(struct dn200_priv *priv,
					     struct dn200_queue_iatu_info
					     *queue_info, u64 tar_addr,
					     atomic_t **iatu_ref_ptr)
{
	int i = 0;
	struct list_head *entry, *tmp;
	bool find = false;
	struct dn200_priv_iatu_map *global_iatu;

	/*clean cached target to avoid reuse after freed */
	queue_info->cached.cached_target = 0;
	/*operate global iatu region, lock global */

	if (!spin_trylock_bh(&priv->iatu_info.tx_lock))
		return false;

	for (; i < DN200_MAX_IATU_MAP_PER_QUEUE; i++) {
		struct iatu_map_info *info = &queue_info->map_info[i];

		list_for_each_safe(entry, tmp, &info->list) {
			struct iatu_con_hashmap *con_info =
			    container_of(entry, struct iatu_con_hashmap, node);

			/*find a iatu map which only used by self */
			if (con_info && !atomic_read(&con_info->ref_count)
			    && (atomic_read(con_info->global_ref_ptr) == 1)) {
				global_iatu =
				    container_of(con_info->global_ref_ptr,
						struct dn200_priv_iatu_map,
						global_ref);

				*iatu_ref_ptr = &con_info->ref_count;
				atomic_inc(&con_info->ref_count);
				/*update iatu map's target addr */
				con_info->target_addr = tar_addr;
				global_iatu->target_addr = tar_addr;
				dn200_iatu_tgt_addr_updt(priv->ioaddr + DN200_IATU_BAR_OFFSET,
							 con_info->iatu_index,
							 tar_addr);
				find = true;
				goto unlock;
			}
		}
	}
unlock:
	spin_unlock_bh(&priv->iatu_info.tx_lock);
	return find;
}

int dn200_tx_iatu_find(u64 tar_addr, struct dn200_tx_queue *tx_q,
		       atomic_t **iatu_ref_ptr, u64 *base_addr)
{
	int res = 0, ret = 0, tx_map_idx = 0;
	struct iatu_con_hashmap *con_info = NULL;
	struct iatu_map_info *info = NULL;
	struct dn200_priv *priv = tx_q->priv_data;
	struct dn200_func_iatu_basic_info *basic_info =
	    &priv->iatu_info.basic_info;

	priv->swc.tx_iatu_find_cnt++;
	*iatu_ref_ptr = NULL;

	/*all high bits are zero, use default iatu region */
	if (tar_addr >> MAX_LIMIT_RANGE_SHIFT == 0) {
		*base_addr = tar_addr;
		if (tx_q->priv_data->dma32_iatu_used) {
			u16 dma32_iatu_idx =
			    tx_q->priv_data->iatu_info.dma32_info.iatu_index;
			*base_addr = basic_info->tbl[dma32_iatu_idx].base_addr;
		}
		return 0;
	}

	tar_addr = (tar_addr >> CMP_ADDR_SHIFT);
	if (dn200_tx_iatu_match(tx_q, tar_addr, iatu_ref_ptr, base_addr)) {
		priv->swc.tx_iatu_match_cnt++;
		return 0;
	}

	atomic_wait_sch(&tx_q->tx_scheduling);
	con_info = kzalloc(sizeof(struct iatu_con_hashmap), GFP_ATOMIC);
	if (unlikely(!con_info)) {
		dev_err(tx_q->priv_data->device, "%s %d alloc failure.\n",
			__func__, __LINE__);
		res = -ENOMEM;
		goto free_sch;
	}
	ret =
	    dn200_tx_iatu_alloc(tx_q->priv_data, tar_addr, tx_q->queue_index,
				&tx_map_idx);
	if (ret < 0) {
		kfree(con_info);
		res = -ENOSPC;
		goto free_sch;
	}
	info =
	    &tx_q->iatu_info.map_info[hash_32((u32) tar_addr, DN200_MAX_IATU_MAP_SHIFT)];
	con_info->iatu_index = ret;
	con_info->target_addr = tar_addr;
	atomic_set(&con_info->ref_count, 0);
	atomic_inc(&con_info->ref_count);
	*iatu_ref_ptr = &con_info->ref_count;
	con_info->global_ref_ptr =
	    &tx_q->priv_data->iatu_info.tx_info[tx_map_idx].global_ref;
    /*barrier to protect gloval_ref_ptr's value available*/
	smp_mb();
	list_add_tail(&con_info->node, &info->list);
	*base_addr = basic_info->tbl[ret].base_addr;
free_sch:
	atomic_free_sch(&tx_q->tx_scheduling);
	return res;
}

int dn200_rx_iatu_find(u64 tar_addr, struct dn200_priv *priv, u64 *base_addr)
{
	int res = 0, ret = 0, rx_map_idx = 0;
	u64 dma_addr = 0;
	struct dn200_func_iatu_basic_info *basic_info =
	    &priv->iatu_info.basic_info;

	if (dma_can_direct_use(priv, tar_addr)) {
		*base_addr = tar_addr;
		return 0;
	}
	/*all high bits are zero, use default iatu region */
	if ((tar_addr >> MAX_LIMIT_RANGE_SHIFT) == 0) {
		*base_addr = tar_addr;
		if (priv->dma32_iatu_used) {
			u16 dma32_iatu_idx =
			    priv->iatu_info.dma32_info.iatu_index;

			*base_addr = basic_info->tbl[dma32_iatu_idx].base_addr | (tar_addr & ((u64) BIT(32) - 1));
		}
		return 0;
	}
	dma_addr = (tar_addr >> CMP_ADDR_SHIFT);

	ret = dn200_rx_iatu_match(priv, dma_addr, &rx_map_idx);
	if (ret < 0) {
		dev_dbg(priv->device,
			"%s %d dma_addr %#llx alloc iatu failed\n", __func__,
			__LINE__, dma_addr);
		res = -ENOSPC;
		goto free_sch;
	}
	*base_addr = basic_info->tbl[ret].base_addr | (tar_addr & LIMIT_MASK);
	pr_debug("%s, %d, tar_addr %#llx base addr:%#llx\n", __func__, __LINE__,
		 tar_addr, *base_addr);

free_sch:
	return res;
}

static void dn200_display_queue_iatu(struct dn200_priv *priv,
				     struct dn200_queue_iatu_info *queue_info,
				     struct seq_file *seq)
{
	int i = 0;
	struct list_head *entry, *tmp;

	for (; i < DN200_MAX_IATU_MAP_PER_QUEUE; i++) {
		struct iatu_map_info *info = &queue_info->map_info[i];

		list_for_each_safe(entry, tmp, &info->list) {
			struct iatu_con_hashmap *con_info =
			    container_of(entry, struct iatu_con_hashmap, node);

			if (!con_info->is_rx) {
				seq_printf(seq,
					   "  index %d, iatu index:%d, tar_addr %llx, local ref_cnt %d, global ref %d\n",
					   i, con_info->iatu_index,
					   con_info->target_addr,
					   atomic_read(&con_info->ref_count),
					   atomic_read(con_info->global_ref_ptr));
			}
		}
	}
}

void dn200_iatu_display(struct dn200_priv *priv, struct seq_file *seq)
{
	struct dn200_priv_iatu_map *info;
	int i = 0;
	u16 total_iatu = priv->iatu_info.basic_info.max_tx_iatu_num +
	    priv->iatu_info.basic_info.max_rx_iatu_num;

	if (priv->dma32_iatu_used)
		total_iatu++;

	seq_printf(seq, "====== fun:%d, total iatu:%d ======\n",
		   priv->plat_ex->funcid, total_iatu);

	seq_printf(seq, "=== tx iatu total:%d ===\n",
		   priv->iatu_info.basic_info.max_tx_iatu_num);
	for (i = 0; i < priv->iatu_info.basic_info.max_tx_iatu_num; i++) {
		info = &priv->iatu_info.tx_info[i];
		seq_printf(seq,
			   "index %d, iatu index:%d, target_addr %llx, global_ref %d, from rx:%s\n",
			   i, info->iatu_index, info->target_addr,
			   atomic_read(&info->global_ref),
			   info->is_rx ? "yes" : "no");
	}

	seq_printf(seq, "=== rx iatu total:%d ===\n",
		   priv->iatu_info.basic_info.max_rx_iatu_num);
	for (i = 0; i < priv->iatu_info.basic_info.max_rx_iatu_num; i++) {
		info = &priv->iatu_info.rx_info[i];
		seq_printf(seq, "index %d, iatu index:%d, target_addr %llx\n",
			   i, info->iatu_index, info->target_addr);
	}

	seq_printf(seq, "=== dma32 iatu used:%d, iatu index:%d ===\n",
		   priv->dma32_iatu_used,
		   priv->iatu_info.dma32_info.iatu_index);

	seq_puts(seq, "=== tx iatu per queue: ===\n");
	for (i = 0; i < priv->plat->tx_queues_to_use; i++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[i];

		if (likely(atomic_wait_trysch(&tx_q->tx_scheduling))) {
			seq_printf(seq, "%s queue index %d :\n", __func__, i);
			dn200_display_queue_iatu(priv, &tx_q->iatu_info, seq);
			atomic_free_sch(&tx_q->tx_scheduling);
		}
	}
}

static int dn200_add_rx_iatu2tx_queue(struct dn200_priv *priv, u8 queue_id,
				      u8 *rx_num)
{
	int i = 0;
	struct dn200_priv_iatu_map *info;
	struct iatu_map_info *list_info;
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue_id];

	for (i = 0; i < priv->iatu_info.basic_info.max_rx_iatu_num; i++) {
		info = &priv->iatu_info.rx_info[i];
		if (info->target_addr) {
			struct iatu_con_hashmap *con_info =
			    kzalloc(sizeof(struct iatu_con_hashmap),
				    GFP_ATOMIC);

			if (unlikely(!con_info)) {
				dev_err(priv->device, "%s %d alloc failure.\n",
					__func__, __LINE__);
				return 0;
			}

			list_info =
			    &tx_q->iatu_info.map_info[hash_32((u32) info->target_addr, DN200_MAX_IATU_MAP_SHIFT)];
			con_info->iatu_index = info->iatu_index;
			con_info->target_addr = info->target_addr;
			con_info->is_rx = true;
			dev_dbg(priv->device,
				"%s %d iatu index %d target_addr %#llx queue_id %d\n",
				__func__, __LINE__, info->iatu_index,
				info->target_addr, queue_id);
			/*barrier to protect con_info value available*/
			smp_mb();
			list_add_tail(&con_info->node, &list_info->list);
		} else {
			*rx_num = i;
			break;
		}
	}
	return 0;
}

int dn200_add_rx_iatu2tx(struct dn200_priv *priv)
{
	int i = 0;
	u8 rx_num = 0;
	u8 unused_num = 0;
	struct dn200_priv_iatu_info *iatu_info = &priv->iatu_info;
	struct dn200_priv_iatu_map *tx_iatu_map;
	struct dn200_priv_iatu_map *rx_iatu_map;
	int max_rx_iatu_num = priv->iatu_info.basic_info.max_rx_iatu_num;
	int max_tx_iatu_num = priv->iatu_info.basic_info.max_tx_iatu_num;

	rx_num = max_rx_iatu_num;
	for (; i < priv->plat->tx_queues_to_use; i++) {
		if (dn200_add_rx_iatu2tx_queue(priv, i, &rx_num))
			return 0;
	}
	unused_num = max_rx_iatu_num - rx_num;
	if (unused_num) {
		dev_dbg(priv->device, "%s %d rx use %d put unused %d to tx\n",
			__func__, __LINE__, rx_num, unused_num);
		i = 0;
		for (; i < unused_num; i++) {
			tx_iatu_map = &iatu_info->tx_info[max_tx_iatu_num + i];
			rx_iatu_map = &iatu_info->rx_info[rx_num + i];
			tx_iatu_map->iatu_index = rx_iatu_map->iatu_index;
			tx_iatu_map->is_rx = true;
			dev_dbg(priv->device,
				"%s %d addr rx index %d iatu_index %d to tx_index %d\n",
				__func__, __LINE__, rx_num + i,
				rx_iatu_map->iatu_index, max_tx_iatu_num + i);
		}

		priv->iatu_info.basic_info.max_tx_iatu_num += unused_num;
		priv->iatu_info.basic_info.max_rx_iatu_num -= unused_num;
	}

	return 0;
}

int dn200_iatu_init(struct dn200_priv *priv)
{
	int i = 0, j = 0;
	u8 max_iatu = 0;
	u8 dma32_iatu_base = 0, rx_iatu_base = 0, tx_iatu_base = 0;
	struct dn200_priv_iatu_info *iatu_info = &priv->iatu_info;
	struct dn200_func_iatu_basic_info *basic_info = &iatu_info->basic_info;
	struct dn200_priv_iatu_map *iatu_map;	/*global iatu map info per function */
	struct dn200_queue_iatu_info *queue_iatu;	/*iatu map info per queue */
	u8 max_rx_iatu, max_tx_iatu;

	basic_info->max_tx_iatu_num = 0;
	basic_info->max_rx_iatu_num = 0;
	memset(iatu_info, 0, sizeof(struct dn200_priv_iatu_info));
	spin_lock_init(&iatu_info->rx_lock);
	spin_lock_init(&iatu_info->tx_lock);

	if (dn200_iatu_cfg_get(priv, &tx_iatu_base, &max_iatu) < 0)
		return -EINVAL;

	max_tx_iatu = (max_iatu >> 1);
	max_tx_iatu = min_t(u8, max_tx_iatu, (u8) DN200_MAX_IATU_MAP_TX);
	max_rx_iatu = (max_iatu - max_tx_iatu - 1);
	max_rx_iatu = min_t(u8, max_rx_iatu, (u8) DN200_MAX_IATU_MAP_RX);

	/* if pf support raid or it is vf, should set dma32 iatu
	 * to solve address confict between raid and pf/vf
	 * the iatu is also used for vf outband address routing
	 */
	if (priv->plat_ex->raid_supported || PRIV_IS_VF(priv))
		priv->dma32_iatu_used = true;

	/* for pf without raid, no need to reserve dma32 iatu, just use it for tx */
	if (!priv->dma32_iatu_used)
		max_tx_iatu += 1;

	basic_info->max_tx_iatu_num = max_tx_iatu;
	basic_info->max_rx_iatu_num = max_rx_iatu;
	/* tx_iatu_base ~ (tx_iatu_base + max_tx_iatu - 1) used by tx queues */
	for (i = 0; i < max_tx_iatu; i++) {
		iatu_map = &iatu_info->tx_info[i];
		iatu_map->iatu_index = i + tx_iatu_base;
		iatu_map->target_addr = 0;
		atomic_set(&iatu_map->global_ref, 0);
		dev_dbg(priv->device,
			"%s, %d, tx i:%d, iatu_index:%d, max_tx_iatu:%d, tx_iatu_base:%d!\n",
			__func__, __LINE__, i, iatu_map->iatu_index,
			max_tx_iatu, tx_iatu_base);
	}

	/* iatu index (rx_iatu_base) ~ (rx_iatu_base + rx_iatu_base - 1) used by rx queues */
	rx_iatu_base = tx_iatu_base + max_tx_iatu;
	for (i = 0; i < max_rx_iatu; i++) {
		iatu_map = &iatu_info->rx_info[i];
		iatu_map->iatu_index = i + rx_iatu_base;
		iatu_map->target_addr = 0;
		atomic_set(&iatu_map->global_ref, 0);
		dev_dbg(priv->device,
			"%s, %d, rx i:%d, iatu_index:%d, max_rx_iatu:%d, rx_iatu_base:%d!\n",
			__func__, __LINE__, i, iatu_map->iatu_index,
			max_rx_iatu, rx_iatu_base);
	}

	/* last iatu index used by default dma32 in two conditions: for vf or pf support raid */
	if (priv->dma32_iatu_used) {
		dma32_iatu_base = rx_iatu_base + max_rx_iatu;
		iatu_info->dma32_info.iatu_index = dma32_iatu_base;
		iatu_info->dma32_info.target_addr = 0;
		dev_dbg(priv->device,
			"%s, %d, dma32_iatu_base or iatu_index:%d!\n", __func__,
			__LINE__, iatu_info->dma32_info.iatu_index);
	}

	for (i = 0; i < priv->plat->tx_queues_to_use; i++) {
		queue_iatu = &priv->tx_queue[i].iatu_info;
		for (j = 0; j < DN200_MAX_IATU_MAP_PER_QUEUE; j++) {
			struct iatu_map_info *map_info =
			    &queue_iatu->map_info[j];

			INIT_LIST_HEAD(&map_info->list);
		}
	}

	for (i = 0; i < priv->plat->rx_queues_to_use; i++) {
		queue_iatu = &priv->rx_queue[i].iatu_info;
		for (j = 0; j < DN200_MAX_IATU_MAP_PER_QUEUE; j++) {
			struct iatu_map_info *map_info =
			    &queue_iatu->map_info[j];

			INIT_LIST_HEAD(&map_info->list);
		}
	}

	if (dn200_iatu_tbl_init(priv) < 0)
		return -EINVAL;

	set_bit(DN200_IATU_INIT, &priv->state);
	return 0;
}

static void dn200_queue_iatu_free(struct dn200_priv *priv,
				  struct dn200_queue_iatu_info *queue_info)
{
	int i = 0;
	struct list_head *entry, *tmp;

	for (; i < DN200_MAX_IATU_MAP_PER_QUEUE; i++) {
		struct iatu_map_info *info = &queue_info->map_info[i];

		if (list_empty(&info->list) || !info->list.next)
			continue;
		list_for_each_safe(entry, tmp, &info->list) {
			struct iatu_con_hashmap *con_info =
			    container_of(entry, struct iatu_con_hashmap, node);
			if (con_info) {
				list_del(&con_info->node);
				kfree(con_info);
			}
		}
		INIT_LIST_HEAD(&info->list);
	}
	memset(queue_info->map_info, 0, sizeof(queue_info->map_info));
}

static int dn200_iatu_tbl_clear(struct dn200_priv *priv)
{
	int i;
	u8 pf_id = priv->plat_ex->pf_id;
	u8 max_iatu = 0;
	struct dn200_iatu_tbl_entry iatu_entry;
	u8 iatu_start_idx = 0;

	if (dn200_iatu_cfg_get(priv, &iatu_start_idx, &max_iatu) < 0)
		return -EINVAL;

	for (i = iatu_start_idx; i < iatu_start_idx + max_iatu; i++) {
		if (i >= DN200_MAX_IATU_TBL_SIZE) {
			dev_err(priv->device,
					"%s, %d, iatu index:%d exceed max iatu tbl size:%d, iatu start:%d, max:%d.\n",
					__func__, __LINE__, i, DN200_MAX_IATU_TBL_SIZE,
					iatu_start_idx, max_iatu);
			break;
		}

		iatu_entry.tgt_addr = 0;
		iatu_entry.base_addr = 0;
		iatu_entry.iatu_offset = i;
		iatu_entry.limit_mask = 0;
		iatu_entry.pf_id = pf_id;
		iatu_entry.is_vf = 0;
		iatu_entry.vf_offset = 0;

		dn200_iatu_tbl_entry_write(priv->ioaddr + DN200_IATU_BAR_OFFSET,
									&iatu_entry, i, false);
	}

	return 0;
}

void dn200_iatu_uninit(struct dn200_priv *priv)
{
	int i = 0;
	u8 tx_iatu_base, max_iatu;

	if (dn200_iatu_cfg_get(priv, &tx_iatu_base, &max_iatu) < 0)
		return;

	for (; i < priv->plat->tx_queues_to_use; i++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[i];

		atomic_wait_sch(&tx_q->tx_scheduling);
		dn200_queue_iatu_free(priv, &tx_q->iatu_info);
		atomic_free_sch(&tx_q->tx_scheduling);
	}
	dn200_iatu_tbl_clear(priv);
	clear_bit(DN200_IATU_INIT, &priv->state);
}
