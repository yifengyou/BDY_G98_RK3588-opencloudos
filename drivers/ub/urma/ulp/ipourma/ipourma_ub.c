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

void ipourma_init_tjetty_aging_work(struct ipourma_tjetty_lru *tjetty_lru)
{
	/* this function will be filled in the next commit */
}

void ipourma_post_send(struct work_struct *work)
{
	/* this function will be filled in the next commit */
}

void ipourma_replenish_segments(struct work_struct *work)
{
	/* this function will be filled in the next commit */
}

STATIC int ipourma_alloc_rx_buffer(struct net_device *dev, u32 eid_idx, u32 idx)
{
	/* this function will be filled in the next commit */
	return IPOURMA_OK;
}

int ipourma_urma_post_recv(struct net_device *dev, u32 eid_idx, u32 idx)
{
	/* this function will be filled in the next commit */
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
	/* this function will be filled in the next commit */
}

void ipourma_handle_tx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr)
{
	/* this function will be filled in the next commit */
}

void ipourma_handle_rx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr)
{
	/* this function will be filled in the next commit */
}
