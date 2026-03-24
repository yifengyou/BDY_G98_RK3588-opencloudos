// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma urma resource management
 */
#include <linux/timer.h>
#include "ipourma_err.h"
#include "ipourma_ub.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_res.h"

u32 ipourma_tx_ring_size __read_mostly = IPOURMA_TX_RING_SIZE;
u32 ipourma_rx_ring_size __read_mostly = IPOURMA_RX_RING_SIZE;

STATIC void ipourma_uninit_tx_bufs(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	if (IS_ERR_OR_NULL(priv->tx_ring[eid_idx]))
		return;
	for (u32 i = 0; i < ipourma_tx_ring_size; i++) {
		priv->tx_ring[eid_idx][i].seg[0] = NULL;
		priv->tx_ring[eid_idx][i].buf_aligned = NULL;
	}

	if (!IS_ERR_OR_NULL(priv->ipourma_ub_tx_seg[eid_idx])) {
		for (size_t i = 0; i < priv->tx_buf_num; i++) {
			if (IS_ERR_OR_NULL(priv->ipourma_ub_tx_seg[eid_idx][i]))
				continue;
			ubcore_unregister_seg(priv->ipourma_ub_tx_seg[eid_idx][i]);
			priv->ipourma_ub_tx_seg[eid_idx][i] = NULL;
		}
		kfree(priv->ipourma_ub_tx_seg[eid_idx]);
		priv->ipourma_ub_tx_seg[eid_idx] = NULL;
	}

	if (!IS_ERR_OR_NULL(priv->tx_buf_aligned[eid_idx])) {
		for (size_t i = 0; i < priv->tx_buf_num; i++) {
			if (IS_ERR_OR_NULL(priv->tx_buf_aligned[eid_idx][i]))
				continue;
			kfree(priv->tx_buf_aligned[eid_idx][i]);
			priv->tx_buf_aligned[eid_idx][i] = NULL;
		}
		kfree(priv->tx_buf_aligned[eid_idx]);
		priv->tx_buf_aligned[eid_idx] = NULL;
	}
}

void ipourma_uninit_rx_bufs(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	for (u32 i = 0; i < ipourma_rx_ring_size; i++) {
		if (!IS_ERR_OR_NULL(priv->rx_ring[eid_idx][i].seg[0]))
			priv->rx_ring[eid_idx][i].seg[0] = NULL;
		if (!IS_ERR_OR_NULL(priv->rx_ring[eid_idx][i].buf_aligned))
			priv->rx_ring[eid_idx][i].buf_aligned = NULL;

		if (!IS_ERR_OR_NULL(priv->rx_ring[eid_idx][i].skb_pass_up)) {
			dev_kfree_skb_any(priv->rx_ring[eid_idx][i].skb_pass_up);
			priv->rx_ring[eid_idx][i].skb_pass_up = NULL;
		}
	}
	if (IS_ERR_OR_NULL(priv->ipourma_ub_rx_seg[eid_idx]))
		return;
	for (u32 i = 0; i < priv->rx_buf_num; i++) {
		if (IS_ERR_OR_NULL(priv->ipourma_ub_rx_seg[eid_idx][i]))
			continue;
		ubcore_unregister_seg(priv->ipourma_ub_rx_seg[eid_idx][i]);
		priv->ipourma_ub_rx_seg[eid_idx][i] = NULL;
		if (IS_ERR_OR_NULL(priv->rx_buf_aligned[eid_idx][i]))
			continue;
		kfree(priv->rx_buf_aligned[eid_idx][i]);
		priv->rx_buf_aligned[eid_idx][i] = NULL;
	}
	kfree(priv->ipourma_ub_rx_seg[eid_idx]);
	priv->ipourma_ub_rx_seg[eid_idx] = NULL;
}

void ipourma_uninit_rings_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	/* this function will be filled in the next commit */
}

