// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Description: ipourma ip configuration support
 */

#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/inet.h>
#include "ipourma_addr_res.h"
#include "ipourma_ip.h"

#define IPV6_PREFIX_LEN 64

/* send ipv6 address by netlink */
int ipourma_send_ipv6_netlink(struct net_device *dev, union ubcore_eid *eid, int msg_type)
{
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	struct ifaddrmsg *ifa;
	struct in6_addr addr;
	int ret;

	ret = ipourma_resolve_ipaddr(dev, ETH_P_IPV6, eid, &addr);
	if (ret != 0)
		return ret;

	size_t msg_size = NLMSG_SPACE(sizeof(struct ifaddrmsg))
					+ (uint32_t)nla_total_size(sizeof(struct in6_addr));
	skb = nlmsg_new(msg_size, GFP_KERNEL);
	if (IS_ERR_OR_NULL(skb)) {
		pr_err("send_ipv6_netlink:alloc skb failed\n");
		return -ENOMEM;
	}

	nlh = nlmsg_put(skb, 0, 0, msg_type, sizeof(struct ifaddrmsg),
		NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL);
	if (IS_ERR_OR_NULL(nlh)) {
		kfree_skb(skb);
		pr_err("send_ipv6_netlink:construct netlink msg failed\n");
		return -IPOURMA_NLMSG_ERR;
	}

	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;
	NETLINK_CB(skb).flags = NETLINK_SKB_DST;

	ifa = nlmsg_data(nlh);
	ifa->ifa_family = AF_INET6;
	ifa->ifa_prefixlen = IPV6_PREFIX_LEN;
	ifa->ifa_index = (uint32_t)dev->ifindex;
	ifa->ifa_scope = RT_SCOPE_UNIVERSE;
	ifa->ifa_flags = IFA_F_PERMANENT;

	ret = nla_put(skb, IFA_ADDRESS, sizeof(struct in6_addr), &addr);
	if (ret != 0) {
		kfree_skb(skb);
		pr_err("send_ipv6_netlink:construct netlink data failed: %d\n", ret);
		return -IPOURMA_NLDATA_ERR;
	}
	nlmsg_end(skb, nlh);

	/*
	 * send a netlink message to the kernel to simulate the ${ip addr add} command
	 * in the user space and trigger the
	 * kernel to invoke the private function ipv6_add_addr.
	 */
	ret = rtnl_unicast(skb, dev_net(dev), 0);
	if (ret != 0) {
		kfree_skb(skb);
		pr_err("send_ipv6_netlink:send netlink msg failed: %d\n", ret);
		return -IPOURMA_NL_SEND_ERR;
	}
	pr_debug("send_ipv6_netlink success\n");
	return 0;
}
