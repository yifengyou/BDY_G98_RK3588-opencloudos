// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma underlay datapath implementaion
 */

#include <net/dst.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/smp.h>
#include <linux/jiffies.h>
#include "ipourma_res.h"
#include "ipourma_main.h"
#include "ipourma_utils.h"
#include "ipourma_err.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_ub.h"

void ipourma_build_seg_cfg(struct ubcore_seg_cfg *cfg,
			   uint64_t va, uint64_t len)
{
	union ubcore_reg_seg_flag flag = {
		.bs.token_policy = UBCORE_TOKEN_NONE,
		.bs.cacheable = UBCORE_NON_CACHEABLE,
		.bs.access = UBCORE_ACCESS_LOCAL_ONLY,
	};
	cfg->va = va;
	cfg->len = len;
	cfg->flag = flag;
}

STATIC int ipourma_prepare_tx_data(struct ubcore_device *urma_dev,
					struct ipourma_dev_priv *priv,
					struct ipourma_tx_buf *tx_req)
{
	struct sk_buff *skb = tx_req->skb;
	struct ubcore_seg_cfg cfg = {0};
	u32 offset = 1, i, j;
	u32 linear_len = skb_headlen(skb);

	pr_skb_head_plus_linear(skb, "Register tx seg");
	if (unlikely(linear_len > priv->tx_buf_size)) {
		priv->runtime_stats.tx_stats.linear_len_oversize++;
		netdev_dbg(priv->dev, "%s: linear len %u, tx buffer size %u\n",
					ipourma_err_desc(IPOURMA_UNSUPPORTED_LINEAR_DATA_LEN),
					linear_len, priv->tx_buf_size);
		return IPOURMA_UNSUPPORTED_LINEAR_DATA_LEN;
	}
	if (likely(linear_len > 0))
		memcpy(tx_req->buf_aligned, skb->data, linear_len);
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		void *vaddr = skb_frag_address(frag);
		unsigned int frag_len = skb_frag_size(frag);

		ipourma_build_seg_cfg(&cfg, (u64)vaddr, frag_len);
		tx_req->seg[i + offset] = ubcore_register_seg(urma_dev, &cfg, NULL);
		if (IS_ERR_OR_NULL(tx_req->seg[i + offset]))
			goto partial_reg_out;
	}
	return IPOURMA_OK;

partial_reg_out:
	for (j = 1; j < i + offset; j++) {
		if (IS_ERR_OR_NULL(tx_req->seg[j]))
			continue;
		ubcore_unregister_seg(tx_req->seg[j]);
	}
	return IPOURMA_REGISTER_SEG_FAILED;
}

STATIC int ipourma_setup_sges(struct ipourma_dev_priv *priv,
	struct ipourma_tx_buf *tx_req)
{
	int i, offset = 0;
	struct sk_buff *skb = tx_req->skb;
	skb_frag_t *frags = skb_shinfo(skb)->frags;
	int nr_frags = skb_shinfo(skb)->nr_frags;

	if (skb_headlen(skb)) {
		tx_req->tx_sge[0].len = skb_headlen(skb);
		offset++;
	}
	for (i = 0; i < nr_frags; i++) {
		tx_req->tx_sge[i + offset].addr = tx_req->seg[i + offset]->seg.ubva.va;
		tx_req->tx_sge[i + offset].len = skb_frag_size(&frags[i]);
		tx_req->tx_sge[i + offset].tseg = tx_req->seg[i + offset];
	}

	return (nr_frags + offset);
}

STATIC struct ipourma_tjetty_hash_node *ipourma_locate_tjetty_node(
					struct ipourma_tjetty_lru *tjetty_lru,
					union ubcore_eid *src_eid,
					union ubcore_eid *dst_eid,
					bool lock_free)
{
	// this function will be filled in the next commit
	return NULL;
}

STATIC inline void ipourma_delete_tjetty_node(struct ipourma_tjetty_hash_node *tjetty_node)
{
	hlist_del(&tjetty_node->hlist);
	ubcore_unimport_jetty(tjetty_node->tjetty);
	kfree(tjetty_node);
}