int ipourma_init_tjetty_hmap(struct net_device *dev)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct ipourma_tjetty_hmap *tjetty_hmap = &priv->tjetty_lru.tjetty_hmap;
	int ret = IPOURMA_OK;

	tjetty_hmap->hash_seed = get_random_u32();
	tjetty_hmap->buckets =
		kcalloc(IPOURMA_TJETTY_HMAP_SIZE, sizeof(struct hlist_head), GFP_KERNEL);
	if (IS_ERR_OR_NULL(tjetty_hmap->buckets)) {
		netdev_warn(priv->dev, "%s\n",
			ipourma_err_desc(IPOURMA_INIT_TJETTY_HMAP_FAILED));
		return IPOURMA_INIT_TJETTY_HMAP_FAILED;
	}

	for (int i = 0; i < IPOURMA_TJETTY_HMAP_SIZE; i++)
		INIT_HLIST_HEAD(tjetty_hmap->buckets + i);

	ipourma_init_tjetty_aging_work(&priv->tjetty_lru);

	return ret;
}

STATIC int ipourma_alloc_tx_buf_aligned(struct ipourma_dev_priv *priv, u32 eid_idx, u32 idx)
{
	u32 offset = (idx * priv->tx_buf_size) % IPOURMA_REGISTER_SEG_SIZE;
	u32 blk_idx = idx * priv->tx_buf_size / IPOURMA_REGISTER_SEG_SIZE;
	struct ipourma_tx_buf *tx_buf = &priv->tx_ring[eid_idx][idx];

	tx_buf->buf_aligned = priv->tx_buf_aligned[eid_idx][blk_idx] + offset;

	if (IS_ERR_OR_NULL(tx_buf->buf_aligned) ||
		(u64)(tx_buf->buf_aligned) % IPOURMA_SEGMENT_ALIGN_SIZE != 0) {
		tx_buf->buf_aligned = NULL;
		netdev_warn(priv->dev, "%s: addr = 0x%llx, align = %d\n",
					ipourma_err_desc(IPOURMA_ADDRESS_NOT_ALIGNED),
					(u64)tx_buf->buf_aligned, IPOURMA_SEGMENT_ALIGN_SIZE);
		return IPOURMA_ADDRESS_NOT_ALIGNED;
	}

	tx_buf->seg[0] = priv->ipourma_ub_tx_seg[eid_idx][blk_idx];
	tx_buf->tx_sge[0].addr = (u64)tx_buf->buf_aligned;
	tx_buf->tx_sge[0].tseg = tx_buf->seg[0];

	return IPOURMA_OK;
}

STATIC inline void ipourma_init_tx_wr(struct ipourma_tx_buf *tx_buf)
{
	/* only init the fixed fields */
	tx_buf->tx_wr.user_ctx = tx_buf->idx;
	tx_buf->tx_wr.opcode = UBCORE_OPC_SEND;
	tx_buf->tx_wr.send.src.sge = tx_buf->tx_sge;
	tx_buf->tx_wr.flag.bs.complete_enable = 1;
}

STATIC inline void ipourma_init_rx_wr(struct ipourma_rx_buf *rx_buf)
{
	/* only init the fixed fields */
	rx_buf->rx_wr.user_ctx = rx_buf->idx;
	rx_buf->rx_wr.src.sge = rx_buf->rx_sge;
	rx_buf->rx_wr.src.num_sge = IPOURMA_MAX_RX_SGES;
}

STATIC int ipourma_init_tx_bufs(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	struct ubcore_seg_cfg cfg = { 0 };
	int ret = IPOURMA_OK;
	u32 i;

	priv->tx_buf_aligned[eid_idx] = kcalloc(priv->tx_buf_num, sizeof(u8 *), GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->tx_buf_aligned[eid_idx]))
		goto alloc_tx_bufs_failed;
	priv->ipourma_ub_tx_seg[eid_idx] = kcalloc(priv->tx_buf_num,
						   sizeof(struct ubcore_target_seg **),
						   GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->ipourma_ub_tx_seg[eid_idx]))
		goto alloc_tx_bufs_failed;
	for (i = 0; i < priv->tx_buf_num; i++) {
		priv->tx_buf_aligned[eid_idx][i] = kzalloc(IPOURMA_REGISTER_SEG_SIZE, GFP_KERNEL);
		if (IS_ERR_OR_NULL(priv->tx_buf_aligned[eid_idx][i]))
			goto alloc_tx_bufs_failed;
		ipourma_build_seg_cfg(&cfg, (u64)priv->tx_buf_aligned[eid_idx][i],
						IPOURMA_REGISTER_SEG_SIZE);
		priv->ipourma_ub_tx_seg[eid_idx][i] = ubcore_register_seg(priv->urma_dev,
									  &cfg, NULL);
		if (IS_ERR_OR_NULL(priv->ipourma_ub_tx_seg[eid_idx][i]))
			goto alloc_tx_bufs_failed;
	}
	for (i = 0; i < ipourma_tx_ring_size; i++) {
		priv->tx_ring[eid_idx][i].priv = priv;
		priv->tx_ring[eid_idx][i].idx = i;
		priv->tx_ring[eid_idx][i].eid_index = eid_idx;
		INIT_WORK(&(priv->tx_ring[eid_idx][i].work), ipourma_post_send);
		ipourma_init_tx_wr(&(priv->tx_ring[eid_idx][i]));
		ret = ipourma_alloc_tx_buf_aligned(priv, eid_idx, i);
		if (ret != IPOURMA_OK)
			goto alloc_tx_bufs_failed;
	}

	return ret;
