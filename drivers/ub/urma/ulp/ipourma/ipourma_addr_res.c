// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma address resolution
 */
#include <linux/netdevice.h>
#include <linux/ipv6.h>
#include <linux/in6.h>
#include "ipourma_err.h"
#include "ipourma_addr_res.h"

STATIC int ipourma_resolve_ipv6_from_eids(struct net_device *dev,
	union ubcore_eid *eid, struct in6_addr *addr)
{
	memcpy(addr, eid, UBCORE_EID_SIZE);
	return IPOURMA_OK;
}

int ipourma_resolve_ipaddr(struct net_device *dev,
	u16 proto, union ubcore_eid *eid, struct in6_addr *addr)
{
	switch (proto) {
	case ETH_P_IPV6:
		return ipourma_resolve_ipv6_from_eids(dev, eid, addr);
	default:
		netdev_dbg(dev, "%s\n", ipourma_err_desc(IPOURMA_UNSUPPORTED_ETH_PROTO));
		return IPOURMA_UNSUPPORTED_ETH_PROTO;
	}
	return IPOURMA_OK;
}