STATIC void ipourma_lru_del_tail(struct ipourma_tjetty_lru *tjetty_lru, bool lock_free)
{
	struct ipourma_tjetty_hash_node *tjetty_node = NULL;

	if (!lock_free)
		spin_lock(&tjetty_lru->lock);

	if (likely(!list_empty(&tjetty_lru->list))) {
		tjetty_node = list_last_entry(&tjetty_lru->list,
						struct ipourma_tjetty_hash_node,
						lru_list);
		list_del(&tjetty_node->lru_list);
		ipourma_delete_tjetty_node(tjetty_node);
		tjetty_lru->count--;
	}

	if (!lock_free)
		spin_unlock(&tjetty_lru->lock);
}

void ipourma_lru_clear(struct ipourma_tjetty_lru *tjetty_lru)
{
	spin_lock(&tjetty_lru->lock);
	while (tjetty_lru->count > 0)
		ipourma_lru_del_tail(tjetty_lru, true);
	spin_unlock(&tjetty_lru->lock);
}

STATIC void tjetty_aging_callback(struct work_struct *work)
{
	struct delayed_work *tjetty_aging_work = container_of(work,
							      struct delayed_work,
							      work);
	struct ipourma_tjetty_lru *tjetty_lru = container_of(tjetty_aging_work,
							     struct ipourma_tjetty_lru,
							     tjetty_aging_work);
	struct ipourma_tjetty_hash_node *tjetty_node = NULL;
	u64 now_jiffies = 0;

	if (!spin_trylock(&tjetty_lru->lock)) {
		schedule_delayed_work(tjetty_aging_work,
			msecs_to_jiffies(IPOURMA_TJETTY_CB_S * MSEC_PER_SEC));
		return;
	}
	now_jiffies = get_jiffies_64();
	while (tjetty_lru->count > 0) {
		tjetty_node = list_last_entry(&tjetty_lru->list,
					struct ipourma_tjetty_hash_node,
					lru_list);
		if (now_jiffies - tjetty_node->last_jiffies
			< (u64)tjetty_lru->tjetty_aging_timeout_s * HZ)
			break;
		list_del(&tjetty_node->lru_list);
		ipourma_delete_tjetty_node(tjetty_node);
		tjetty_lru->count--;
	}
	schedule_delayed_work(tjetty_aging_work,
		msecs_to_jiffies((u32)tjetty_lru->tjetty_aging_interval_s * MSEC_PER_SEC));
	spin_unlock(&tjetty_lru->lock);
}

void ipourma_init_tjetty_aging_work(struct ipourma_tjetty_lru *tjetty_lru)
{
	struct delayed_work *work = &tjetty_lru->tjetty_aging_work;
	unsigned long flags;

	spin_lock_irqsave(&tjetty_lru->lock, flags);
	tjetty_lru->tjetty_aging_interval_s = IPOURMA_TJETTY_CB_S;
	tjetty_lru->tjetty_aging_timeout_s = IPOURMA_TJETTY_TIMEOUT_S;
	INIT_DELAYED_WORK(work, tjetty_aging_callback);
	schedule_delayed_work(work,
		msecs_to_jiffies((u32)tjetty_lru->tjetty_aging_interval_s * MSEC_PER_SEC));
	spin_unlock_irqrestore(&tjetty_lru->lock, flags);
}

STATIC struct ubcore_tjetty *ipourma_import_new_tjetty(
	struct ipourma_dev_priv *priv, struct ipourma_tx_buf *tx_req)
{
	// this function will be filled in the next commit
	return NULL;
}

STATIC void ipourma_advance_tx_tail(struct ipourma_dev_priv *priv,
	struct ipourma_tx_buf *tx_req)
{
	u32 i = 0, eid_idx = tx_req->eid_index;
	unsigned long flags;

	spin_lock_irqsave(&priv->tx_ring_locks[eid_idx], flags);
	for (i = priv->tx_tail[eid_idx] % ipourma_tx_ring_size;
		i != (tx_req->idx + 1) % ipourma_tx_ring_size;
		i = (i + 1) % ipourma_tx_ring_size) {
		if (priv->tx_ring[eid_idx][i].tx_buf_in_use == 1)
			break;
		priv->tx_tail[eid_idx]++;
	}
	if (unlikely(priv->tx_ring_is_full[eid_idx] &&
		test_bit(IPOURMA_DEV_ADMIN_UP, &priv->flags)) &&
		priv->tx_head[eid_idx] - priv->tx_tail[eid_idx] <= (ipourma_tx_ring_size >> 1)) {
		priv->tx_ring_is_full[eid_idx] = false;
		atomic_sub(1, &priv->tx_ring_blocked);
		if (atomic_read(&priv->tx_ring_blocked) == 0)
			netif_wake_queue(priv->dev);
	}
	spin_unlock_irqrestore(&priv->tx_ring_locks[eid_idx], flags);
}

