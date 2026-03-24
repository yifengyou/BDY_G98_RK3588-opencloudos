/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2025. All rights reserved.
 *
 * Description: Functions definition of ipourma_ub
 */

#ifndef _IPOURMA_UB_H
#define _IPOURMA_UB_H

#include "ipourma_types.h"

void ipourma_build_seg_cfg(struct ubcore_seg_cfg *cfg,
			   uint64_t va, uint64_t len);
void ipourma_init_tjetty_aging_work(struct ipourma_tjetty_lru *tjetty_lru);
int ipourma_urma_init_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx);
void ipourma_urma_dev_uninit(struct net_device *dev);
void ipourma_post_send(struct work_struct *work);
int ipourma_urma_post_recv(struct net_device *dev, u32 eid_idx, u32 idx);
void ipourma_replenish_segments(struct work_struct *work);
void ipourma_handle_tx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr);
void ipourma_handle_rx_wc(struct net_device *dev,
			  struct ipourma_dev_priv *priv,
			  struct ubcore_cr *cr);

#endif
