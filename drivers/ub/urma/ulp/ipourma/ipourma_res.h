/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2025. All rights reserved.
 *
 * Description: Functions definition of ipourma_res
 */

#ifndef _IPOURMA_RES_H
#define _IPOURMA_RES_H

#include "ipourma_types.h"

void ipourma_uninit_rings_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx);
void ipourma_uninit_rx_bufs(struct ipourma_dev_priv *priv, u32 eid_idx);
int ipourma_init_tjetty_hmap(struct net_device *dev);
int ipourma_reset_rings(struct ipourma_dev_priv *priv);
int ipourma_init_rings_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx);
void ipourma_uninit_urma_resources_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx);
int ipourma_init_urma_resources_by_eid(struct ipourma_dev_priv *priv, u32 eid_idx);

#endif