STATIC inline void ipourma_handle_tx_failed(struct net_device *dev, struct ipourma_tx_buf *tx_req)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);

	tx_req->tx_buf_in_use = 0;
	ipourma_advance_tx_tail(priv, tx_req);
	dev_kfree_skb_any(tx_req->skb);
}

STATIC int ipourma_update_wr(struct net_device *dev, struct ipourma_tx_buf *tx_req)
{
	struct ipourma_tjetty_hash_node *tjetty_node = NULL;
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct ubcore_tjetty *tjetty_new = NULL;
	union ubcore_eid *src_eid, *dst_eid;

	if (IS_ERR_OR_NULL(tx_req->tjetty)) {
		src_eid = &priv->eid_info[tx_req->eid_index].eid;
		dst_eid = &tx_req->dst_eid;
		tjetty_node = ipourma_locate_tjetty_node(&priv->tjetty_lru,
								src_eid, dst_eid, false);
		if (IS_ERR_OR_NULL(tjetty_node)) {
			priv->runtime_stats.tx_stats.num_import_jetty_real++;
			tjetty_new = ipourma_import_new_tjetty(priv, tx_req);
			if (IS_ERR_OR_NULL(tjetty_new)) {
				ipourma_handle_tx_failed(dev, tx_req);
				priv->runtime_stats.tx_stats.import_jetty_failed++;
				return IPOURMA_TJETTY_NODE_ALLOC_FAILED;
			}
		} else {
			tjetty_new = tjetty_node->tjetty;
			priv->runtime_stats.tx_stats.num_import_jetty_bypass++;
		}
		tx_req->tx_wr.tjetty = tjetty_new;
	} else {
		tx_req->tx_wr.tjetty = tx_req->tjetty;
		priv->runtime_stats.tx_stats.num_tjetty_hash_hit++;
	}

	tx_req->tx_wr.send.src.num_sge = (u32)ipourma_setup_sges(priv, tx_req);
	return 0;
}

void ipourma_post_send(struct work_struct *work)
{
	struct ipourma_tx_buf *tx_req = container_of(work, struct ipourma_tx_buf, work);
	struct ipourma_dev_priv *priv = tx_req->priv;
	int ret;
	struct ubcore_jfs_wr *jfs_bad_wr = NULL;

	priv->runtime_stats.tx_stats.post_send_start++;
	pr_debug("post_send start, idx %u, jetty %u\n", tx_req->idx, tx_req->eid_index);
	ret = ipourma_prepare_tx_data(priv->urma_dev, priv, tx_req);
	if (ret != IPOURMA_OK)
		goto free_skb_out;
	ret = ipourma_update_wr(priv->dev, tx_req);
	if (unlikely(ret != 0))
		goto free_skb_out;
	ret = ubcore_post_jetty_send_wr(priv->jetty[tx_req->eid_index],
								&tx_req->tx_wr, &jfs_bad_wr);
	if (unlikely(ret != 0)) {
		priv->runtime_stats.tx_stats.send_wr_failed++;
		netdev_dbg(priv->dev, "%s: error code = %d\n",
					ipourma_err_desc(IPOURMA_POST_SEND_FAILED), ret);
		goto free_skb_out;
	}
	priv->runtime_stats.tx_stats.pass_to_ub++;
	pr_debug("post_send finish, idx %u, jetty %u\n", tx_req->idx, tx_req->eid_index);
	return;
free_skb_out:
	dev_kfree_skb_any(tx_req->skb);
	tx_req->skb = NULL;
}

