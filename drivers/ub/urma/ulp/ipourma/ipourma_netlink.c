// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma netlink support
 */

#include <net/rtnetlink.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/if_link.h>

#include "ipourma_netlink.h"

void ipourma_set_rtnl_link_ops(struct net_device *dev)
{
	// this function will be filled in the next commit
}