alloc_tx_bufs_failed:
	ipourma_uninit_tx_bufs(priv, eid_idx);
	return IPOURMA_ADDRESS_NOT_ALIGNED;
}

STATIC int ipourma_init_rx_bufs(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	struct ubcore_seg_cfg cfg = { 0 };
	size_t i;
	int ret;

	ret = IPOURMA_OK;
	priv->rx_buf_aligned[eid_idx] = kcalloc(priv->rx_buf_num, sizeof(u8 *), GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->rx_buf_aligned[eid_idx]))
		return IPOURMA_ADDRESS_NOT_ALIGNED;
	for (i = 0; i < priv->rx_buf_num; i++) {
		priv->rx_buf_aligned[eid_idx][i] = kzalloc(IPOURMA_REGISTER_SEG_SIZE,
							   GFP_KERNEL);
		if (IS_ERR_OR_NULL(priv->rx_buf_aligned[eid_idx][i])) {
			ret = IPOURMA_ADDRESS_NOT_ALIGNED;
			goto alloc_rx_buf_aligned_err;
		}
	}
	priv->ipourma_ub_rx_seg[eid_idx] = kcalloc(priv->rx_buf_num,
				sizeof(struct ubcore_target_seg *), GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->ipourma_ub_rx_seg[eid_idx])) {
		ret = IPOURMA_ADDRESS_NOT_ALIGNED;
		goto alloc_rx_seg_err;
	}
	for (i = 0; i < priv->rx_buf_num; i++) {
		ipourma_build_seg_cfg(&cfg, (u64)priv->rx_buf_aligned[eid_idx][i],
							IPOURMA_REGISTER_SEG_SIZE);
		priv->ipourma_ub_rx_seg[eid_idx][i] = ubcore_register_seg(priv->urma_dev,
										&cfg, NULL);
		if (IS_ERR_OR_NULL(priv->ipourma_ub_rx_seg[eid_idx][i])) {
			ret = IPOURMA_ADDRESS_NOT_ALIGNED;
			goto reg_rx_seg_err;
		}
	}

	for (i = 0; i < ipourma_rx_ring_size; i++) {
		priv->rx_ring[eid_idx][i].priv = priv;
		priv->rx_ring[eid_idx][i].idx = i;
		priv->rx_ring[eid_idx][i].eid_index = eid_idx;
		INIT_WORK(&(priv->rx_ring[eid_idx][i].work), ipourma_replenish_segments);
		ipourma_init_rx_wr(&(priv->rx_ring[eid_idx][i]));
	}

	return IPOURMA_OK;
reg_rx_seg_err:
	for (i--; i >= 0; i--) {
		if (IS_ERR_OR_NULL(priv->ipourma_ub_rx_seg[eid_idx][i]))
			continue;
		ubcore_unregister_seg(priv->ipourma_ub_rx_seg[eid_idx][i]);
		priv->ipourma_ub_rx_seg[eid_idx][i] = NULL;
		if (i == 0)
			break;
	}
	kfree(priv->ipourma_ub_rx_seg[eid_idx]);
	priv->ipourma_ub_rx_seg[eid_idx] = NULL;