int ipourma_register_rx_segments(struct net_device *dev,
	struct ubcore_seg_cfg *cfg, struct ipourma_rx_buf *rx_req)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	u32 blk_idx;
	u32 i;

	blk_idx = rx_req->idx * priv->skb_buf_size / IPOURMA_REGISTER_SEG_SIZE;

	for (i = 0; i < IPOURMA_MAX_RX_SGES; i++) {
		rx_req->seg[i] = priv->ipourma_ub_rx_seg[rx_req->eid_index][blk_idx];
		/* FIME: deal with partial failures */
		if (IS_ERR_OR_NULL(rx_req->seg[i])) {
			priv->runtime_stats.rx_stats.register_seg_failed++;
			netdev_warn(dev, "%s\n",
						ipourma_err_desc(IPOURMA_REGISTER_SEG_FAILED));
			return IPOURMA_REGISTER_SEG_FAILED;
		}
	}

	return IPOURMA_OK;
}

STATIC struct sk_buff *ipourma_alloc_rx_skb(struct net_device *dev)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct sk_buff *skb;

	skb = alloc_skb(priv->skb_buf_size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(skb)) {
		priv->runtime_stats.rx_stats.alloc_skb_failed++;
		netdev_dbg(dev, "%s\n",
					ipourma_err_desc(IPOURMA_ALLOC_RX_SKB_FAILED));
		return NULL;
	}
	skb_push(skb, skb_headroom(skb));
	skb_trim(skb, 0);

	return skb;
}

STATIC int ipourma_alloc_rx_buffer(struct net_device *dev, u32 eid_idx, u32 idx)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct ubcore_seg_cfg cfg = { 0 };
	struct sk_buff *skb_pass_up;
	u32 blk_idx, offset;

	skb_pass_up = ipourma_alloc_rx_skb(dev);
	if (IS_ERR_OR_NULL(skb_pass_up))
		return IPOURMA_ALLOC_RX_SKB_FAILED;

	blk_idx = idx * priv->skb_buf_size / IPOURMA_REGISTER_SEG_SIZE;
	offset = (idx * priv->skb_buf_size) % IPOURMA_REGISTER_SEG_SIZE;
	priv->rx_ring[eid_idx][idx].buf_aligned = priv->rx_buf_aligned[eid_idx][blk_idx] + offset;

	if (ipourma_register_rx_segments(dev, &cfg, &priv->rx_ring[eid_idx][idx])
			!= IPOURMA_OK) {
		dev_kfree_skb_any(skb_pass_up);
		return IPOURMA_REGISTER_SEG_FAILED;
	}

	priv->rx_ring[eid_idx][idx].skb_pass_up = skb_pass_up;
	pr_skb_head_plus_linear(skb_pass_up, "Alloced rx buffer skb_pass_up");

	return IPOURMA_OK;
}

int ipourma_urma_post_recv(struct net_device *dev, u32 eid_idx, u32 idx)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	struct ipourma_rx_buf *rx_buf = &priv->rx_ring[eid_idx][idx];
	int ret;
	struct ubcore_jfr_wr *jfr_bad_wr = NULL;

	rx_buf->rx_sge[0].tseg = rx_buf->seg[0];
	rx_buf->rx_sge[0].addr = (u64)rx_buf->buf_aligned;
	rx_buf->rx_sge[0].len = priv->urma_mtu;
	pr_debug("post_recv start, eid idx %u, idx %u\n", rx_buf->eid_index, rx_buf->idx);
	ret = ubcore_post_jetty_recv_wr(priv->jetty[eid_idx], &rx_buf->rx_wr, &jfr_bad_wr);
	if (unlikely(ret != 0)) {
		priv->runtime_stats.rx_stats.post_wr_failed++;
		netdev_dbg(dev, "%s:%d\n",
					ipourma_err_desc(IPOURMA_URMA_POST_RECV_FAILED), ret);
		dev_kfree_skb_any(rx_buf->skb_pass_up);
		rx_buf->skb_pass_up = NULL;
		return IPOURMA_URMA_POST_RECV_FAILED;
	}
	priv->runtime_stats.rx_stats.num_post_wr++;
	pr_debug("post_recv finish, eid idx %u, idx %u\n", rx_buf->eid_index, rx_buf->idx);
	return IPOURMA_OK;
}

