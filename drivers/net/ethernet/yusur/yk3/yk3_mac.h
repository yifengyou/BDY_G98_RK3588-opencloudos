/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_MAC_H
#define _YK3_MAC_H

#include "yk3_mac_regs.h"

#define SPEED_10M	10
#define SPEED_100M	100
#define SPEED_1G	1000
#define SPEED_10G	10000
#define SPEED_25G	25000
#define SPEED_40G	40000
#define SPEED_50G	50000
#define SPEED_100G	100000
#define SPEED_200G	100000
#define SPEED_AUTO	100001

enum {
	MAC_FRAMES_XMIT_ERROR,
	MAC_FRAMES_XMIT_SIZELT64,
	MAC_FRAMES_XMIT_SIZEEQ64,
	MAC_FRAMES_XMIT_SIZE65TO127,
	MAC_FRAMES_XMIT_SIZE128TO255,
	MAC_FRAMES_XMIT_SIZE256TO511,
	MAC_FRAMES_XMIT_SIZE512TO1023,
	MAC_FRAMES_XMIT_SIZE1024TO1518,
	MAC_FRAMES_XMIT_SIZE1519TO2047,
	MAC_FRAMES_XMIT_SIZE2048TO4095,
	MAC_FRAMES_XMIT_SIZE4096TO8191,
	MAC_FRAMES_XMIT_SIZE8192TO9215,
	MAC_FRAMES_XMIT_SIZEGT9216,
	MAC_FRAMES_XMIT_PAUSE,
	MAC_FRAMES_XMIT_PRIPAUSE,
	MAC_FRAMES_XMIT_PRI0,
	MAC_FRAMES_XMIT_PRI1,
	MAC_FRAMES_XMIT_PRI2,
	MAC_FRAMES_XMIT_PRI3,
	MAC_FRAMES_XMIT_PRI4,
	MAC_FRAMES_XMIT_PRI5,
	MAC_FRAMES_XMIT_PRI6,
	MAC_FRAMES_XMIT_PRI7,

	MAC_FRAMES_RCVD_CRCERROR,
	MAC_FRAMES_RCVD_SIZELT64,
	MAC_FRAMES_RCVD_SIZEEQ64,
	MAC_FRAMES_RCVD_SIZE65TO127,
	MAC_FRAMES_RCVD_SIZE128TO255,
	MAC_FRAMES_RCVD_SIZE256TO511,
	MAC_FRAMES_RCVD_SIZE512TO1023,
	MAC_FRAMES_RCVD_SIZE1024TO1518,
	MAC_FRAMES_RCVD_SIZE1419TO2047,
	MAC_FRAMES_RCVD_SIZE2048TO4095,
	MAC_FRAMES_RCVD_SIZE4096TO8191,
	MAC_FRAMES_RCVD_SIZE8192TO9215,
	MAC_FRAMES_RCVD_SIZEGT9216,
	MAC_FRAMES_RCVD_PAUSE,
	MAC_FRAMES_RCVD_PRIPAUSE,
	MAC_FRAMES_RCVD_PRI0,
	MAC_FRAMES_RCVD_PRI1,
	MAC_FRAMES_RCVD_PRI2,
	MAC_FRAMES_RCVD_PRI3,
	MAC_FRAMES_RCVD_PRI4,
	MAC_FRAMES_RCVD_PRI5,
	MAC_FRAMES_RCVD_PRI6,
	MAC_FRAMES_RCVD_PRI7,

	MAC_DUMMY_TX_DISCARD_PHY,
	MAC_DUMMY_RX_DISCARD_PHY,
	MAC_STATE_STATISTICS_NUM,
};

