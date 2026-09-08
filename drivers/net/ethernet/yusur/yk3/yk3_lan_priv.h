/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_LAN_PRIV_H
#define _YK3_LAN_PRIV_H

#include <linux/tracepoint.h>
#include "yk3_ethtool.h"
#include "yk3_esw.h"

struct yk3_lan_mbox_ndev_msg {
	u8 enable;
};

enum {
	YK3_LAN_MBOX_MAC_UPD = 0,
	YK3_LAN_MBOX_UC_MAC_ADD = 1,
	YK3_LAN_MBOX_UC_MAC_DEL = 2,
	YK3_LAN_MBOX_MC_MAC_ADD = 3,
	YK3_LAN_MBOX_MC_MAC_DEL = 4,
	YK3_LAN_MBOX_VF_MAC_UPD = 5,
};

struct yk3_lan_mbox_mac_msg {
	u8 mac[ETH_ALEN];
	u8 old_mac[ETH_ALEN];
	u16 qset;
	u16 mac_op;
};

enum {
	YK3_LAN_MBOX_VLAN_ADD = 0,
	YK3_LAN_MBOX_VLAN_DEL = 1,
	YK3_LAN_MBOX_VF_VLAN = 2,
};

struct yk3_lan_mbox_vlan_msg {
	u16 vlan_id;
	__be16 proto;
	u8 qos;
	u16 qset;
	u16 vlan_op;
};

struct yk3_lan_mbox_mtu_msg {
	u16 qset;
	u16 mtu;
};

struct yk3_lan_mbox_ndev_feature_msg {
	u64 changed;
	u64 features;
	u16 qset;
	u8 rss_fields[YK3_RXFH_MAX_FLOW_TYPE];
};

struct yk3_lan_mbox_rss_msg {
	u16 qset;
	u8 fields;
	u32 flow_type;
};

struct yk3_lan_mbox_ndev_flags_msg {
	u32 changed;
	u32 flags;
	u16 qset;
};

struct yk3_lan_mbox_ethtool_priv_flags_msg {
	u32 flags;
	u16 enable;
	u16 qset;
};

#ifdef ALL_MC_FEATURE
struct yk3_lan_mbox_set_all_mc_msg {
	u16 qset;
	u8 enable;
	u32 flags;
};
#endif
struct yk3_lan_mbox_umd_pvid_msg {
	u16 pvid;
	__be16 proto;
	u8 qos;
	u8 tx_only;
	u16 qset;
};

struct yk3_lan_mbox_umd_spoofchk_msg {
	u16 qset;
	bool enable;
};

struct yk3_lan_mbox_umd_reset_vf_conf_msg {
	u16 vf_id;
};

enum {
	YK3_LAN_MBOX_RET_OK		= 0,
	YK3_LAN_MBOX_MBOX_ERR		= 1,
	YK3_LAN_MBOX_INVALID_VF		= 2,
	YK3_LAN_MBOX_RET_DUP_MAC	= 3,
	YK3_LAN_MBOX_RET_NOPERM		= 4,
	YK3_LAN_MBOX_RET_ERR		= 0xFF,
};

struct yk3_lan_mbox_ack_msg {
	u32 ret_code;
	union {
		u8 mac[ETH_ALEN];
	};
};

struct yk3_lan_debug_counter {
	u32 cnt_intf_mac_upd;
	u32 cnt_intf_mac_upd_err;
	u32 cnt_uc_mac_add;
	u32 cnt_uc_mac_add_err;
	u32 cnt_uc_mac_del;
	u32 cnt_uc_mac_del_err;
	u32 cnt_mc_mac_add;
	u32 cnt_mc_mac_add_err;
	u32 cnt_mc_mac_del;
	u32 cnt_mc_mac_del_err;
	u32 cnt_vlan_add;
	u32 cnt_vlan_add_err;
	u32 cnt_vlan_del;
	u32 cnt_vlan_del_err;
#ifdef ALL_MC_FEATURE
	u32 cnt_all_mc_set;
	u32 cnt_all_mc_set_err;
	u32 cnt_all_mc_unset;
	u32 cnt_all_mc_unset_err;
#endif
	/* pf only */
	u32 cnt_vf_ndev_init;
	u32 cnt_vf_ndev_init_err;
	u32 cnt_vf_ndev_init_mac;
	u32 cnt_vf_ndev_init_mac_err;
	u32 cnt_vf_mac;
	u32 cnt_vf_mac_err;
	u32 cnt_vf_set_mac;
	u32 cnt_vf_set_mac_err;
	u32 cnt_vf_vlan;
	u32 cnt_vf_vlan_err;
	u32 cnt_vf_spoofchk;
	u32 cnt_vf_spoofchk_err;
	u32 cnt_vf_trust;
	u32 cnt_vf_trust_err;
	u32 cnt_vf_ndev_feature;
	u32 cnt_vf_ndev_feature_err;
	u32 cnt_vf_ndev_rss;
	u32 cnt_vf_ndev_rss_err;
	u32 cnt_vf_ndev_flags;
	u32 cnt_vf_ndev_flags_err;
	u32 cnt_vf_priv_flags;
	u32 cnt_vf_priv_flags_err;
	u32 cnt_vf_mtu;
	u32 cnt_vf_mtu_err;
	u32 cnt_set_evb_mode;
	u32 cnt_set_evb_mode_err;
	/* vf only */
	u32 cnt_pf_set_vf_mac;
	u32 cnt_pf_set_vf_mac_err;
	/* ndev common */
	u32 cnt_ndev_set_flags;
	u32 cnt_ndev_set_flags_err;
	u32 cnt_ndev_set_priv_flags;
	u32 cnt_ndev_set_priv_flags_err;
	u32 cnt_ndev_set_rss;
	u32 cnt_ndev_set_rss_err;
	u32 cnt_ndev_set_feature;
	u32 cnt_ndev_set_feature_err;
	u32 cnt_ndev_set_mtu;
	u32 cnt_ndev_set_mtu_err;
};

#define YK3_LAN_DBG_CNT_INC(counter) do { \
	struct yk3_lan_debug_counter *dbg_cnt; \
	if (pdev_priv->lan) { \
		dbg_cnt = &pdev_priv->lan->dbg_cnt; \
		dbg_cnt->cnt_##counter++; \
	} \
} while (0)

struct yk3_lan {
	void __iomem *hw_addr;
	struct yk3_esw_manager *esw_mgr;
	/* pf pdev netdev private */
	struct yk3_ndev_priv *ndev_priv;
	struct yk3_lan_debug_counter dbg_cnt;
	struct dentry *dbgfs_lan_counter;
};
#endif /* _YK3_LAN_PRIV_H */