static int ipourma_post_recv_by_eid(struct net_device *dev, u32 eid_idx)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);
	int ret = IPOURMA_OK;
	u32 i;

	for (i = 0; i < ipourma_rx_ring_size; i++) {
		if (ipourma_alloc_rx_buffer(dev, eid_idx, i) != IPOURMA_OK) {
			ret = IPOURMA_ALLOC_RX_SKB_FAILED;
			goto post_recv_failed;
		}

		if (ipourma_urma_post_recv(dev, eid_idx, i) != IPOURMA_OK) {
			ret = IPOURMA_URMA_POST_RECV_FAILED;
			goto post_recv_failed;
		}
	}
	return ret;

post_recv_failed:
	ipourma_uninit_rx_bufs(priv, eid_idx);
	return ret;
}

void ipourma_replenish_segments(struct work_struct *work)
{
	struct ipourma_rx_buf *rx_buf = container_of(work, struct ipourma_rx_buf, work);
	struct ipourma_dev_priv *priv = rx_buf->priv;

	priv->runtime_stats.rx_stats.replenish_deque++;

	/* if post failed, skb will be released as well */
	rx_buf->skb_pass_up = ipourma_alloc_rx_skb(priv->dev);
	if (IS_ERR_OR_NULL(rx_buf->skb_pass_up)) {
		netdev_dbg(priv->dev, "%s: idx = %u\n",
			ipourma_err_desc(IPOURMA_REPLENISH_RX_SEG_FAILED), rx_buf->idx);
		return;
	}
	if (test_bit(IPOURMA_DEV_ADMIN_UP, &priv->flags))
		ipourma_urma_post_recv(priv->dev, rx_buf->eid_index, rx_buf->idx);
}

STATIC inline void ipourma_napi_del(struct net_device *dev)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);

	netif_napi_del(&priv->napi_send);
	netif_napi_del(&priv->napi_recv);
}

int ipourma_urma_init_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	int ret;

	ret = ipourma_init_rings_by_eid(priv, eid_idx);
	if (ret != IPOURMA_OK)
		goto init_rings_by_eid_failed;

	ret = ipourma_init_urma_resources_by_eid(priv, eid_idx);
	if (ret != IPOURMA_OK)
		goto init_urma_res_by_eid_failed;

	ret = ipourma_post_recv_by_eid(priv->dev, eid_idx);
	if (ret != IPOURMA_OK)
		goto post_recv_by_eid_failed;

	return ret;
post_recv_by_eid_failed:
	ipourma_uninit_urma_resources_by_eid(priv, eid_idx);
init_urma_res_by_eid_failed:
	ipourma_uninit_rings_by_eid(priv, eid_idx);
init_rings_by_eid_failed:
	return ret;
}

void ipourma_urma_dev_uninit(struct net_device *dev)
{
	ipourma_uninit_tjetty_hmap(dev);
	ipourma_uninit_urma_resources(dev);
	ipourma_uninit_rings(dev);
	ipourma_napi_del(dev);
}

STATIC void ipourma_do_handle_tx_wc(struct net_device *dev,
	struct ipourma_dev_priv *priv, u32 eid_idx, u32 idx, struct ubcore_cr *cr)
{
	struct ipourma_tx_buf *tx_req = &priv->tx_ring[eid_idx][idx];

	if (unlikely(tx_req->tx_buf_in_use == 0))
		return;
	tx_req->tx_buf_in_use = 0;
	ipourma_advance_tx_tail(priv, tx_req);
	if (unlikely(cr->status != UBCORE_CR_SUCCESS)) {
		netdev_dbg(dev, "%s: status = %d\n",
					ipourma_err_desc(IPOURMA_TX_CQE_ERR), cr->status);
		goto free_tx_skb;
	}
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += tx_req->skb->len;
	netif_trans_update(dev);
free_tx_skb:
	dev_kfree_skb_any(tx_req->skb);
	tx_req->skb = NULL;
}

