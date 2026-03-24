// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma ethtool support
 */

#include <linux/kernel.h>
#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <linux/version.h>

#include "ub/urma/ubcore_uapi.h"
#include "ipourma_ethtool.h"

#define IPOURMA_DRIVER_VERSION	"0.1.0"
#define IPOURMA_FW_VERSION	"0.1.0"

void ipourma_set_ethtool_ops(struct net_device *dev)
{
	// this function will be filled in the next commit
}
