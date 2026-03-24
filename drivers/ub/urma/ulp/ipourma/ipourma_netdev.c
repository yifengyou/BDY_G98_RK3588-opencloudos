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
#include "ipourma_ip.h"
#include "ipourma_addr_res.h"
#include "ub/urma/ubcore_uapi.h"
#include "ipourma_netdev.h"

#define IN6_ADDR_BITS   (sizeof(struct in6_addr) * 8)

STATIC u8 g_route_tbl_idx = 100;

STATIC int prepare_route_rule(struct ipourma_dev_priv *priv, struct sk_buff *skb, int type,
							  struct in6_addr *src_ipv6)
{
	struct fib_rule_hdr *frh;
	struct nlmsghdr *nlh;

	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;
	NETLINK_CB(skb).flags = NETLINK_SKB_DST;

	nlh = nlmsg_put(skb, 0, 0, type,
		sizeof(struct fib_rule_hdr), NLM_F_REQUEST | NLM_F_CREATE);
	if (IS_ERR_OR_NULL(nlh))
		return -EMSGSIZE;

	frh = nlmsg_data(nlh);
	memset(frh, 0, sizeof(*frh));
	frh->family = AF_INET6;
	frh->action = FR_ACT_TO_TBL;
	frh->src_len = (u8)IN6_ADDR_BITS;
	if (priv->route_tbl_idx)
		frh->table = priv->route_tbl_idx;
	else {
		frh->table = g_route_tbl_idx;
		priv->route_tbl_idx = g_route_tbl_idx++;
	}

	if (nla_put(skb, FRA_SRC, sizeof(struct in6_addr), src_ipv6))
		return -EMSGSIZE;

	nlmsg_end(skb, nlh);

	return 0;
}

STATIC int ipourma_add_route_rule(struct ipourma_dev_priv *priv, union ubcore_eid *eid)
{
	struct in6_addr addr;
	struct sk_buff *skb;
	int ret;

	if (IS_ERR_OR_NULL(priv) || IS_ERR_OR_NULL(eid))
		return -EINVAL;

	ret = ipourma_resolve_ipaddr(priv->dev, ETH_P_IPV6, eid, &addr);
	if (ret != 0) {
		netdev_err(priv->dev, "Faile to resolve eid "EID_FMT"\n", EID_ARGS(*eid));
		return ret;
	}

	skb = nlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(skb)) {
		netdev_err(priv->dev, "Failed to alloc skb\n");
		return -ENOMEM;
	}

	if (prepare_route_rule(priv, skb, RTM_NEWRULE, &addr)) {
		netdev_err(priv->dev, "Failed to prepare rule\n");
		kfree_skb(skb);
		return -EMSGSIZE;
	}

	ret = rtnl_unicast(skb, dev_net(priv->dev), 0);
	if (ret)
		netdev_err(priv->dev, "Failed to new route\n");

	return ret;
}

STATIC int prepare_route_entry(struct ipourma_dev_priv *priv, struct sk_buff *skb, int type)
{
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;

	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;
	NETLINK_CB(skb).flags = NETLINK_SKB_DST;

	nlh = nlmsg_put(skb, 0, 0, type,
		sizeof(struct fib_rule_hdr), NLM_F_REQUEST | NLM_F_CREATE);
	if (IS_ERR_OR_NULL(nlh))
		return -EMSGSIZE;

	rtm = nlmsg_data(nlh);
	rtm->rtm_family = AF_INET6;
	rtm->rtm_dst_len = 0;
	rtm->rtm_src_len = 0;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_scope = RT_SCOPE_UNIVERSE;
	rtm->rtm_type = RTN_UNICAST;
	rtm->rtm_table = priv->route_tbl_idx;

	if (nla_put_u32(skb, RTA_OIF, priv->dev->ifindex))
		return -EMSGSIZE;

	nlmsg_end(skb, nlh);

	return 0;
}

