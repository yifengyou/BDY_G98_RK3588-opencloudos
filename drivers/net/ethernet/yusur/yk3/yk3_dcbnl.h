/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __YK3_DCBNL_H__
#define __YK3_DCBNL_H__

#include "yk3_base.h"

struct yk3_fc_dbg {
	int			offset;
	struct net_device	*ndev;
	struct dentry		*dentry;
};

enum yk3_dbg_fc_types {
	YK3_DBG_FC_XON,
	YK3_DBG_FC_XOFF,
	YK3_DBG_PFC_MAC_L1XON,
	YK3_DBG_PFC_MAC_L1XOFF,
	YK3_DBG_PFC_MAC_L2XON,
	YK3_DBG_PFC_MAC_L2XOFF,
	YK3_DBG_PFC_PCIE_L1XON,
	YK3_DBG_PFC_PCIE_L1XOFF,
	YK3_DBG_PFC_PCIE_L2XON,
	YK3_DBG_PFC_PCIE_L2XOFF,

	YK3_DBG_FC_MAX,
};

struct yk3_fcdbg_params {
	struct dentry		*root;
	struct yk3_fc_dbg	params[YK3_DBG_FC_MAX];
};

enum yk3_prio_state {
	YK3_PRIO_VLAN = 0,
	YK3_PRIO_DSCP = 1,
};

struct yk3_umac_mbox_priv_flags_msg {
	u8 mac_ch;
	u8 enable;
};

#ifdef CONFIG_DCB

enum yk3_dcbx_mode {
	YK3_DCBX_HOST  = 0x0,
	YK3_DCBX_FW    = 0x1,
};

enum yk3_pf_flags {
	YK3_FLAG_DCB_CAPABLE,
	YK3_FLAG_DCB_ENA,
	YK3_FLAG_LLDP_FW,
	YK3_FLAGS_NBITS,
};

struct yk3_dcb_pfc_cfg {
	u8 willing;
	u8 pfccap;
	u8 pfcena;
};

#define YK3_MAX_DSCP (64)

struct yk3_dcbx {
	u8 dcbx_cap;
	enum yk3_dcbx_mode mode;
	enum yk3_prio_state prio;
	DECLARE_BITMAP(flags, YK3_FLAGS_NBITS);
	u8 dscp2prio[YK3_MAX_DSCP];
	u8 dscp2tc[YK3_MAX_DSCP];

	struct yk3_dcb_pfc_cfg pfc;
};

void yk3_dcbnl_init(struct yk3_ndev_priv *ndev_priv);
void yk3_dcbnl_exit(struct yk3_ndev_priv *ndev_priv);
int yk3_get_pfc_mac_l1xon(struct net_device *ndev, u32 *val);
int yk3_get_pfc_mac_l1xoff(struct net_device *ndev, u32 *val);
int yk3_get_pfc_mac_l2xon(struct net_device *ndev, u32 *val);
int yk3_get_pfc_mac_l2xoff(struct net_device *ndev, u32 *val);
int yk3_get_pfc_pcie_l1xon(struct net_device *ndev, u32 *val);
int yk3_get_pfc_pcie_l1xoff(struct net_device *ndev, u32 *val);
int yk3_get_pfc_pcie_l2xon(struct net_device *ndev, u32 *val);
int yk3_get_pfc_pcie_l2xoff(struct net_device *ndev, u32 *val);
int yk3_set_pfc_mac_l1xon(struct net_device *ndev, u32 val);
int yk3_set_pfc_mac_l1xoff(struct net_device *ndev, u32 val);
int yk3_set_pfc_mac_l2xon(struct net_device *ndev, u32 val);
int yk3_set_pfc_mac_l2xoff(struct net_device *ndev, u32 val);
int yk3_set_pfc_pcie_l1xon(struct net_device *ndev, u32 val);
int yk3_set_pfc_pcie_l1xoff(struct net_device *ndev, u32 val);
int yk3_set_pfc_pcie_l2xon(struct net_device *ndev, u32 val);
int yk3_set_pfc_pcie_l2xoff(struct net_device *ndev, u32 val);

int yk3_set_pfc_pcie_l1vq(struct net_device *ndev, u32 xoff, u32 xon);

int yk3_rdma_setpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc);
int yk3_rdma_getpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc);
#else
static inline void yk3_dcbnl_init(struct yk3_ndev_priv *ndev_priv) {}
static inline void yk3_dcbnl_exit(struct yk3_ndev_priv *ndev_priv) {}
static inline int yk3_get_pfc_mac_l1xon(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_mac_l1xoff(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_mac_l2xon(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_mac_l2xoff(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_pcie_l1xon(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_pcie_l1xoff(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_pcie_l2xon(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_get_pfc_pcie_l2xoff(struct net_device *ndev, u32 *val) { return 0; }
static inline int yk3_set_pfc_mac_l1xon(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_mac_l1xoff(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_mac_l2xon(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_mac_l2xoff(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_pcie_l1xon(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_pcie_l1xoff(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_pcie_l2xon(struct net_device *ndev, u32 val) { return 0; }
static inline int yk3_set_pfc_pcie_l2xoff(struct net_device *ndev, u32 val) { return 0; }

static inline int yk3_set_pfc_pcie_l1vq(struct net_device *ndev, u32 xoff, u32 xon) { return 0; }

static inline int yk3_rdma_setpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc)
{ return 0; }
static inline int yk3_rdma_getpfc(struct yk3_pdev_priv *pdev_priv, struct ieee_pfc *pfc)
{ return 0; }
#endif

int yk3_debug_fc_init(struct yk3_ndev_priv *ndev_priv);
void yk3_debug_fc_exit(struct yk3_ndev_priv *ndev_priv);
int yk3_set_pause_srcaddr(struct yk3_pdev_priv *pdev_priv, const u8 *addr);
int yk3_set_prio_map_tc(struct yk3_ndev_priv *ndev_priv, u8 priority, u8 tc);

#endif // __YK3_DCBNL_H__