alloc_rx_seg_err:
	i = priv->rx_buf_num;
alloc_rx_buf_aligned_err:
	for (i--; i >= 0; i--) {
		if (IS_ERR_OR_NULL(priv->rx_buf_aligned[eid_idx][i]))
			continue;
		kfree(priv->rx_buf_aligned[eid_idx][i]);
		priv->rx_buf_aligned[eid_idx][i] = NULL;
		if (i == 0)
			break;
	}
	kfree(priv->rx_buf_aligned[eid_idx]);
	priv->rx_buf_aligned[eid_idx] = NULL;
	return IPOURMA_ADDRESS_NOT_ALIGNED;
}

STATIC void cleanup_tx_ring_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	struct ipourma_tx_buf *tx_ring = priv->tx_ring[eid_idx];
	unsigned long flags;

	if (IS_ERR_OR_NULL(tx_ring))
		return;

	spin_lock_irqsave(&priv->tx_ring_locks[eid_idx], flags);
	priv->tx_head[eid_idx] = 0;
	priv->tx_tail[eid_idx] = 0;
	priv->tx_ring_is_full[eid_idx] = false;
	for (u32 i = 0; i < ipourma_tx_ring_size; i++) {
		if (unlikely(tx_ring[i].tx_buf_in_use == 1)) {
			tx_ring[i].tx_buf_in_use = 0;
			dev_kfree_skb_any(tx_ring[i].skb);
		}
	}
	spin_unlock_irqrestore(&priv->tx_ring_locks[eid_idx], flags);
}

int ipourma_init_rings_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	int ret = IPOURMA_OK;

	priv->tx_ring[eid_idx] = vzalloc(sizeof(struct ipourma_tx_buf) * ipourma_tx_ring_size);
	if (IS_ERR_OR_NULL(priv->tx_ring[eid_idx]))
		return IPOURMA_ALLOC_TX_RING_FAILED;

	ret = ipourma_init_tx_bufs(priv, eid_idx);
	if (ret != IPOURMA_OK)
		goto init_tx_bufs_failed;

	priv->rx_ring[eid_idx] = kcalloc(ipourma_rx_ring_size, sizeof(struct ipourma_rx_buf),
								GFP_KERNEL);
	if (IS_ERR_OR_NULL(priv->rx_ring[eid_idx]))
		goto alloc_rx_ring_failed;

	ret = ipourma_init_rx_bufs(priv, eid_idx);
	if (ret != IPOURMA_OK)
		goto init_rx_bufs_failed;

	spin_lock_init(&priv->tx_ring_locks[eid_idx]);
	return ret;

init_rx_bufs_failed:
	kfree(priv->rx_ring[eid_idx]);
	priv->rx_ring[eid_idx] = NULL;
alloc_rx_ring_failed:
	ipourma_uninit_tx_bufs(priv, eid_idx);
init_tx_bufs_failed:
	vfree(priv->tx_ring[eid_idx]);
	priv->tx_ring[eid_idx] = NULL;
	return ret;
}

STATIC void ipourma_reset_tx_bufs_by_eid(struct ipourma_dev_priv *priv,
					 struct ubcore_cr *cr, u32 eid_idx)
{
	struct ubcore_jetty_attr attr = {
		.mask = UBCORE_JETTY_STATE,
		.state = UBCORE_JETTY_STATE_ERROR,
	};
	struct net_device *dev = priv->dev;
	int tx_cr_num = 0;

	if (IS_ERR_OR_NULL(priv->jetty[eid_idx]))
		return;

	/* Clear the SQEs that have not been processed by the hardware */
	tx_cr_num = ubcore_flush_jetty(priv->jetty[eid_idx], ipourma_tx_ring_size, cr);
	if (unlikely(tx_cr_num < 0)) {
		netdev_err(dev, "%s\n", ipourma_err_desc(IPOURMA_FLUSH_JETTY_FAILED));
		return;
	}
	for (int j = 0; j < tx_cr_num; j++) {
		priv->runtime_stats.tx_stats.cqe_recved++;
		ipourma_handle_tx_wc(dev, priv, &cr[j]);
	}

