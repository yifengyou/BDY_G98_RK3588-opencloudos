/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_QOS_H
#define _YK3_QOS_H

#include "yk3_base.h"

#define YK3_MAX_RATE		25000  //25G in Mbps

struct yk3_ets_cfg {
	u8 willing;
	u8 cbs;
	u8 maxtcs;
	u8 prio_table[YK3_MAX_TRAFFIC_CLASS];
	u8 tcbwtable[YK3_MAX_TRAFFIC_CLASS];
	u8 tsatable[YK3_MAX_TRAFFIC_CLASS];
	u64 max_rate[YK3_MAX_TRAFFIC_CLASS];	/* tc_maxrate in Mbps */
};

u8 yk3_qos_get_num_tc(struct yk3_ets_cfg *etscfg);
int yk3_qos_set_qp_tc(struct yk3_pdev_priv *pdev_priv, int qp, int tc);
int yk3_qos_set_qp_rate(struct yk3_pdev_priv *pdev_priv, int qp, u32 maxrate);
int yk3_qos_set_rt_dst(struct yk3_pdev_priv *pdev_priv,
		       u8 qrange_id, u32 qrange, u8 rt_dst_id, u32 rt_dst);
void yk3_qos_init_rt_dst(struct yk3_qos *qos);

int yk3_qos_set_tx_rate(struct yk3_ndev_priv *ndev_priv, u32 rate);

int yk3_qos_set_vf_rate(struct net_device *dev, int vf, int minrate, int maxrate);
int yk3_qos_get_vf_rate(struct yk3_ndev_priv *ndev_priv);
int yk3_qos_set_queue_rate(struct net_device *dev, int index, u32 maxrate);
int yk3_qos_set_link_speed(struct yk3_pdev_priv *pdev_priv, u32 speed_mbps);

int yk3_ets_set_tc_default(struct net_device *ndev, struct yk3_ets_cfg *cfg);
int yk3_dcbnl_getets(struct net_device *netdev, struct ieee_ets *ets);
int yk3_dcbnl_setets(struct net_device *netdev, struct ieee_ets *ets);
int yk3_dcbnl_getmaxrate(struct net_device *netdev, struct ieee_maxrate *maxrate);
int yk3_dcbnl_setmaxrate(struct net_device *netdev, struct ieee_maxrate *maxrate);
int yk3_qos_setup_tc_mqprio_qdisc(struct net_device *ndev, void *type_data);

int yk3_qos_ndev_init(struct net_device *ndev);
void yk3_qos_ndev_exit(struct net_device *ndev);

int yk3_qos_init(struct yk3_pdev_priv *pdev_priv);
void yk3_qos_exit(struct yk3_pdev_priv *pdev_priv);

#endif
