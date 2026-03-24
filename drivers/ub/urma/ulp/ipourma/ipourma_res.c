// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma urma resource management
 */
#include <linux/timer.h>
#include "ipourma_ub.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_res.h"

u32 ipourma_tx_ring_size __read_mostly = IPOURMA_TX_RING_SIZE;
u32 ipourma_rx_ring_size __read_mostly = IPOURMA_RX_RING_SIZE;

int ipourma_reset_rings(struct ipourma_dev_priv *priv)
{
	return IPOURMA_OK;
}