	/* Clear the SQEs currently being processed by the hardware */
	if (unlikely(ubcore_modify_jetty(priv->jetty[eid_idx], &attr, NULL) != 0)) {
		netdev_err(dev, "%s\n", ipourma_err_desc(IPOURMA_MODIFY_JETTY_FAILED));
		return;
	}
	tx_cr_num = ubcore_poll_jfc(priv->tx_jfc, ipourma_tx_ring_size, cr);
	if (unlikely(tx_cr_num < 0)) {
		priv->runtime_stats.tx_stats.poll_jfc_failed++;
		netdev_err(dev, "%s:%d\n", ipourma_err_desc(IPOURMA_POLL_JFC_FAILED),
				tx_cr_num);
		return;
	}
	for (int i = 0; i < tx_cr_num; i++) {
		priv->runtime_stats.tx_stats.cqe_recved++;
		ipourma_handle_tx_wc(dev, priv, &cr[i]);
	}
}

STATIC int ipourma_reset_tx_bufs(struct ipourma_dev_priv *priv, struct ubcore_cr *cr)
{
	if (IS_ERR_OR_NULL(priv->jetty))
		return IPOURMA_OK;
	for (u32 i = 0; i < IPOURMA_MAX_EID_CNT; i++) {
		ipourma_reset_tx_bufs_by_eid(priv, cr, i);
		cleanup_tx_ring_by_eid(priv, i);
		ipourma_uninit_tx_bufs(priv, i);
	}
	atomic_set(&priv->tx_ring_blocked, 0);
	return IPOURMA_OK;
}

STATIC void ipourma_reset_rx_buf_by_eid(struct ipourma_dev_priv *priv,
					struct ubcore_cr *cr, u32 eid_idx)
{
	struct ubcore_jfr_attr attr = {
		.mask = UBCORE_JFR_STATE,
		.state = UBCORE_JFR_STATE_ERROR,
	};
	struct net_device *dev = priv->dev;
	int rx_cr_num = 0;

	if (IS_ERR_OR_NULL(priv->jfr[eid_idx]))
		return;
	if (unlikely(ubcore_modify_jfr(priv->jfr[eid_idx], &attr, NULL) != 0)) {
		netdev_err(dev, "%s\n", ipourma_err_desc(IPOURMA_MODIFY_JFR_FAILED));
		return;
	}
	rx_cr_num = ubcore_poll_jfc(priv->rx_jfc, ipourma_rx_ring_size, cr);
	if (unlikely(rx_cr_num < 0)) {
		priv->runtime_stats.rx_stats.poll_jfc_failed++;
		netdev_dbg(dev, "%s:%d\n", ipourma_err_desc(IPOURMA_POLL_JFC_FAILED),
				rx_cr_num);
		return;
	}

	for (int i = 0; i < rx_cr_num; i++) {
		priv->runtime_stats.rx_stats.cqe_recved++;
		ipourma_handle_rx_wc(dev, priv, &cr[i]);
	}
}

STATIC int ipourma_reset_rx_bufs(struct ipourma_dev_priv *priv, struct ubcore_cr *cr)
{
	/* this function will be filled in the next commit */
	ipourma_reset_rx_buf_by_eid(priv, cr, 0);
	return 0;
}

int ipourma_reset_rings(struct ipourma_dev_priv *priv)
{
	size_t size = sizeof(struct ubcore_cr) * ipourma_rx_ring_size;
	struct ubcore_cr *cr = NULL;

	cr = vzalloc(size);
	if (IS_ERR_OR_NULL(cr)) {
		netdev_err(priv->dev, "%s\n", ipourma_err_desc(IPOURMA_ALLOC_CR_FAILED));
		return IPOURMA_ALLOC_CR_FAILED;
	}

	ipourma_reset_tx_bufs(priv, cr);
	ipourma_reset_rx_bufs(priv, cr);
	for (u32 i = 0; i < IPOURMA_MAX_EID_CNT; i++)
		ipourma_uninit_urma_resources_by_eid(priv, i);
	vfree(cr);

	return IPOURMA_OK;
}

void ipourma_uninit_urma_resources_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	/* this function will be filled in the next commit */
}

int ipourma_init_urma_resources_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	/* this function will be filled in the next commit */
	return 0;
}
