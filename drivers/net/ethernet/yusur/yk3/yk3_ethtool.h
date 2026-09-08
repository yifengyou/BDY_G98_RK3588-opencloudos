/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_ETHTOOL_H
#define _YK3_ETHTOOL_H

#include "yk3_base.h"

enum {
	YK3_RXFH_TCP_V4_FLOW = 0,
	YK3_RXFH_TCP_V6_FLOW = 1,
	YK3_RXFH_UDP_V4_FLOW = 2,
	YK3_RXFH_UDP_V6_FLOW = 3,
	YK3_RXFH_MAX_FLOW_TYPE = 4,
};

enum {
	YK3_RXH_L3_PROTO	= 1 << 0,
	YK3_RXH_IP_SRC		= 1 << 1,
	YK3_RXH_IP_DST		= 1 << 2,
	YK3_RXH_L4_SRC		= 1 << 3,
	YK3_RXH_L4_DST		= 1 << 4,
};

struct yk3_ethtool_ksetting {
	__ETHTOOL_DECLARE_LINK_MODE_MASK(supported);
	__ETHTOOL_DECLARE_LINK_MODE_MASK(advertising);
	__ETHTOOL_DECLARE_LINK_MODE_MASK(lp_advertising);
};

static inline u8 yk3_get_rxfh_conf(struct yk3_ndev_priv *ndev_priv, u32 flow_type)
{
	return (ndev_priv->rxfh_cfg >> (flow_type * 8)) & 0xff;
}

static inline void yk3_set_rxfh_conf(struct yk3_ndev_priv *ndev_priv, u32 flow_type, u8 fields)
{
	ndev_priv->rxfh_cfg = (ndev_priv->rxfh_cfg & ~(0xff << (flow_type * 8))) |
			      ((fields & 0xff) << (flow_type * 8));
}

enum {
	YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY,
	YK3_ET_PFLAG_PVID_MISS_UPLOAD,
	YK3_ET_PFLAG_FC_PAUSE_FILTER,
	YK3_ET_PFLAG_PFC_PAUSE_FILTER,
	YK3_ET_PFLAG_PRIO_VLAN_MODE,
	YK3_ET_PFLAG_LLDP_MODE,
	YK3_ET_FFLAG_LINK_DOWN_ON_CLOSE,
	YK3_ET_FFLAG_SRC_EQ_DST_DROP,
	YK3_ET_PFLAG_MAX,		/* end of private flags */
};

extern const struct ethtool_ops yk3_ethtool_ops;

int yk3_get_fc_xon(struct net_device *ndev, u32 *val);
int yk3_get_fc_xoff(struct net_device *ndev, u32 *val);
int yk3_set_fc_xon(struct net_device *ndev, u32 val);
int yk3_set_fc_xoff(struct net_device *ndev, u32 val);
int yk3_clear_fc_register(struct yk3_ndev_priv *ndev_priv);

#endif /* _YK3_ETHTOOL_H */
