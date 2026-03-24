// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma netdev operations
 */

#include <net/addrconf.h>
#include <net/pkt_sched.h>
#include <net/ip_fib.h>
#include <net/route.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/ipv6.h>
#include <linux/in6.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/interrupt.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/ethtool.h>

#include "ipourma_res.h"
#include "ipourma_ub.h"
#include "ipourma_main.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_netdev.h"

#define IN6_ADDR_BITS   (sizeof(struct in6_addr) * 8)

struct net_device *ipourma_alloc_netdev(struct ubcore_device *urma_dev)
{
	return NULL;
}

int ipourma_unalloc_netdev(struct net_device *dev)
{
	return 0;
}

void ipourma_create_new_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	/* this function will be filled in the next commit */
}