enum {
	MAC_MODE_SPEED_DISABLED = 0,
	MAC_MODE_SPEED_10GBASE = 15,
	MAC_MODE_SPEED_10GBASE_FC = 16,
	MAC_MODE_SPEED_25GBASE = 21,
	MAC_MODE_SPEED_25GBASE_FC = 22,
	MAC_MODE_SPEED_25GBASE_RS_IEEE = 23,
	MAC_MODE_SPEED_25GBASE_RS_CONS = 24,
	MAC_MODE_SPEED_40GBASE = 26,
	MAC_MODE_SPEED_40GBASE_FC = 27,
	MAC_MODE_SPEED_50GBASE = 37,
	MAC_MODE_SPEED_50GBASE_FC = 38,
	MAC_MODE_SPEED_50GBASE_RS = 39,
	MAC_MODE_SPEED_100GBASE = 46,
	MAC_MODE_SPEED_100GBASE_RS = 47,
};

enum yk3_module_id {
	YK3_MODULE_ID_SFP = 0x3,
	YK3_MODULE_ID_QSFP = 0xC,
	YK3_MODULE_ID_QSFP_PLUS = 0xD,
	YK3_MODULE_ID_QSFP28 = 0x11,
	YK3_MODULE_ID_DSFP = 0x1B,
};

enum {
	YK3_MAC_LINK_TEST,
	YK3_MAC_SPEET_TEST,
	YK3_MAC_REG_TEST,
	YK3_MAC_INT_TEST,
	YK3_MAC_LOOPBACK_TEST,
};

enum yk3_port_type {
	YK3_10G = 0,
	YK3_25G,
	YK3_40G,
	YK3_100G,
	YK3_AUTONEG,
};

enum yk3_fec {
	YK3_OFFFEC = 0,
	YK3_FCFEC,
	YK3_RSFEC,
	YK3_AUTOFEC,
};

struct yk3_pf_intr_args {
	u32 irq_vector;
	enum yk3_mac_channel mac_ch;
	u8 pf_id;
};

struct yk3_mac {
	struct work_struct work;
	struct pci_dev *pdev;
	enum yk3_mac_channel mac_ch;
	struct timer_list link_timer;
	u32 irq_vector;
	u32 sts_regval;
	u32 speed_cfg;
	u32 speed;
	u8 fec_cfg;
	u8 port;
	u8 speed_cfg_autoneg:1;
	u8 reserved:7;
	u32 phy_stat_err;
	u32 link_int_cnt;
	u64 phy_stat[MAC_STATE_STATISTICS_NUM];
};

int yk3_mac_init(struct yk3_pdev_priv *pdev_priv);
void yk3_mac_exit(struct yk3_pdev_priv *pdev_priv);
int yk3_mac_ndev_init(struct net_device *ndev);
void yk3_mac_ndev_start(struct yk3_ndev_priv *ndev_priv);
void yk3_mac_self_offline_test(struct net_device *ndev,
			       struct ethtool_test *eth_test, u64 *data);
u32 yk3_mac_set_link_speed(struct yk3_ndev_priv *ndev_priv, u32 speed);
u32 yk3_mac_set_link_autoneg(struct yk3_ndev_priv *ndev_priv);
void yk3_mac_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data);
void yk3_mac_et_get_stats_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail);
int yk3_mac_et_get_sset_count(struct yk3_ndev_priv *ndev_priv);
int yk3_mac_set_fc(struct yk3_ndev_priv *ndev_priv, u8 mac_ch, u8 config);
int yk3_mac_set_pfc(struct yk3_ndev_priv *ndev_priv, u8 mac_ch, u8 config);
int yk3_mac_set_priv_flags(struct yk3_ndev_priv *ndev_priv, const u32 flags, bool enable);
int yk3_mac_ndev_set_phy_state(struct yk3_ndev_priv *ndev_priv, bool on);
void yk3_np_tm_speed_limit(struct yk3_pdev_priv *pdev_priv, u32 speed);
void yk3_edma_l2_vq(struct yk3_pdev_priv *pdev_priv, u32 speed);
int yk3_mac_get_connector(struct yk3_pdev_priv *pdev_priv);
u32 yk3_mac_get_speed(struct yk3_pdev_priv *pdev_priv, u32 *speed);
u32 yk3_mac_set_ifg(struct yk3_ndev_priv *ndev_priv, u8 ifg_num);

#endif /* _YK3_MAC_H */