void ipourma_handle_tx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr)
{
	u32 eid_idx, idx;

	if (cr->status >= IPOURMA_MAX_CR_STATUS) {
		priv->runtime_stats.tx_stats.cqe_err++;
		return;
	}

	priv->runtime_stats.tx_stats.cqe_stats[cr->status]++;
	if (likely(cr->status == UBCORE_CR_SUCCESS))
		priv->runtime_stats.tx_stats.cqe_success++;
	else
		priv->runtime_stats.tx_stats.cqe_err++;

	if (unlikely(cr->status == UBCORE_CR_WR_FLUSH_ERR_DONE)) {
		/* should not happen, user_ctx is invalid in this case */
		netdev_dbg(dev, "%s:%d\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_CR_STATUS), cr->status);
		return;
	}
	if (unlikely(cr->status == UBCORE_CR_FLUSH_ERR))
		priv->runtime_stats.tx_stats.flush_jetty_success++;

	if (unlikely(cr->local_id < IPOURMA_WELL_KNOWN_JETTY_ID ||
		cr->local_id >= IPOURMA_MAX_EID_CNT + IPOURMA_WELL_KNOWN_JETTY_ID)) {
		netdev_dbg(dev, "%s:%u\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_WQE_JETTY_IDX), eid_idx);
		return;
	}
	eid_idx = cr->local_id - IPOURMA_WELL_KNOWN_JETTY_ID;

	idx = cr->user_ctx;
	if (unlikely(idx >= ipourma_tx_ring_size)) {
		netdev_dbg(dev, "%s:%u\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_WQE_IDX), idx);
		return;
	}
	pr_debug("now cr status:%d\n", cr->status);
	ipourma_do_handle_tx_wc(dev, priv, eid_idx, idx, cr);
}

STATIC void ipourma_do_handle_rx_wc(struct net_device *dev,
					struct ipourma_dev_priv *priv,
					u32 eid_idx, u32 idx,
					struct ubcore_cr *cr)
{
	struct ipourma_rx_buf *rx_req = &priv->rx_ring[eid_idx][idx];
	struct sk_buff *skb;
	u32 data_len;

	/* hold the skb & segments */
	skb = rx_req->skb_pass_up;
	rx_req->skb_pass_up = NULL;
	if (cr->completion_len > priv->tx_buf_size) {
		dev_kfree_skb_any(skb);
		priv->runtime_stats.rx_stats.cr_len_err++;
		goto rx_wc_out;
	}
	data_len = cr->completion_len - (u32)sizeof(struct ipourma_header);

	skb->protocol = ((struct ipourma_header *)rx_req->buf_aligned)->proto;
	memcpy(skb->head, rx_req->buf_aligned + sizeof(struct ipourma_header), data_len);
	/* update the skb pointer */
	skb->data_len = 0;
	skb_trim(skb, 0);
	skb_put(skb, data_len);

	skb->pkt_type = PACKET_HOST;
	skb->dev = dev;
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;
	pr_debug("IPoURMA received packet's proto: 0x%x\n",
			 ntohs(skb->protocol));
	pr_skb_head_plus_linear(skb, "Skb passed to kernel stack");
	netif_rx(skb);
	priv->runtime_stats.rx_stats.pass_to_kernel++;

rx_wc_out:
	priv->runtime_stats.rx_stats.replenish_enque++;
	queue_work(priv->rx_wq, &(rx_req->work));
}

void ipourma_handle_rx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr)
{
	u32 eid_idx, idx;

	if (cr->status >= IPOURMA_MAX_CR_STATUS) {
		priv->runtime_stats.rx_stats.cqe_err++;
		return;
	}
	priv->runtime_stats.rx_stats.cqe_stats[cr->status]++;
	if (unlikely(cr->status != UBCORE_CR_SUCCESS)) {
		priv->runtime_stats.rx_stats.cqe_err++;
		netdev_dbg(dev, "%s:%d\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_CR_STATUS), cr->status);
		return;
	}
	priv->runtime_stats.rx_stats.cqe_success++;
	if (unlikely(cr->local_id < IPOURMA_WELL_KNOWN_JETTY_ID ||
		cr->local_id >= IPOURMA_MAX_EID_CNT + IPOURMA_WELL_KNOWN_JETTY_ID)) {
		netdev_dbg(dev, "%s:%u\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_WQE_JETTY_IDX), eid_idx);
		return;
	}
	eid_idx = cr->local_id - IPOURMA_WELL_KNOWN_JETTY_ID;
	idx = cr->user_ctx;
	if (unlikely(idx >= ipourma_rx_ring_size)) {
		netdev_dbg(dev, "%s:%u\n",
				   ipourma_err_desc(IPOURMA_INCORRECT_WQE_IDX), idx);
		return;
	}
	ipourma_do_handle_rx_wc(dev, priv, eid_idx, idx, cr);
}