STATIC void ipourma_add_route_entry(struct work_struct *work)
{
	struct ipourma_dev_priv *priv;
	struct sk_buff *skb;
	int ret;

	priv = container_of(work, struct ipourma_dev_priv, set_route_entry);

	skb = nlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (IS_ERR_OR_NULL(skb)) {
		netdev_err(priv->dev, "Failed to alloc skb\n");
		return;
	}

	if (prepare_route_entry(priv, skb, RTM_NEWROUTE)) {
		netdev_err(priv->dev, "Failed to prepare route entry\n");
		nlmsg_free(skb);
		return;
	}

	ret = rtnl_unicast(skb, dev_net(priv->dev), 0);
	if (ret)
		netdev_err(priv->dev, "Failed to new route\n");
}

STATIC void ipourma_netdev_setup(struct net_device *dev)
{
	// dev->netdev_ops        = &ipourma_netdev_ops;

	/* ipourma_set_ethtool_ops(dev);
	 * ipourma_set_rtnl_link_ops(dev);
	 */
	dev->type              = ARPHRD_ETHER;
	dev->hard_header_len   = IPOURMA_HARD_LEN;
	dev->addr_len          = IPOURMA_ALEN;
	dev->tx_queue_len      = DEFAULT_TX_QUEUE_LEN;
	dev->needs_free_netdev = false;
	dev->mtu               = IPOURMA_DEFAULT_MTU;
	dev->flags            |= IFF_NOARP;
	dev->features         |= NETIF_F_HIGHDMA |
								NETIF_F_GRO;
	dev->features         &= ~(NETIF_F_SG | NETIF_F_TSO | NETIF_F_GSO);
	dev->num_rx_queues     = 1;
	dev->num_tx_queues     = 1;
	dev->watchdog_timeo    = HZ;
	dev->priv_flags |= IFF_LIVE_ADDR_CHANGE | IFF_NO_ADDRCONF;

	netif_keep_dst(dev);
}

STATIC int ipourma_priv_base_init(struct net_device *dev,
	struct ubcore_device *urma_dev)
{
	return IPOURMA_OK;
}

struct net_device *ipourma_alloc_netdev(struct ubcore_device *urma_dev)
{
	struct ipourma_dev_priv *priv;
	struct net_device *dev;

	pr_debug("alloc_netdev\n");
	dev = alloc_netdev(sizeof(struct ipourma_dev_priv), "ipourma%d",
					   NET_NAME_PREDICTABLE, ipourma_netdev_setup);
	if (IS_ERR_OR_NULL(dev)) {
		pr_err("Failed to allocate net_device\n");
		return NULL;
	}
	priv = netdev_priv(dev);
	if (unlikely(ipourma_priv_base_init(dev, urma_dev) != IPOURMA_OK)) {
		free_netdev(dev);
		return NULL;
	}
	if (unlikely(ipourma_init_tjetty_hmap(dev) != IPOURMA_OK)) {
		/* free netdev which has been alloced before eid_init */
		destroy_workqueue(priv->register_wq);
		free_netdev(dev);
		return NULL;
	}
	pr_debug("alloc_netdev completed.\n");

	return dev;
}

STATIC inline void ipourma_priv_eid_uninit(struct ipourma_dev_priv *priv)
{
	kfree(priv->eid_info);
	priv->eid_info = NULL;
	kfree(priv->tx_ring_is_full);
	priv->tx_ring_is_full = NULL;
	priv->eid_count = 0;
}

int ipourma_unalloc_netdev(struct net_device *dev)
{
	struct ipourma_dev_priv *priv = netdev_priv(dev);

	if (priv->eid_info)
		ipourma_priv_eid_uninit(priv);
	free_netdev(dev);
	return 0;
}

void ipourma_create_new_eid(struct ipourma_dev_priv *priv, u32 eid_idx)
{
	int ret;

	ret = ipourma_urma_init_by_eid(priv, eid_idx);
	if (ret != IPOURMA_OK) {
		memset(&priv->eid_info[eid_idx].eid, 0, UBCORE_EID_SIZE);
		if (netif_running(priv->dev))
			atomic_sub(1, &priv->need_set_ip_route);
		return;
	}

	if (netif_running(priv->dev) && atomic_read(&priv->need_set_ip_route) > 0) {
		ipourma_send_ipv6_netlink(priv->dev, &(priv->eid_info[eid_idx].eid), RTM_NEWADDR);
		ipourma_add_route_rule(priv, &(priv->eid_info[eid_idx].eid));
		ipourma_add_route_entry(&(priv->set_route_entry));
		atomic_sub(1, &priv->need_set_ip_route);
	}
}
