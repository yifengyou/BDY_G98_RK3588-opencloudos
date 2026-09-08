// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_mbox.h"
#include "yk3_lan.h"
#include "yk3_lan_priv.h"
#include "yk3_lan_regs.h"
#include "yk3_esw.h"
#include "yk3_ethtool.h"
#include "yk3_lan_trace.h"

#define YK3_LAN_PF_BASE_MAC_CH	2048
#define YK3_LAN_SYNC_MAC_TIMEOUT 200 // 200ms

enum {
	YK3_LAN_TX_SRC_MAC_FILTER_DROP_COUNTER,
};

static const char yk3_lan_et_stats_strings[][ETH_GSTRING_LEN] = {
	"tx_spoofcheck_drop_packets",
};

struct yk3_lan_mbox_opcode_cb {
	u16 opcode;
	void (*cb)(struct yk3_mbox_msg *msg, void *param);
};

static int yk3_lan_ndev_sync_mac(struct yk3_ndev_priv *ndev_priv, const u8 *addr,
				 bool uc, bool add, gfp_t gfp_flags);

static inline void yk3_lan_reset_regs(void __iomem *hw_addr, u32 offset, u32 buf_len)
{
	u32 i = 0;

	for (i = 0; i < (buf_len / sizeof(u32)); i++)
		yk3_wr32(hw_addr, offset + i * 0x4, 0);
}

static void yk3_lan_hw_reset(void __iomem *hw_addr)
{
	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_TX_DST_QSET_BASE,
			   YK3_LAN_TX_DST_QSET_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_TX_QSET_QINQ_CFG_BASE,
			   YK3_LAN_TX_QSET_QINQ_CFG_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_TX_QSET_OFFLOAD_VEB_BASE,
			   YK3_LAN_TX_QSET_OFFLOAD_VEB_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_TX_VPORT_BASE,
			   YK3_LAN_TX_VPORT_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_RX_QSET_BASE,
			   YK3_LAN_RX_QSET_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_ESW_MAC_FILTER_BASE,
			   YK3_LAN_ESW_MAC_FILTER_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_ESW_MAC_QSET_BMP_BASE,
			   YK3_LAN_ESW_MAC_QSET_BMP_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_ESW_VLAN_FILTER_TYPE0_BASE,
			   YK3_LAN_ESW_VLAN_FILTER_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_ESW_VLAN_FILTER_TYPE1_BASE,
			   YK3_LAN_ESW_VLAN_FILTER_TOTAL_LEN);

	yk3_lan_reset_regs(hw_addr,
			   YK3_LAN_ESW_VLAN_QSET_BMP_BASE,
			   YK3_LAN_ESW_VLAN_QSET_BMP_TOTAL_LEN);
}

static void yk3_lan_init_tx_global(void __iomem *hw_addr)
{
	u32 reg = 0;

	reg = FIELD_PREP(YK3_LAN_TX_GLOBAL_SWITCH_SUB_EN, 1) |
	      FIELD_PREP(YK3_LAN_TX_GLOBAL_OFFLOAD_SUB_EN, 1) |
	      FIELD_PREP(YK3_LAN_TX_GLOBAL_VLANTAG_OFFLOAD_SUB_EN, 1) |
	      FIELD_PREP(YK3_LAN_TX_GLOBAL_LAN_PRE_SUB_EN, 1) |
	      FIELD_PREP(YK3_LAN_TX_GLOBAL_LAN_PARSER_EN, 1);
	yk3_wr32(hw_addr, YK3_LAN_TX_GLOBAL_SUBSYSTEM_EN, reg);

	reg = FIELD_PREP(YK3_LAN_TX_GLOBAL_LOOP_ENABLE, 0) |
	      FIELD_PREP(YK3_LAN_TX_GLOBAL_VLAN_BITFLAG_EN, 1);
	yk3_wr32(hw_addr, YK3_LAN_TX_GLOBAL_GLOBAL_MODE, reg);
}

static void yk3_lan_init_rx_global(void __iomem *hw_addr)
{
	u32 reg = 0;

	reg = FIELD_PREP(YK3_LAN_RX_PARSER_ENABLE, 1);
	yk3_wr32(hw_addr, YK3_LAN_RX_PARSER_EN, reg);
}

static void yk3_lan_init_rx_qset_hash(u16 qset, void __iomem *hw_addr)
{
	u32 reg;
	u8 hash_mode;
	/* init qset hash */
	hash_mode = YK3_RXH_L3_PROTO |
		    YK3_RXH_IP_DST |
		    YK3_RXH_IP_SRC |
		    YK3_RXH_L4_SRC |
		    YK3_RXH_L4_DST;

	reg = FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_TCP, hash_mode) |
	      FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_UDP, hash_mode) |
	      FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_TCP, hash_mode) |
	      FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_UDP, hash_mode) |
	      FIELD_PREP(YK3_LAN_RX_QSET_TUNNEL_PKT_HASH_SEL, 0);

	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_HASH(qset), reg);
	yk3_wr32(hw_addr, YK3_LAN_RX_REP_QSET_HASH(qset), reg);
}

static void yk3_lan_init_rx_qset_mtu(u16 qset, void __iomem *hw_addr)
{
	u32 reg;

	reg = FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_MTU, ETH_DATA_LEN) |
	      FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VLAN_STRIP_MODE, YK3_LAN_RX_VLAN_STRIP_ALL) |
	      FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_OVER_MTU_DROP, 0) |
	      FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_FCS_ERR_DROP, 0) |
	      FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_CHKSUM_ERR_DROP, 0) |
	      FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VPORT, 0);

	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_MTU(qset), reg);
	yk3_wr32(hw_addr, YK3_LAN_RX_REP_QSET_MTU(qset), reg);
}

static void yk3_lan_init_rx_qset_vlan(u16 qset, void __iomem *hw_addr)
{
	u32 reg = 0;

	/* YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS 1 == disable bypass */
	reg = FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS, 1) |
	      FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_STAG_VLAN_VALID, 1) |
	      FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_CTAG_VLAN_VALID, 1) |
	      /* YK3_LAN_RX_QSET_VLAN_MISC_SRC_EQ_DST_DROP 0 is drop */
	      FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_SRC_EQ_DST_DROP, 0);

	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
}

static void yk3_lan_init_rx_vf_qset_vlan(u16 qset, void __iomem *hw_addr)
{
	u32 reg;

	/* YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS 1 == disable bypass */
	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_BC_DISABLE, 1);
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_SMARTNIC_PROMISCUOUS, 1);
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, 1);
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PROMISCUOUS_ENABLE, 1);
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_SRC_EQ_DST_DROP, 1);
	reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS, 1) |
	       FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_STAG_VLAN_VALID, 1) |
	       FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_CTAG_VLAN_VALID, 1);

	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
}

static void yk3_lan_init_tx_ndev_config(u16 qset, void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));

	reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VLAN_TYPE, 0xffff);
	reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VLAN_TYPE, 0x88a8);

	reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_INNER_VLAN_TRUST, 0x1);
	reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_INNER_VLAN_TRUST, 1);

	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
}

static void yk3_lan_init_tx_qset(u16 qset, void __iomem *hw_addr)
{
	u32 reg;

	reg = FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VLAN_TYPE, 0x88a8) |
	      FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VEB_EN, 1) |
	      FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_QINQ_VLAN_MODE, 0) |
	      FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_INNER_VLAN_TRUST, 1) |
	      FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_SRC_MAC_FILTER_EN, 0);
	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);

	reg = 0;
	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_QINQ_CFG(qset), reg);
}

static void yk3_lan_init_counter(u16 qset, void __iomem *hw_addr)
{
	yk3_wr64(hw_addr, YK3_LAN_TX_SRC_MAC_FILTER_DROP_CNT(qset), 0);
}

static void yk3_lan_init_all_qset(void __iomem *hw_addr)
{
	u16 qset;

	for (qset = 0; qset < YK3_LAN_RX_QSET_NUM; qset++) {
		yk3_lan_init_rx_qset_hash(qset, hw_addr);
		yk3_lan_init_rx_qset_mtu(qset, hw_addr);
		yk3_lan_init_rx_qset_vlan(qset, hw_addr);
	}

	for (qset = 0; qset < YK3_LAN_TX_QSET_NUM; qset++)
		yk3_lan_init_tx_qset(qset, hw_addr);
}

static void yk3_lan_init_qset(u16 qset, void __iomem *hw_addr)
{
	yk3_lan_init_rx_qset_hash(qset, hw_addr);
	yk3_lan_init_rx_qset_mtu(qset, hw_addr);
	yk3_lan_init_rx_qset_vlan(qset, hw_addr);
	yk3_lan_init_tx_qset(qset, hw_addr);
	yk3_lan_init_counter(qset, hw_addr);
}

static void yk3_lan_init_hw(void __iomem *hw_addr)
{
	yk3_lan_init_tx_global(hw_addr);

	yk3_lan_init_rx_global(hw_addr);

	yk3_lan_init_all_qset(hw_addr);
}

static void yk3_lan_init_qset_path_legacy(u8 mac_ch, u16 qset, bool enable,
					  void __iomem *hw_addr)
{
	u32 reg;
	u16 dst_qset;
	u8 val;

	if (enable) {
		/* init default function mapping mac channel */
		dst_qset = YK3_LAN_PF_BASE_MAC_CH + mac_ch;
		reg = FIELD_PREP(YK3_LAN_TX_DST_QSET_VALUE, dst_qset);
		yk3_wr32(hw_addr, YK3_LAN_TX_DST_QSET(qset), reg);

		/* init mac channel attribute */
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		val = FIELD_GET(YK3_LAN_RX_QSET_VLAN_MISC_PORT_ENABLE, reg);
		val |= 1 << mac_ch;
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PORT_ENABLE, val);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	} else {
		/* uninit default function mapping mac channel */
		yk3_wr32(hw_addr, YK3_LAN_TX_DST_QSET(qset), 0);
		/* uninit mac channel attribute */
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		val = FIELD_GET(YK3_LAN_RX_QSET_VLAN_MISC_PORT_ENABLE, reg);
		val &= ~(1 << mac_ch);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PORT_ENABLE, 0xff);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PORT_ENABLE, val);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	}
}

static void yk3_lan_reset_ndev_config(u16 qset, void __iomem *hw_addr)
{
	yk3_lan_init_rx_qset_hash(qset, hw_addr);
	yk3_lan_init_rx_qset_mtu(qset, hw_addr);
	yk3_lan_init_rx_vf_qset_vlan(qset, hw_addr);
	yk3_lan_init_tx_ndev_config(qset, hw_addr);
}

static void yk3_lan_init_port(u16 qset, u8 mac_ch, bool enable, void __iomem *hw_addr)
{
	if (enable) {
		yk3_lan_init_qset(qset, hw_addr);
		yk3_lan_init_qset_path_legacy(mac_ch, qset, true, hw_addr);
	} else {
		yk3_lan_init_qset_path_legacy(mac_ch, qset, false, hw_addr);
	}
}

static void yk3_lan_set_rxhash(u16 qset, u8 fields[], void __iomem *hw_addr)
{
	u32 reg;
	int i;

	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_HASH(qset));
	for (i = 0; i < YK3_RXFH_MAX_FLOW_TYPE; i++) {
		switch (i) {
		case YK3_RXFH_TCP_V4_FLOW:
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_TCP, 0x1f);
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_TCP, fields[i] & 0x1f);
			break;
		case YK3_RXFH_UDP_V4_FLOW:
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_UDP, 0x1f);
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_UDP, fields[i] & 0x1f);
			break;
		case YK3_RXFH_TCP_V6_FLOW:
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_TCP, 0x1f);
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_TCP, fields[i] & 0x1f);
			break;
		case YK3_RXFH_UDP_V6_FLOW:
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_UDP, 0x1f);
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_UDP, fields[i] & 0x1f);
			break;
		default:
			break;
		}
	}
	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_HASH(qset), reg);
}

static void yk3_lan_set_feature(u16 qset, netdev_features_t changed,
				netdev_features_t features, void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_MTU(qset));
	if (changed & NETIF_F_HW_VLAN_CTAG_RX) {
		if (features & NETIF_F_HW_VLAN_CTAG_RX)
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VLAN_STRIP_MODE,
					  YK3_LAN_RX_VLAN_STRIP_CTAG);
		else
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VLAN_STRIP_MODE,
					   YK3_LAN_RX_VLAN_STRIP_CTAG);
	}

	if (changed & NETIF_F_HW_VLAN_STAG_RX) {
		if (features & NETIF_F_HW_VLAN_STAG_RX)
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VLAN_STRIP_MODE,
					  YK3_LAN_RX_VLAN_STRIP_STAG);
		else
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_VLAN_STRIP_MODE,
					   YK3_LAN_RX_VLAN_STRIP_STAG);
	}
	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_MTU(qset), reg);

	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
	if (changed & NETIF_F_HW_VLAN_CTAG_FILTER) {
		if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_CTAG_VLAN_VALID, 1);
		else
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_CTAG_VLAN_VALID, 1);
	}

	if (changed & NETIF_F_HW_VLAN_STAG_FILTER) {
		if (features & NETIF_F_HW_VLAN_STAG_FILTER)
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_STAG_VLAN_VALID, 1);
		else
			reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_STAG_VLAN_VALID, 1);
	}
	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);

	reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
	if (changed & NETIF_F_HW_VLAN_CTAG_TX || changed & NETIF_F_HW_VLAN_STAG_TX) {
		/*set inner vlan trust*/
		reg &= ~YK3_LAN_TX_QSET_OFFLOAD_VEB_INNER_VLAN_TRUST;
		if (features & NETIF_F_HW_VLAN_CTAG_TX || features & NETIF_F_HW_VLAN_STAG_TX)
			reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_INNER_VLAN_TRUST, 1);
	}
	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
}

static void yk3_lan_set_mtu(u16 qset, int mtu, void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_MTU(qset));
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_MTU, 0x3fff);
	reg |= FIELD_PREP(YK3_LAN_RX_QSET_MTU_MISC_MTU, mtu);
	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_MTU(qset), reg);
}

static void yk3_lan_set_rss_hash_fields(u16 qset, u32 flow_type, u8 fields,
					void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_HASH(qset));
	switch (flow_type) {
	case YK3_RXFH_TCP_V4_FLOW:
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_TCP, 0x1f);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_TCP, fields & 0x1f);
		break;
	case YK3_RXFH_UDP_V4_FLOW:
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_UDP, 0x1f);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV4_UDP, fields & 0x1f);
		break;
	case YK3_RXFH_TCP_V6_FLOW:
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_TCP, 0x1f);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_TCP, fields & 0x1f);
		break;
	case YK3_RXFH_UDP_V6_FLOW:
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_UDP, 0x1f);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_HASH_MODE_IPV6_UDP, fields & 0x1f);
		break;
	default:
		break;
	}
	yk3_wr32(hw_addr, YK3_LAN_RX_QSET_HASH(qset), reg);
}

static void yk3_lan_set_vf_vlan(u16 qset, u16 vlan_id, __be16 proto, u8 qos,
				void __iomem *hw_addr)
{
	u32 reg;

	if (vlan_id) {
		// tx vf vlan regs
		reg = FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_QINQ_TYPE, ntohs(proto)) |
		      FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_PRI, qos) |
		      FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_PVID, vlan_id);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_QINQ_CFG(qset), reg);

		reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
		reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_QINQ_VLAN_MODE, 1);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
		// rx vf vlan regs
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_VALID, 1);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID, 0xfff);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_TYPE, 1);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID, vlan_id) |
		       FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_VALID, 1);
		if (proto == htons(ETH_P_8021AD))
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_TYPE, 1);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	} else {
		reg = 0;
		// tx vf vlan regs
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_QINQ_CFG(qset), reg);
		reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
		reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_QINQ_VLAN_MODE, 1);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
		// rx vf vlan regs
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_VALID, 1);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID, 0xfff);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_TYPE, 1);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	}
}

static void yk3_lan_set_tx_vf_vlan(u16 qset, u16 vlan_id, __be16 proto, u8 qos,
				   void __iomem *hw_addr)
{
	u32 reg;

	if (vlan_id) {
		// tx vf vlan regs
		reg = FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_QINQ_TYPE, ntohs(proto)) |
		      FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_PRI, qos) |
		      FIELD_PREP(YK3_LAN_TX_QSET_QINQ_CFG_PVID, vlan_id);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_QINQ_CFG(qset), reg);

		reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
		reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_QINQ_VLAN_MODE, 1);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
	} else {
		reg = 0;
		// tx vf vlan regs
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_QINQ_CFG(qset), reg);
		reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
		reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_QINQ_VLAN_MODE, 1);
		yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
	}
}

static void yk3_lan_set_ndev_flags(u16 qset, u32 changed, u32 flags, void __iomem *hw_addr)
{
	u32 reg;
	u32 enable;

	if (changed & IFF_PROMISC) {
		enable = 0;
		if (flags & IFF_PROMISC)
			enable = 1;
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PROMISCUOUS_ENABLE, 1);
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, 1);
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PROMISCUOUS_ENABLE, enable);
#ifdef ALL_MC_FEATURE
		if (flags & IFF_ALLMULTI)
			enable = 1;
#endif
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, enable);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	}
	// TODO support IFF_MULTICAST, IFF_BROADCAST
	if (changed & IFF_ALLMULTI) {
		enable = 0;
		if (flags & IFF_ALLMULTI)
			enable = 1;
	}
}

static void yk3_lan_set_ethtool_priv_flags(u16 qset, u32 flag, u8 enable,
					   void __iomem *hw_addr)
{
	u32 reg;

	if (flag == YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY) {
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_HASH(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_TUNNEL_PKT_HASH_SEL, 1);
		enable = enable ? 0 : 1; // hw define 1 use underlay info, 0 use overlay info
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_TUNNEL_PKT_HASH_SEL, enable);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_HASH(qset), reg);
	} else if (flag == YK3_ET_PFLAG_PVID_MISS_UPLOAD) {
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS, 1);
		enable = enable ? 0 : 1; // hw define 0 bypass, 1 check
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_PVID_BYPASS, enable);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	} else if (flag == YK3_ET_FFLAG_SRC_EQ_DST_DROP) {
		reg = yk3_rd32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset));
		reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_SRC_EQ_DST_DROP, 1);
		enable = enable ? 0 : 1; // hw define 0 drop, 1 bypass
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_SRC_EQ_DST_DROP, enable);
		yk3_wr32(hw_addr, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
	}
}

static void yk3_lan_set_evb_mode(u16 qset, u16 mode, void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));
	reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VEB_EN, 1);
	reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_VEB_EN,
			  mode == BRIDGE_MODE_VEPA ? 0 : 1);
	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
}

static void yk3_lan_set_vf_spoofchk(u16 qset, bool check, void __iomem *hw_addr)
{
	u32 reg;

	reg = yk3_rd32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset));

	reg &= ~FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_SRC_MAC_FILTER_EN, 1);
	if (check)
		reg |= FIELD_PREP(YK3_LAN_TX_QSET_OFFLOAD_VEB_SRC_MAC_FILTER_EN, 1);
	yk3_wr32(hw_addr, YK3_LAN_TX_QSET_OFFLOAD_VEB(qset), reg);
}

int yk3_lan_ndev_set_mac(struct net_device *ndev, u8 *old_mac, u8 *mac)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_mac_msg *mac_task;
	struct yk3_lan_mbox_ack_msg *ack;
	int ret;

	YK3_LAN_DBG_CNT_INC(intf_mac_upd);
	// let pf update vf mac info
	if (yk3_pdev_is_vf(pdev_priv)) {
		memset(&mbox_msg, 0, sizeof(mbox_msg));
		mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
		mac_task->qset = ndev_priv->qsetid;
		memcpy(mac_task->mac, mac, ETH_ALEN);
		mac_task->mac_op = YK3_LAN_MBOX_MAC_UPD;

		mbox_msg.opcode = YK3_MBOX_OPCODE_UPD_VF_MAC;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 3000;
		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg);
		if (ret != 0) {
			yk3_dev_err("send update vf mac mbox message errno %d failed!\n",
				    ret);
			YK3_LAN_DBG_CNT_INC(intf_mac_upd_err);
			return -EFAULT;
		}
		ack = (struct yk3_lan_mbox_ack_msg *)ack_msg.data;
		if (ack->ret_code != YK3_LAN_MBOX_RET_OK) {
			if (ack->ret_code == YK3_LAN_MBOX_RET_DUP_MAC) {
				ret = -EEXIST;
				yk3_dev_err("vf set duplicate mac!\n");
			} else {
				ret = -EFAULT;
				yk3_dev_err("vf set mac failed err code %d failed!\n",
					    ack->ret_code);
				YK3_LAN_DBG_CNT_INC(intf_mac_upd_err);
			}
			return ret;
		}
	}

	memset(&mbox_msg, 0, sizeof(mbox_msg));
	memset(&mbox_opt, 0, sizeof(mbox_opt));
	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	mac_task->qset = ndev_priv->qsetid;
	memcpy(mac_task->mac, mac, ETH_ALEN);
	if (old_mac)
		memcpy(mac_task->old_mac, old_mac, ETH_ALEN);
	mac_task->mac_op = YK3_LAN_MBOX_MAC_UPD;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
	mbox_msg.dst_id = yk3_mbox_master_id();

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("send upd intf mac mbox message errno %d failed!\n", ret);
		YK3_LAN_DBG_CNT_INC(intf_mac_upd_err);
		return -EFAULT;
	}

	return 0;
}

int yk3_lan_ndev_set_mtu(struct net_device *ndev, int new_mtu)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_lan_mbox_mtu_msg *mtu_task;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	int ret;

	YK3_LAN_DBG_CNT_INC(ndev_set_mtu);
	if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_lan_set_mtu(ndev_priv->qsetid, new_mtu, pdev_priv->lan->hw_addr);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		mtu_task = (struct yk3_lan_mbox_mtu_msg *)mbox_msg.data;
		mtu_task->qset = ndev_priv->qsetid;
		mtu_task->mtu = new_mtu;
		mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MTU;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send mtu mbox message errno %d failed!\n", ret);
			YK3_LAN_DBG_CNT_INC(ndev_set_mtu_err);
			return -EFAULT;
		}
	}

	return 0;
}

int yk3_lan_ndev_vlan(struct net_device *ndev, __be16 proto, u16 vlan_id, bool enable)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_vlan_msg *vlan_task;
	int ret;

	vlan_task = (struct yk3_lan_mbox_vlan_msg *)mbox_msg.data;
	vlan_task->vlan_id = vlan_id;
	vlan_task->proto = proto;
	vlan_task->qset = ndev_priv->qsetid;
	if (enable) {
		vlan_task->vlan_op = YK3_LAN_MBOX_VLAN_ADD;
		YK3_LAN_DBG_CNT_INC(vlan_add);
	} else {
		vlan_task->vlan_op = YK3_LAN_MBOX_VLAN_DEL;
		YK3_LAN_DBG_CNT_INC(vlan_del);
	}

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_VLAN;
	mbox_msg.dst_id = yk3_mbox_master_id();

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("send vlan mbox message errno %d failed!\n", ret);
		if (enable)
			YK3_LAN_DBG_CNT_INC(vlan_add_err);
		else
			YK3_LAN_DBG_CNT_INC(vlan_del_err);
		return -EFAULT;
	}

	return 0;
}

int yk3_lan_ndev_set_feature(struct net_device *ndev, netdev_features_t changed,
			     netdev_features_t features)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_lan_mbox_ndev_feature_msg *feature_task;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	u8 fields[YK3_RXFH_MAX_FLOW_TYPE] = {};
	int i;
	int ret;

	YK3_LAN_DBG_CNT_INC(ndev_set_feature);
	if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_lan_set_feature(ndev_priv->qsetid, changed, features,
				    pdev_priv->lan->hw_addr);
		if (changed & NETIF_F_RXHASH) {
			if (features & NETIF_F_RXHASH) {
				for (i = 0; i < YK3_RXFH_MAX_FLOW_TYPE; i++)
					fields[i] = yk3_get_rxfh_conf(ndev_priv, i);
			}
			yk3_lan_set_rxhash(ndev_priv->qsetid, fields, pdev_priv->lan->hw_addr);
		}
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		feature_task = (struct yk3_lan_mbox_ndev_feature_msg *)mbox_msg.data;
		feature_task->qset = ndev_priv->qsetid;
		feature_task->changed = changed;
		feature_task->features = features;

		if (changed & NETIF_F_RXHASH) {
			if (features & NETIF_F_RXHASH) {
				for (i = 0; i < YK3_RXFH_MAX_FLOW_TYPE; i++)
					feature_task->rss_fields[i] =
						yk3_get_rxfh_conf(ndev_priv, i);
			}
		}

		mbox_msg.opcode = YK3_MBOX_OPCODE_SET_NDEV_FEATURES;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send set ndev features mbox message errno %d failed!\n",
				    ret);
			YK3_LAN_DBG_CNT_INC(ndev_set_feature_err);
			return -EFAULT;
		}
	}
	return 0;
}

static int yk3_lan_ndev_sync_mac(struct yk3_ndev_priv *ndev_priv, const u8 *addr,
				 bool uc, bool add, gfp_t gfp_flags)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_mac_msg *mac_task;
	unsigned long timeout;
	int ret;

	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	mac_task->qset = ndev_priv->qsetid;
	memcpy(mac_task->mac, addr, ETH_ALEN);
	if (uc) {
		if (add) {
			mac_task->mac_op = YK3_LAN_MBOX_UC_MAC_ADD;
			YK3_LAN_DBG_CNT_INC(uc_mac_add);
		} else {
			mac_task->mac_op = YK3_LAN_MBOX_UC_MAC_DEL;
			YK3_LAN_DBG_CNT_INC(uc_mac_del);
		}
	} else {
		if (add) {
			mac_task->mac_op = YK3_LAN_MBOX_MC_MAC_ADD;
			YK3_LAN_DBG_CNT_INC(mc_mac_add);
		} else {
			mac_task->mac_op = YK3_LAN_MBOX_MC_MAC_DEL;
			YK3_LAN_DBG_CNT_INC(mc_mac_del);
		}
	}

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
	mbox_msg.dst_id = yk3_mbox_master_id();

	if (gfp_flags == GFP_ATOMIC) {
		ret = yk3_mbox_send_msg_atomic(pdev_priv, &mbox_msg);
		if (ret == -EAGAIN) {
			timeout = jiffies + msecs_to_jiffies(YK3_LAN_SYNC_MAC_TIMEOUT);
			udelay(1000);
			while (time_before(jiffies, timeout)) {
				ret = yk3_mbox_send_msg_atomic(pdev_priv, &mbox_msg);
				if (ret != -EAGAIN)
					break;
				udelay(1000);
			}
		}
	} else {
		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	}

	if (ret != 0) {
		yk3_dev_err("send sync mac mbox message errno %d failed!\n", ret);
		if (uc)
			if (add)
				YK3_LAN_DBG_CNT_INC(uc_mac_add_err);
			else
				YK3_LAN_DBG_CNT_INC(uc_mac_del_err);
		else
			if (add)
				YK3_LAN_DBG_CNT_INC(mc_mac_add_err);
			else
				YK3_LAN_DBG_CNT_INC(mc_mac_del_err);
		return -1;
	}

	return 0;
}

static int yk3_lan_ndev_sync_mc_atomic(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, false, true, GFP_ATOMIC);
}

static int yk3_lan_ndev_unsync_mc_atomic(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, false, false, GFP_ATOMIC);
}

static int yk3_lan_ndev_sync_uc_atomic(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, true, true, GFP_ATOMIC);
}

static int yk3_lan_ndev_unsync_uc_atomic(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, true, false, GFP_ATOMIC);
}

int yk3_lan_ndev_sync_mc(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, false, true, GFP_KERNEL);
}

int yk3_lan_ndev_sync_uc(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, true, true, GFP_KERNEL);
}

int yk3_lan_ndev_unsync_mc(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, false, false, GFP_KERNEL);
}

int yk3_lan_ndev_unsync_uc(struct net_device *ndev, const u8 *addr)
{
	return yk3_lan_ndev_sync_mac(netdev_priv(ndev), addr, true, false, GFP_KERNEL);
}

#ifdef ALL_MC_FEATURE
static void yk3_lan_ndev_set_allmulti(struct yk3_pdev_priv *pdev_priv, u16 qset,
				      bool enable, u32 flags)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_lan_mbox_set_all_mc_msg *all_mc_task;
	int ret;

	all_mc_task = (struct yk3_lan_mbox_set_all_mc_msg *)mbox_msg.data;
	all_mc_task->qset = qset;
	all_mc_task->flags = flags;
	if (enable)
		all_mc_task->enable = 1;
	else
		all_mc_task->enable = 0;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_ALL_MC;
	mbox_msg.dst_id = yk3_mbox_master_id();

	ret = yk3_mbox_send_msg_atomic(pdev_priv, &mbox_msg);
	if (ret != 0)
		yk3_dev_err("set all multicast send mbox message errno %d failed!\n", ret);
}
#endif

int yk3_lan_ndev_set_rx_mode(struct net_device *ndev, u32 changed, u32 flags)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_lan_mbox_ndev_flags_msg *flags_task;
	int ret;

	if (changed) {
		YK3_LAN_DBG_CNT_INC(ndev_set_flags);
		if (yk3_pdev_is_pf(pdev_priv)) {
			yk3_lan_set_ndev_flags(ndev_priv->qsetid, changed, flags,
					       pdev_priv->lan->hw_addr);
#ifdef ALL_MC_FEATURE
			if (changed & IFF_ALLMULTI)
				yk3_lan_ndev_set_allmulti(pdev_priv, ndev_priv->qsetid,
							  flags & IFF_ALLMULTI ? true : false,
							  flags);
#endif
		} else if (yk3_pdev_is_vf(pdev_priv)) {
			flags_task = (struct yk3_lan_mbox_ndev_flags_msg *)mbox_msg.data;
			flags_task->qset = ndev_priv->qsetid;
			flags_task->changed = changed;
			flags_task->flags = flags;

			mbox_msg.opcode = YK3_MBOX_OPCODE_SET_NDEV_FLAGS;
			mbox_msg.dst_id = yk3_mbox_pf_id(0);

			ret = yk3_mbox_send_msg_atomic(pdev_priv, &mbox_msg);
			if (ret != 0) {
				yk3_dev_err("send mbox set ndev flags message errno %d failed!\n",
					    ret);
				YK3_LAN_DBG_CNT_INC(ndev_set_flags_err);
			}
		}
	}

	/* ndev take over by umd no need sync kernel mac list */
	if (ndev_priv->umd_enable)
		return 0;

	__dev_mc_sync(ndev, yk3_lan_ndev_sync_mc_atomic, yk3_lan_ndev_unsync_mc_atomic);
	__dev_uc_sync(ndev, yk3_lan_ndev_sync_uc_atomic, yk3_lan_ndev_unsync_uc_atomic);

	return 0;
}

int yk3_lan_ethtool_set_rxnfc(struct net_device *ndev, const u32 flow_type, const u8 fields)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_rss_msg *rss_task;
	int ret;

	YK3_LAN_DBG_CNT_INC(ndev_set_rss);
	if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_lan_set_rss_hash_fields(ndev_priv->qsetid, flow_type, fields,
					    pdev_priv->lan->hw_addr);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		rss_task = (struct yk3_lan_mbox_rss_msg *)mbox_msg.data;
		rss_task->qset = ndev_priv->qsetid;
		rss_task->fields = fields;
		rss_task->flow_type = flow_type;

		mbox_msg.opcode = YK3_MBOX_OPCODE_SET_RSS_HASH;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send rss hash mbox message errno %d failed!\n",
				    ret);
			YK3_LAN_DBG_CNT_INC(ndev_set_rss_err);
			return -EFAULT;
		}
	}

	return 0;
}

int yk3_lan_ethtool_set_priv_flags(struct net_device *ndev, const u32 flags, bool enable)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_ethtool_priv_flags_msg *priv_flags_task;
	int ret;

	YK3_LAN_DBG_CNT_INC(ndev_set_priv_flags);
	if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_lan_set_ethtool_priv_flags(ndev_priv->qsetid, flags, enable,
					       pdev_priv->lan->hw_addr);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		priv_flags_task = (struct yk3_lan_mbox_ethtool_priv_flags_msg *)mbox_msg.data;
		priv_flags_task->qset = ndev_priv->qsetid;
		priv_flags_task->flags = flags;
		if (enable)
			priv_flags_task->enable = 1;
		else
			priv_flags_task->enable = 0;

		mbox_msg.opcode = YK3_MBOX_OPCODE_SET_ETHTOOL_PRIV;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send ethtool priv flags mbox message errno %d failed!\n",
				    ret);
			YK3_LAN_DBG_CNT_INC(ndev_set_priv_flags_err);
			return -EFAULT;
		}
	} else {
		return -EOPNOTSUPP;
	}

	return 0;
}

int yk3_lan_et_get_sset_count(struct yk3_ndev_priv *ndev_priv)
{
	if (!yk3_ndev_is_pf(ndev_priv))
		return 0;

	return ARRAY_SIZE(yk3_lan_et_stats_strings);
}

void yk3_lan_et_get_stats_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail)
{
	u8 *cur = data;
	int i;

	if (!yk3_ndev_is_pf(ndev_priv))
		return;

	for (i = 0; i < ARRAY_SIZE(yk3_lan_et_stats_strings); i++) {
		memcpy(cur, yk3_lan_et_stats_strings[i], ETH_GSTRING_LEN);
		cur += ETH_GSTRING_LEN;
	}

	*tail = cur;
}

void yk3_lan_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data)
{
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	u64 drop_cnt = 0;
	int i;

	if (!yk3_ndev_is_pf(ndev_priv))
		return;

	pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	if (!pdev_priv || !pdev_priv->lan) {
		yk3_net_warn("%s pdev_priv or lan null\n", __func__);
		goto no_cnt;
	}

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (!sriov_priv)
		goto no_cnt;

	for (i = 0; i < sriov_priv->num_vfs; i++) {
		vf_info = yk3_sriov_get_vfinfo(sriov_priv, i);
		if (!vf_info)
			continue;
		drop_cnt += yk3_rd64(pdev_priv->lan->hw_addr,
				     YK3_LAN_TX_SRC_MAC_FILTER_DROP_CNT(vf_info->qsetid));
	}
	data[YK3_LAN_TX_SRC_MAC_FILTER_DROP_COUNTER] = drop_cnt;
	yk3_sriov_put_priv(sriov_priv);
	return;
no_cnt:
	data[YK3_LAN_TX_SRC_MAC_FILTER_DROP_COUNTER] = 0;
}

void yk3_lan_ndev_set_evb_mode(struct net_device *ndev, u16 mode)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int i;

	YK3_LAN_DBG_CNT_INC(set_evb_mode);
	for (i = 0; i < pdev_priv->sriov_priv->num_vfs; i++)
		yk3_lan_set_evb_mode(pdev_priv->sriov_priv->vf_info[i].qsetid,
				     mode, pdev_priv->lan->hw_addr);
	yk3_lan_set_evb_mode(ndev_priv->qsetid, mode, pdev_priv->lan->hw_addr);
}

int yk3_lan_ndev_set_vf_mac(struct net_device *ndev, int vf, u16 qset,
			    u8 *old_mac, u8 *mac)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_mac_msg *mac_task;
	int ret;

	YK3_LAN_DBG_CNT_INC(vf_set_mac);
	// send mac message to mgr pf
	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	mac_task->qset = qset;
	memcpy(mac_task->mac, mac, ETH_ALEN);
	memcpy(mac_task->old_mac, old_mac, ETH_ALEN);
	mac_task->mac_op = YK3_LAN_MBOX_MAC_UPD;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
	mbox_msg.dst_id = yk3_mbox_master_id();

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("send mbox message errno %d failed!\n", ret);
		YK3_LAN_DBG_CNT_INC(vf_set_mac_err);
		return -1;
	}
	// send mac message to vf
	memset(&mbox_msg, 0, sizeof(mbox_msg));
	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	memcpy(mac_task->mac, mac, ETH_ALEN);
	memcpy(mac_task->old_mac, old_mac, ETH_ALEN);
	mac_task->mac_op = YK3_LAN_MBOX_VF_MAC_UPD;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_VF_MAC;
	mbox_msg.dst_id = yk3_mbox_vf_id(vf);

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("send mbox message errno %d failed!\n", ret);
		YK3_LAN_DBG_CNT_INC(vf_set_mac_err);
		return -1;
	}

	return 0;
}

int yk3_lan_ndev_set_vf_vlan(struct net_device *ndev, u16 vf_qset, u16 vlan,
			     u8 qos, __be16 proto)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	YK3_LAN_DBG_CNT_INC(vf_vlan);
	yk3_lan_set_vf_vlan(vf_qset, vlan, proto, qos, pdev_priv->lan->hw_addr);

	return 0;
}

int yk3_lan_ndev_set_vf_trust(struct net_device *ndev, int vf, bool trust)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_vf_info *vf_info = &pdev_priv->sriov_priv->vf_info[vf];
	u32 changed = 0, ndev_flags = 0;

	YK3_LAN_DBG_CNT_INC(vf_trust);
	if (trust) {
		// TODO support IFF_ALLMULTI?
		if (vf_info->ndev_flags & IFF_PROMISC) {
			changed |= IFF_PROMISC;
			ndev_flags |= IFF_PROMISC;
		}
		if (vf_info->ndev_flags & IFF_ALLMULTI) {
			changed |= IFF_ALLMULTI;
			ndev_flags |= IFF_ALLMULTI;
		}
	} else {
		if (vf_info->ndev_flags & IFF_PROMISC)
			changed |= IFF_PROMISC;
		if (vf_info->ndev_flags & IFF_ALLMULTI)
			changed |= IFF_ALLMULTI;
	}

	if (changed)
		yk3_lan_set_ndev_flags(vf_info->qsetid, changed, ndev_flags,
				       pdev_priv->lan->hw_addr);
#ifdef ALL_MC_FEATURE
	if (changed & IFF_ALLMULTI)
		yk3_lan_ndev_set_allmulti(pdev_priv, vf_info->qsetid,
					  ndev_flags & IFF_ALLMULTI ? true : false,
					  ndev_flags);
#endif
	return 0;
}

int yk3_lan_ndev_set_vf_spoofchk(struct net_device *ndev, u16 vf_qset, bool enable)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	YK3_LAN_DBG_CNT_INC(vf_spoofchk);
	yk3_lan_set_vf_spoofchk(vf_qset, enable, pdev_priv->lan->hw_addr);

	return 0;
}

int yk3_lan_ndev_init(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_ndev_msg *ndev_task;
	struct yk3_lan_mbox_mac_msg *mac_task;
	struct yk3_lan_mbox_ack_msg *ack;
	u8 fields;
	int ret;

	if (yk3_pdev_is_mgr(pdev_priv))
		return 0;
	// announce unicast filter support
	ndev->priv_flags |= IFF_UNICAST_FLT;
	// init rxfh flow config
	fields = YK3_RXH_IP_SRC | YK3_RXH_IP_DST | YK3_RXH_L4_SRC |
		 YK3_RXH_L4_DST | YK3_RXH_L3_PROTO;
	yk3_set_rxfh_conf(ndev_priv, YK3_RXFH_TCP_V4_FLOW, fields);
	yk3_set_rxfh_conf(ndev_priv, YK3_RXFH_TCP_V6_FLOW, fields);
	yk3_set_rxfh_conf(ndev_priv, YK3_RXFH_UDP_V4_FLOW, fields);
	yk3_set_rxfh_conf(ndev_priv, YK3_RXFH_UDP_V6_FLOW, fields);
	// init ndev qset lan config
	if (yk3_pdev_is_pf(pdev_priv)) {
		pdev_priv->lan->ndev_priv = ndev_priv;
		yk3_lan_init_port(ndev_priv->qsetid, pdev_priv->pf_id, true,
				  pdev_priv->lan->hw_addr);
		// set ndev mac
		mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
		mac_task->qset = ndev_priv->qsetid;
		memcpy(mac_task->mac, ndev->dev_addr, ETH_ALEN);
		mac_task->mac_op = YK3_LAN_MBOX_MAC_UPD;

		mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
		mbox_msg.dst_id = yk3_mbox_master_id();

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("ndev init send mbox message errno %d failed!\n", ret);
			return -EFAULT;
		}
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		memset(&mbox_msg, 0, sizeof(mbox_msg));
		ndev_task = (struct yk3_lan_mbox_ndev_msg *)mbox_msg.data;
		ndev_task->enable = 1;

		mbox_msg.opcode = YK3_MBOX_OPCODE_INIT_NDEV;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 3000;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg);
		if (ret != 0) {
			yk3_dev_err("ndev send mbox message errno %d failed!\n", ret);
			return -EFAULT;
		}
		ack = (struct yk3_lan_mbox_ack_msg *)ack_msg.data;
		if (ack->ret_code != YK3_LAN_MBOX_RET_OK) {
			yk3_dev_err("ndev init failed err code %d failed!\n", ack->ret_code);
			return -EFAULT;
		}
		if (!is_valid_ether_addr(ack->mac)) {
			yk3_dev_err("vf init ndev get invalid mac addr!\n");
			yk3_dev_err("invalid mac addr: %pM\n", ack->mac);
			return -EINVAL;
		}
		eth_hw_addr_set(ndev, ack->mac);
		memcpy(ndev_priv->intf_mac, ack->mac, ETH_ALEN);
	}
	/* init priv ethtool flag */
	ndev_priv->ethtool_priv_flags |= BIT(YK3_ET_FFLAG_SRC_EQ_DST_DROP);
	ndev_priv->ethtool_priv_flags |= BIT(YK3_ET_PFLAG_RSS_SEL_UDP_TUN_OVERLAY);

	return 0;
}

void yk3_lan_ndev_exit(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_mac_msg *mac_task;
	int ret;

	// uninit ndev qset lan config
	if (yk3_pdev_is_pf(pdev_priv))
		yk3_lan_init_port(ndev_priv->qsetid, pdev_priv->pf_id,
				  false, pdev_priv->lan->hw_addr);
	yk3_lan_ndev_set_allmulti(pdev_priv, ndev_priv->qsetid, false, 0);
	// uninit ndev mac addr
	memset(&mbox_msg, 0, sizeof(mbox_msg));
	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	mac_task->qset = ndev_priv->qsetid;
	memcpy(mac_task->mac, ndev->dev_addr, ETH_ALEN);
	mac_task->mac_op = YK3_LAN_MBOX_UC_MAC_DEL;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
	mbox_msg.dst_id = yk3_mbox_master_id();

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0)
		yk3_dev_err("send del mac mbox message errno %d failed!\n", ret);
}

int yk3_lan_umd_set_pvid(struct yk3_ndev_priv *ndev_priv, u16 pvid, __be16 proto, u8 qos,
			 bool tx_only)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_lan_mbox_umd_pvid_msg *pvid_task;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	int ret;

	if (yk3_pdev_is_vf(pdev_priv)) {
		pvid_task = (struct yk3_lan_mbox_umd_pvid_msg *)mbox_msg.data;
		pvid_task->qset = ndev_priv->qsetid;
		pvid_task->pvid = pvid;
		pvid_task->proto = proto;
		pvid_task->qos = qos;
		pvid_task->tx_only = tx_only ? 1 : 0;
		mbox_msg.opcode = YK3_MBOX_OPCODE_UMD_SET_PVID;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send umd pvid mbox message qset %d errno %d failed!\n",
				    pvid_task->qset, ret);
			return -EFAULT;
		}
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		if (tx_only)
			yk3_lan_set_tx_vf_vlan(ndev_priv->qsetid, pvid, proto, qos,
					       pdev_priv->lan->hw_addr);
		else
			yk3_lan_set_vf_vlan(ndev_priv->qsetid, pvid, proto, qos,
					    pdev_priv->lan->hw_addr);
	}

	return 0;
}

int yk3_lan_umd_set_spoofchk(struct yk3_ndev_priv *ndev_priv, bool enable)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_lan_mbox_umd_spoofchk_msg *spoofchk_task;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	int ret;

	if (yk3_pdev_is_vf(pdev_priv)) {
		spoofchk_task = (struct yk3_lan_mbox_umd_spoofchk_msg *)mbox_msg.data;
		spoofchk_task->qset = ndev_priv->qsetid;
		spoofchk_task->enable = enable;

		mbox_msg.opcode = YK3_MBOX_OPCODE_UMD_SET_SPOOFCHK;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send umd spoofchk mbox message qset %d errno %d failed!\n",
				    spoofchk_task->qset, ret);
			return -EFAULT;
		}
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_lan_set_vf_spoofchk(ndev_priv->qsetid, enable, pdev_priv->lan->hw_addr);
	}

	return 0;
}

int yk3_lan_umd_restore_vf_conf(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	int ret;

	if (yk3_pdev_is_vf(pdev_priv)) {
		mbox_msg.opcode = YK3_MBOX_OPCODE_UMD_RESET_VF_CONF;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
		if (ret != 0) {
			yk3_dev_err("send umd reset vf config message qset %d failed errno %d!\n",
				    ndev_priv->qsetid, ret);
			return -EFAULT;
		}
	} else {
		yk3_dev_err("%s called by non-vf device!\n", __func__);
	}

	return 0;
}

static void yk3_lan_mbox_set_mac(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_mac_msg *mac_task;
	int ret = 0;

	mac_task = (struct yk3_lan_mbox_mac_msg *)msg->data;
	trace_yk3_lan_mbox_set_mac(mac_task->mac, mac_task->old_mac,
				   mac_task->qset, mac_task->mac_op);

	switch (mac_task->mac_op) {
	case YK3_LAN_MBOX_MAC_UPD:
		YK3_LAN_DBG_CNT_INC(intf_mac_upd);
		ret = yk3_esw_upd_uc_mac(pdev_priv, mac_task->old_mac, mac_task->mac,
					 mac_task->qset);
		if (ret)
			YK3_LAN_DBG_CNT_INC(intf_mac_upd_err);
		break;
	case YK3_LAN_MBOX_UC_MAC_ADD:
		YK3_LAN_DBG_CNT_INC(uc_mac_add);
		ret = yk3_esw_add_uc_mac(pdev_priv, mac_task->mac, mac_task->qset);
		if (ret)
			YK3_LAN_DBG_CNT_INC(uc_mac_add_err);
		break;
	case YK3_LAN_MBOX_UC_MAC_DEL:
		YK3_LAN_DBG_CNT_INC(uc_mac_del);
		ret = yk3_esw_del_uc_mac(pdev_priv, mac_task->mac, mac_task->qset);
		if (ret)
			YK3_LAN_DBG_CNT_INC(uc_mac_del_err);
		break;
	case YK3_LAN_MBOX_MC_MAC_ADD:
		YK3_LAN_DBG_CNT_INC(mc_mac_add);
		ret = yk3_esw_add_mc_mac(pdev_priv, mac_task->mac, mac_task->qset);
		if (ret)
			YK3_LAN_DBG_CNT_INC(mc_mac_add_err);
		break;
	case YK3_LAN_MBOX_MC_MAC_DEL:
		YK3_LAN_DBG_CNT_INC(mc_mac_del);
		ret = yk3_esw_del_mc_mac(pdev_priv, mac_task->mac, mac_task->qset);
		if (ret)
			YK3_LAN_DBG_CNT_INC(mc_mac_del_err);
		break;
	default:
		yk3_dev_err("mgr pf recv invalid mac msg mac_op 0x%04x\n", mac_task->mac_op);
		break;
	}

	if (ret) {
		yk3_dev_err("qset: %d, mac opcode %d, esw return %x failed!\n",
			    mac_task->qset, mac_task->mac_op, ret);
	}
}

static void yk3_lan_mbox_set_vlan(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_vlan_msg *vlan_task;
	int ret;

	vlan_task = (struct yk3_lan_mbox_vlan_msg *)msg->data;
	trace_yk3_lan_mbox_set_vlan(vlan_task->vlan_id, be16_to_cpu(vlan_task->proto),
				    vlan_task->qset, vlan_task->vlan_op);

	if (vlan_task->vlan_op == YK3_LAN_MBOX_VLAN_ADD) {
		YK3_LAN_DBG_CNT_INC(vlan_add);
		ret = yk3_esw_add_vlan(pdev_priv, vlan_task->vlan_id, vlan_task->proto,
				       vlan_task->qset);
	} else {
		YK3_LAN_DBG_CNT_INC(vlan_del);
		ret = yk3_esw_del_vlan(pdev_priv, vlan_task->vlan_id, vlan_task->proto,
				       vlan_task->qset);
	}
	if (ret != 0) {
		yk3_dev_err("qset: %d, vlan id: %04x, tpid: %04x, esw return %x failed!\n",
			    vlan_task->qset, vlan_task->vlan_id,
			    vlan_task->proto, ret);
		if (vlan_task->vlan_op == YK3_LAN_MBOX_VLAN_ADD)
			YK3_LAN_DBG_CNT_INC(vlan_add_err);
		else
			YK3_LAN_DBG_CNT_INC(vlan_del_err);
	}
}

#ifdef ALL_MC_FEATURE
static void yk3_lan_set_umc(void __iomem *hw_base, u16 qset, bool enable, u32 flags)
{
	u32 reg;

	reg = yk3_rd32(hw_base, YK3_LAN_RX_QSET_VLAN_MISC(qset));
	reg &= ~FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, 1);
	if (enable) {
		reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, 1);
	} else {
		if (flags & IFF_PROMISC)
			reg |= FIELD_PREP(YK3_LAN_RX_QSET_VLAN_MISC_RECV_UMC, 1);
	}
	yk3_wr32(hw_base, YK3_LAN_RX_QSET_VLAN_MISC(qset), reg);
}

static void yk3_lan_mbox_set_all_mc(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_set_all_mc_msg *all_mc_task;
	int ret;

	YK3_LAN_DBG_CNT_INC(all_mc_set);
	all_mc_task = (struct yk3_lan_mbox_set_all_mc_msg *)msg->data;
	yk3_lan_set_umc(pdev_priv->lan->hw_addr, all_mc_task->qset, all_mc_task->enable,
			all_mc_task->flags);

	ret = yk3_esw_set_all_mc(pdev_priv, all_mc_task->qset, all_mc_task->enable);
	if (ret != 0) {
		yk3_dev_err("all mc qset: %d, enable: %d, esw return %x failed!\n",
			    all_mc_task->qset, all_mc_task->enable, ret);
		YK3_LAN_DBG_CNT_INC(all_mc_set_err);
	}
}
#endif

static void yk3_lan_mbox_init_ndev(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_mac_msg *mac_task;
	struct yk3_lan_mbox_ack_msg *ack;
	struct yk3_vf_info *vf_info;
	u16 vf_id;
	struct yk3_sriov_priv *sriov_priv;
	u32 ret_code = YK3_LAN_MBOX_RET_OK;
	int ret;

	YK3_LAN_DBG_CNT_INC(vf_ndev_init);
	vf_id = yk3_mbox_get_src_vf_id(msg->src_id);
	if (vf_id == YK3_MBOX_SRC_ID_NULL) {
		yk3_dev_err("recv init vf ndev msg from invalid vf src id %04x!\n",
			    msg->src_id);
		ret_code = YK3_LAN_MBOX_INVALID_VF;
		YK3_LAN_DBG_CNT_INC(vf_ndev_init_err);
		goto send_ack;
	}

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (!sriov_priv) {
		yk3_dev_err("recv init ndev msg get sriov priv failed, vf src id %04x!\n",
			    msg->src_id);
		ret_code = YK3_LAN_MBOX_INVALID_VF;
		YK3_LAN_DBG_CNT_INC(vf_ndev_init_err);
		goto send_ack;
	}
	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf_id);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_dev_err("recv init ndev msg get vf info failed, vf src id %04x!\n",
			    msg->src_id);
		yk3_sriov_put_priv(sriov_priv);
		ret_code = YK3_LAN_MBOX_INVALID_VF;
		YK3_LAN_DBG_CNT_INC(vf_ndev_init_err);
		goto send_ack;
	}

	yk3_lan_reset_ndev_config(vf_info->qsetid, pdev_priv->lan->hw_addr);
	// send vf mac message to mgr pf
	mac_task = (struct yk3_lan_mbox_mac_msg *)mbox_msg.data;
	mac_task->qset = vf_info->qsetid;
	memcpy(mac_task->mac, vf_info->mac_addr, ETH_ALEN);
	mac_task->mac_op = YK3_LAN_MBOX_MAC_UPD;

	mbox_msg.opcode = YK3_MBOX_OPCODE_SET_MAC;
	mbox_msg.dst_id = yk3_mbox_master_id();
	YK3_LAN_DBG_CNT_INC(vf_ndev_init_mac);
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("%s send mbox message errno %d failed!\n", __func__, ret);
		ret_code = YK3_LAN_MBOX_MBOX_ERR;
		yk3_sriov_put_priv(sriov_priv);
		YK3_LAN_DBG_CNT_INC(vf_ndev_init_mac_err);
		goto send_ack;
	}
	yk3_sriov_put_priv(sriov_priv);
send_ack:
	memset(&mbox_msg, 0, sizeof(mbox_msg));
	memset(&mbox_opt, 0, sizeof(mbox_opt));
	ack = (struct yk3_lan_mbox_ack_msg *)mbox_msg.data;
	ack->ret_code = ret_code;
	if (ret_code == 0)
		memcpy(ack->mac, vf_info->mac_addr, ETH_ALEN);
	mbox_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_msg.dst_id = msg->src_id;
	mbox_msg.seqno = msg->seqno;
	mbox_opt.wait_reply = MB_NO_REPLY;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0) {
		yk3_dev_err("send ack mbox message errno %d failed!\n", ret);
		if (ret_code == 0)
			YK3_LAN_DBG_CNT_INC(vf_ndev_init_err);
	}
}

static void yk3_lan_mbox_set_mtu(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_mtu_msg *mtu_task;

	YK3_LAN_DBG_CNT_INC(vf_mtu);
	mtu_task = (struct yk3_lan_mbox_mtu_msg *)msg->data;
	yk3_lan_set_mtu(mtu_task->qset, mtu_task->mtu, pdev_priv->lan->hw_addr);
}

static void yk3_lan_mbox_set_rss(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_rss_msg *rss_task;

	YK3_LAN_DBG_CNT_INC(vf_ndev_rss);
	rss_task = (struct yk3_lan_mbox_rss_msg *)msg->data;
	yk3_lan_set_rss_hash_fields(rss_task->qset, rss_task->flow_type,
				    rss_task->fields, pdev_priv->lan->hw_addr);
}

static void yk3_lan_mbox_set_ndev_feature(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_ndev_feature_msg *feature_task;

	YK3_LAN_DBG_CNT_INC(vf_ndev_feature);
	feature_task = (struct yk3_lan_mbox_ndev_feature_msg *)msg->data;
	yk3_lan_set_feature(feature_task->qset, feature_task->changed,
			    feature_task->features, pdev_priv->lan->hw_addr);

	if (feature_task->changed & NETIF_F_RXHASH) {
		yk3_lan_set_rxhash(feature_task->qset, feature_task->rss_fields,
				   pdev_priv->lan->hw_addr);
	}
}

static void yk3_lan_pf_mbox_vf_mac_task(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_mac_msg *mac_task;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_lan_mbox_ack_msg *ack;
	struct yk3_vf_info *vf_info;
	struct yk3_sriov_priv *sriov_priv;
	u16 vf_id;
	u32 ret_code = YK3_LAN_MBOX_RET_OK;
	int ret;

	YK3_LAN_DBG_CNT_INC(vf_mac);
	mac_task = (struct yk3_lan_mbox_mac_msg *)msg->data;
	trace_yk3_lan_mbox_vf_mac(mac_task->mac, mac_task->qset);

	vf_id = yk3_mbox_get_src_vf_id(msg->src_id);
	if (vf_id == YK3_MBOX_SRC_ID_NULL) {
		yk3_dev_err("recv set mac msg from invalid vf src id %04x!\n",
			    msg->src_id);
		ret_code = YK3_LAN_MBOX_MBOX_ERR;
		YK3_LAN_DBG_CNT_INC(vf_mac_err);
		goto upd_err;
	}

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (!sriov_priv) {
		yk3_dev_err("recv set mac msg get sriov priv failed, VF src id %04x!\n",
			    msg->src_id);
		ret_code = YK3_LAN_MBOX_INVALID_VF;
		YK3_LAN_DBG_CNT_INC(vf_mac_err);
		goto upd_err;
	}

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf_id);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_dev_err("recv set mac msg get vf info failed, VF src id %04x!\n",
			    msg->src_id);
		yk3_sriov_put_priv(sriov_priv);
		ret_code = YK3_LAN_MBOX_INVALID_VF;
		YK3_LAN_DBG_CNT_INC(vf_mac_err);
		goto upd_err;
	}

	if (IS_ERR_OR_NULL(pdev_priv->lan->ndev_priv)) {
		yk3_dev_err("pf get ndev priv failed!\n");
		yk3_sriov_put_priv(sriov_priv);
		ret_code = YK3_LAN_MBOX_RET_ERR;
		YK3_LAN_DBG_CNT_INC(vf_mac_err);
		goto upd_err;
	}

	/* check mac duplicate with pf/vf mac */
	if (yk3_check_dup_vf_mac(pdev_priv->lan->ndev_priv, mac_task->mac,
				 vf_info->vf_vlan, vf_info->vf_vlan_tpid) == -EEXIST) {
		yk3_dev_err("recv vf id %04x set mac but address duplicate with other PF/VF!\n",
			    msg->src_id);
		yk3_sriov_put_priv(sriov_priv);
		ret_code = YK3_LAN_MBOX_RET_DUP_MAC;
		YK3_LAN_DBG_CNT_INC(vf_mac_err);
		goto upd_err;
	}

	memcpy(vf_info->mac_addr, mac_task->mac, ETH_ALEN);
	yk3_sriov_put_priv(sriov_priv);
upd_err:
	ack = (struct yk3_lan_mbox_ack_msg *)mbox_msg.data;
	ack->ret_code = ret_code;
	mbox_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	mbox_msg.seqno = msg->seqno;
	mbox_msg.dst_id = msg->src_id;
	mbox_opt.wait_reply = MB_NO_REPLY;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	if (ret != 0)
		yk3_err("%s send ack mbox message errno %d failed!\n", __func__, ret);
}

static void yk3_lan_vf_mbox_mac_task(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_ndev_priv *ndev_priv = NULL;
	struct net_device *ndev;
	struct yk3_lan_mbox_mac_msg *mac_task;

	YK3_LAN_DBG_CNT_INC(pf_set_vf_mac);
	mac_task = (struct yk3_lan_mbox_mac_msg *)msg->data;
	ndev_priv = yk3_pdev_get_ndev_priv(pdev_priv, YK3_NDEV_T_VF, -1);
	if (IS_ERR_OR_NULL(ndev_priv)) {
		yk3_dev_err("vf get ndev priv failed!\n");
		YK3_LAN_DBG_CNT_INC(pf_set_vf_mac_err);
		return;
	}
	ndev = ndev_priv->ndev;
	memcpy((u8 *)ndev->dev_addr, mac_task->mac, ETH_ALEN);
}

static void yk3_lan_mbox_set_ndev_flags(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_ndev_flags_msg *flags_task;
	struct yk3_vf_info *vf_info;
	struct yk3_sriov_priv *sriov_priv;
	u16 vf_id;

	YK3_LAN_DBG_CNT_INC(vf_ndev_flags);
	flags_task = (struct yk3_lan_mbox_ndev_flags_msg *)msg->data;
	vf_id = yk3_mbox_get_src_vf_id(msg->src_id);
	if (vf_id == YK3_MBOX_SRC_ID_NULL) {
		yk3_dev_err("recv set ndev flags msg from invalid vf src id %04x!\n",
			    msg->src_id);
		YK3_LAN_DBG_CNT_INC(vf_ndev_flags_err);
		return;
	}

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv)) {
		yk3_dev_err("recv set ndev flag msg get sriov priv failed, VF src id %04x!\n",
			    msg->src_id);
		YK3_LAN_DBG_CNT_INC(vf_ndev_flags_err);
		return;
	}
	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf_id);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_dev_err("recv set ndev flag msg get vf info failed, VF src id %04x!\n",
			    msg->src_id);
		YK3_LAN_DBG_CNT_INC(vf_ndev_flags_err);
		yk3_sriov_put_priv(sriov_priv);
		return;
	}

	vf_info->ndev_flags = flags_task->flags;
	if (vf_info->trusted)
		yk3_lan_set_ndev_flags(vf_info->qsetid, flags_task->changed,
				       flags_task->flags, pdev_priv->lan->hw_addr);
#ifdef ALL_MC_FEATURE
	if (vf_info->trusted &&
	    (flags_task->changed & IFF_ALLMULTI || flags_task->flags & IFF_ALLMULTI))
		yk3_lan_ndev_set_allmulti(pdev_priv, vf_info->qsetid,
					  flags_task->flags & IFF_ALLMULTI ? true : false,
					  flags_task->flags);
#endif
	yk3_sriov_put_priv(sriov_priv);
}

static void yk3_lan_mbox_set_ethtool_priv_flags(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_ethtool_priv_flags_msg *priv_flag_task;
	u8 enable;

	YK3_LAN_DBG_CNT_INC(vf_priv_flags);
	priv_flag_task = (struct yk3_lan_mbox_ethtool_priv_flags_msg *)msg->data;
	if (priv_flag_task->enable)
		enable = 1;
	else
		enable = 0;

	yk3_lan_set_ethtool_priv_flags(priv_flag_task->qset, priv_flag_task->flags,
				       enable, pdev_priv->lan->hw_addr);
}

static void yk3_lan_mbox_umd_set_pvid(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_umd_pvid_msg *pvid_task;

	pvid_task = (struct yk3_lan_mbox_umd_pvid_msg *)msg->data;
	yk3_dev_debug("recv umd pvid msg:\n");
	yk3_dev_debug("pvid: %04x, tpid: %04x, qos: %02x, qset: %d, tx_only: %d\n",
		      pvid_task->pvid, pvid_task->proto,
		      pvid_task->qos, pvid_task->qset, pvid_task->tx_only);

	if (pvid_task->tx_only)
		yk3_lan_set_tx_vf_vlan(pvid_task->qset, pvid_task->pvid, pvid_task->proto,
				       pvid_task->qos, pdev_priv->lan->hw_addr);
	else
		yk3_lan_set_vf_vlan(pvid_task->qset, pvid_task->pvid, pvid_task->proto,
				    pvid_task->qos, pdev_priv->lan->hw_addr);
}

static void yk3_lan_mbox_umd_set_spoofchk(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_lan_mbox_umd_spoofchk_msg *spoofchk_task;

	spoofchk_task = (struct yk3_lan_mbox_umd_spoofchk_msg *)msg->data;
	yk3_dev_debug("pf recv umd spoofchk msg:\n");
	yk3_dev_debug("enable: %d, qset: %d\n", spoofchk_task->enable, spoofchk_task->qset);
	yk3_lan_set_vf_spoofchk(spoofchk_task->qset, spoofchk_task->enable,
				pdev_priv->lan->hw_addr);
}

static void yk3_lan_mbox_umd_reset_vf_conf(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = (struct yk3_pdev_priv *)param;
	struct yk3_sriov_priv *sriov_priv;
	struct yk3_vf_info *vf_info;
	u16 vf_id;

	vf_id = yk3_mbox_get_src_vf_id(msg->src_id);
	if (vf_id == YK3_MBOX_SRC_ID_NULL) {
		yk3_dev_err("recv reset vf conf msg from invalid vf src id %04x!\n",
			    msg->src_id);
		return;
	}

	sriov_priv = yk3_sriov_get_priv(pdev_priv);
	if (IS_ERR_OR_NULL(sriov_priv)) {
		yk3_dev_err("recv reset vf conf msg get sriov priv failed, VF src id %04x!\n",
			    msg->src_id);
		return;
	}

	vf_info = yk3_sriov_get_vfinfo(sriov_priv, vf_id);
	if (IS_ERR_OR_NULL(vf_info)) {
		yk3_dev_err("recv reset vf conf msg get vf info failed, VF src id %04x!\n",
			    msg->src_id);
		yk3_sriov_put_priv(sriov_priv);
		return;
	}

	yk3_dev_debug("recv umd reset vf conf msg:\n");
	yk3_dev_debug("vf_id: %d\n", vf_id);
	yk3_lan_set_vf_vlan(vf_info->qsetid, vf_info->vf_vlan, vf_info->vf_vlan_tpid,
			    vf_info->vf_vlan_qos, pdev_priv->lan->hw_addr);
	yk3_lan_set_vf_spoofchk(vf_info->qsetid, vf_info->spoofchk ? true : false,
				pdev_priv->lan->hw_addr);
	yk3_sriov_put_priv(sriov_priv);
}

static struct yk3_lan_mbox_opcode_cb yk3_lan_mgr_pf_opcodes[] = {
	{YK3_MBOX_OPCODE_SET_MAC, yk3_lan_mbox_set_mac},
	{YK3_MBOX_OPCODE_SET_VLAN, yk3_lan_mbox_set_vlan},
#ifdef ALL_MC_FEATURE
	{YK3_MBOX_OPCODE_SET_ALL_MC, yk3_lan_mbox_set_all_mc},
#endif
};

static struct yk3_lan_mbox_opcode_cb yk3_lan_pf_opcodes[] = {
	{YK3_MBOX_OPCODE_UPD_VF_MAC, yk3_lan_pf_mbox_vf_mac_task},
	{YK3_MBOX_OPCODE_INIT_NDEV, yk3_lan_mbox_init_ndev},
	{YK3_MBOX_OPCODE_SET_MTU, yk3_lan_mbox_set_mtu},
	{YK3_MBOX_OPCODE_SET_RSS_HASH, yk3_lan_mbox_set_rss},
	{YK3_MBOX_OPCODE_SET_NDEV_FLAGS, yk3_lan_mbox_set_ndev_flags},
	{YK3_MBOX_OPCODE_SET_NDEV_FEATURES, yk3_lan_mbox_set_ndev_feature},
	{YK3_MBOX_OPCODE_SET_ETHTOOL_PRIV, yk3_lan_mbox_set_ethtool_priv_flags},
	{YK3_MBOX_OPCODE_UMD_SET_PVID, yk3_lan_mbox_umd_set_pvid},
	{YK3_MBOX_OPCODE_UMD_SET_SPOOFCHK, yk3_lan_mbox_umd_set_spoofchk},
	{YK3_MBOX_OPCODE_UMD_RESET_VF_CONF, yk3_lan_mbox_umd_reset_vf_conf},
};

static struct yk3_lan_mbox_opcode_cb yk3_lan_vf_opcodes[] = {
	{YK3_MBOX_OPCODE_SET_VF_MAC, yk3_lan_vf_mbox_mac_task},
};

static int yk3_lan_init_mbox(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	struct yk3_lan_mbox_opcode_cb *opcode_cb;
	int i, opcode_num;
	int ret;

	if (IS_ERR_OR_NULL(mbox))
		return -EINVAL;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_mgr_pf_opcodes);
		opcode_cb = yk3_lan_mgr_pf_opcodes;
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_pf_opcodes);
		opcode_cb = yk3_lan_pf_opcodes;
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_vf_opcodes);
		opcode_cb = yk3_lan_vf_opcodes;
	} else {
		yk3_dev_err("%s meet invalid device id %08x\n", __func__, pdev_priv->device);
		return -EINVAL;
	}

	for (i = 0; i < opcode_num; i++) {
		ret = yk3_mbox_register_callback(pdev_priv,
						 opcode_cb[i].opcode,
						 opcode_cb[i].cb,
						 (void *)pdev_priv);
		if (ret) {
			yk3_dev_err("%s register mbox opcode 0x%04x failed errno %d\n",
				    __func__, opcode_cb[i].opcode, ret);
			for (i = i - 1; i >= 0; i--)
				yk3_mbox_unregister_callback(pdev_priv, opcode_cb[i].opcode);
			return -EFAULT;
		}
	}

	return 0;
}

static int yk3_lan_uninit_mbox(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	struct yk3_lan_mbox_opcode_cb *opcode_cb;
	int i, opcode_num;

	if (IS_ERR_OR_NULL(mbox))
		return -EINVAL;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_mgr_pf_opcodes);
		opcode_cb = yk3_lan_mgr_pf_opcodes;
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_pf_opcodes);
		opcode_cb = yk3_lan_pf_opcodes;
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		opcode_num = ARRAY_SIZE(yk3_lan_vf_opcodes);
		opcode_cb = yk3_lan_vf_opcodes;
	} else {
		yk3_dev_err("%s meet invalid device id %08x\n", __func__, pdev_priv->device);
		return -EINVAL;
	}

	for (i = 0; i < opcode_num; i++)
		yk3_mbox_unregister_callback(pdev_priv, opcode_cb[i].opcode);

	return 0;
}

void yk3_lan_sriov_init(struct yk3_pdev_priv *pdev_priv, bool enable)
{
	struct yk3_sriov_priv *sriov = pdev_priv->sriov_priv;
	struct yk3_vf_info *vf_info;
	int i;

	if (IS_ERR_OR_NULL(sriov))
		return;

	for (i = 0; i < sriov->num_vfs; i++) {
		vf_info = &sriov->vf_info[i];
		yk3_lan_init_port(vf_info->qsetid, pdev_priv->pf_id, enable,
				  pdev_priv->lan->hw_addr);
		yk3_lan_set_evb_mode(vf_info->qsetid, pdev_priv->evb_mode,
				     pdev_priv->lan->hw_addr);
	}
}

static int yk3_dbgfs_lan_counter_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_lan *lan = pdev_priv->lan;

	if (!lan) {
		seq_puts(seq, "\tno lan info\n");
		return 0;
	}

	seq_puts(seq, "lan debug counter table:\n");
	if (yk3_pdev_is_mgr(pdev_priv)) {
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac upd",
			   lan->dbg_cnt.cnt_intf_mac_upd);
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac upd err",
			   lan->dbg_cnt.cnt_intf_mac_upd_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac add",
			   lan->dbg_cnt.cnt_uc_mac_add);
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac add err",
			   lan->dbg_cnt.cnt_uc_mac_add_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac del",
			   lan->dbg_cnt.cnt_uc_mac_del);
		seq_printf(seq, "\t%-24s : %-8d\n", "uc mac del err",
			   lan->dbg_cnt.cnt_uc_mac_del_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "mc mac add",
			   lan->dbg_cnt.cnt_mc_mac_add);
		seq_printf(seq, "\t%-24s : %-8d\n", "mc mac add err",
			   lan->dbg_cnt.cnt_mc_mac_add_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "mc mac del",
			   lan->dbg_cnt.cnt_mc_mac_del);
		seq_printf(seq, "\t%-24s : %-8d\n", "mc mac del err",
			   lan->dbg_cnt.cnt_mc_mac_del_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vlan add",
			   lan->dbg_cnt.cnt_vlan_add);
		seq_printf(seq, "\t%-24s : %-8d\n", "vlan add err",
			   lan->dbg_cnt.cnt_vlan_add_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vlan del",
			   lan->dbg_cnt.cnt_vlan_del);
		seq_printf(seq, "\t%-24s : %-8d\n", "vlan del err",
			   lan->dbg_cnt.cnt_vlan_del_err);
#ifdef ALL_MC_FEATURE
		seq_printf(seq, "\t%-24s : %-8d\n", "all mc set",
			   lan->dbg_cnt.cnt_all_mc_set);
		seq_printf(seq, "\t%-24s : %-8d\n", "all mc set err",
			   lan->dbg_cnt.cnt_all_mc_set_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "all mc unset",
			   lan->dbg_cnt.cnt_all_mc_unset);
		seq_printf(seq, "\t%-24s : %-8d\n", "all mc unset err",
			   lan->dbg_cnt.cnt_all_mc_unset_err);
#endif
		return 0;
	}

	seq_printf(seq, "\t%-24s : %-8d\n", "netdev intf mac upd",
		   lan->dbg_cnt.cnt_intf_mac_upd);
	seq_printf(seq, "\t%-24s : %-8d\n", "netdev intf mac upd err",
		   lan->dbg_cnt.cnt_intf_mac_upd_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc mac add",
		   lan->dbg_cnt.cnt_uc_mac_add);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc mac add err",
		   lan->dbg_cnt.cnt_uc_mac_add_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc mac del",
		   lan->dbg_cnt.cnt_uc_mac_del);
	seq_printf(seq, "\t%-24s : %-8d\n", "uc mac del err",
		   lan->dbg_cnt.cnt_uc_mac_del_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc mac add",
		   lan->dbg_cnt.cnt_mc_mac_add);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc mac add err",
		   lan->dbg_cnt.cnt_mc_mac_add_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc mac del",
		   lan->dbg_cnt.cnt_mc_mac_del);
	seq_printf(seq, "\t%-24s : %-8d\n", "mc mac del err",
		   lan->dbg_cnt.cnt_mc_mac_del_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "vlan add",
		   lan->dbg_cnt.cnt_vlan_add);
	seq_printf(seq, "\t%-24s : %-8d\n", "vlan add err",
		   lan->dbg_cnt.cnt_vlan_add_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "vlan del",
		   lan->dbg_cnt.cnt_vlan_del);
	seq_printf(seq, "\t%-24s : %-8d\n", "vlan del err",
		   lan->dbg_cnt.cnt_vlan_del_err);
#ifdef ALL_MC_FEATURE
	seq_printf(seq, "\t%-24s : %-8d\n", "all mc set",
		   lan->dbg_cnt.cnt_all_mc_set);
	seq_printf(seq, "\t%-24s : %-8d\n", "all mc set err",
		   lan->dbg_cnt.cnt_all_mc_set_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "all mc unset",
		   lan->dbg_cnt.cnt_all_mc_unset);
	seq_printf(seq, "\t%-24s : %-8d\n", "all mc unset err",
		   lan->dbg_cnt.cnt_all_mc_unset_err);
#endif
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set flags",
		   lan->dbg_cnt.cnt_ndev_set_flags);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set flags err",
		   lan->dbg_cnt.cnt_ndev_set_flags_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev ethtool priv flags",
		   lan->dbg_cnt.cnt_ndev_set_priv_flags);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev ethtool priv flags err",
		   lan->dbg_cnt.cnt_ndev_set_priv_flags_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set rss",
		   lan->dbg_cnt.cnt_ndev_set_rss);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set rss err",
		   lan->dbg_cnt.cnt_ndev_set_rss_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set feature",
		   lan->dbg_cnt.cnt_ndev_set_feature);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set feature err",
		   lan->dbg_cnt.cnt_ndev_set_feature_err);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set mtu",
		   lan->dbg_cnt.cnt_ndev_set_mtu);
	seq_printf(seq, "\t%-24s : %-8d\n", "ndev set mtu err",
		   lan->dbg_cnt.cnt_ndev_set_mtu_err);

	if (yk3_pdev_is_pf(pdev_priv)) {
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev init",
			   lan->dbg_cnt.cnt_vf_ndev_init);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev init err",
			   lan->dbg_cnt.cnt_vf_ndev_init_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev init mac upd",
			   lan->dbg_cnt.cnt_vf_ndev_init_mac);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev init mac upd err",
			   lan->dbg_cnt.cnt_vf_ndev_init_mac_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf mac upd",
			   lan->dbg_cnt.cnt_vf_mac);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf mac upd err",
			   lan->dbg_cnt.cnt_vf_mac_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "pf set vf mac",
			   lan->dbg_cnt.cnt_vf_set_mac);
		seq_printf(seq, "\t%-24s : %-8d\n", "pf set vf mac err",
			   lan->dbg_cnt.cnt_vf_set_mac_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf vlan set",
			   lan->dbg_cnt.cnt_vf_vlan);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf vlan set err",
			   lan->dbg_cnt.cnt_vf_vlan_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf spoofchk",
			   lan->dbg_cnt.cnt_vf_spoofchk);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf spoofchk err",
			   lan->dbg_cnt.cnt_vf_spoofchk_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf trust",
			   lan->dbg_cnt.cnt_vf_trust);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf trust err",
			   lan->dbg_cnt.cnt_vf_trust_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev feature",
			   lan->dbg_cnt.cnt_vf_ndev_feature);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev feature err",
			   lan->dbg_cnt.cnt_vf_ndev_feature_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev rss",
			   lan->dbg_cnt.cnt_vf_ndev_rss);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ndev rss err",
			   lan->dbg_cnt.cnt_vf_ndev_rss_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ethtool priv flags",
			   lan->dbg_cnt.cnt_vf_priv_flags);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf ethtool priv flags err",
			   lan->dbg_cnt.cnt_vf_priv_flags_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf mtu",
			   lan->dbg_cnt.cnt_vf_mtu);
		seq_printf(seq, "\t%-24s : %-8d\n", "vf mtu err",
			   lan->dbg_cnt.cnt_vf_mtu_err);
		seq_printf(seq, "\t%-24s : %-8d\n", "set evb mode",
			   lan->dbg_cnt.cnt_set_evb_mode);
		seq_printf(seq, "\t%-24s : %-8d\n", "set evb mode err",
			   lan->dbg_cnt.cnt_set_evb_mode_err);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		seq_printf(seq, "\t%-24s : %-8d\n", "pf set vf mac",
			   lan->dbg_cnt.cnt_pf_set_vf_mac);
		seq_printf(seq, "\t%-24s : %-8d\n", "pf set vf mac err",
			   lan->dbg_cnt.cnt_pf_set_vf_mac_err);
	}

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(yk3_dbgfs_lan_counter);

int yk3_lan_init(struct yk3_pdev_priv *pdev_priv)
{
	int ret = 0;

	pdev_priv->lan = kzalloc(sizeof(*pdev_priv->lan), GFP_KERNEL);
	if (!pdev_priv->lan)
		return -ENOMEM;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		pdev_priv->lan->hw_addr = pdev_priv->bar_addr[YK3_BAR0];
		yk3_lan_hw_reset(pdev_priv->lan->hw_addr);

		yk3_lan_init_hw(pdev_priv->lan->hw_addr);

		ret = yk3_init_esw(pdev_priv);
		if (ret != 0) {
			kfree(pdev_priv->lan);
			yk3_dev_err("yk3_init_esw() failed\n");
			return ret;
		}

		ret = yk3_lan_init_mbox(pdev_priv);
		if (ret != 0) {
			ret = yk3_uninit_esw(pdev_priv);
			if (ret != 0)
				yk3_dev_err("yk3_uninit_esw() failed\n");
			kfree(pdev_priv->lan);
			yk3_dev_err("yk3_lan_init_mbox() failed errno %d\n", ret);
			return ret;
		}
	} else if (yk3_pdev_is_pf(pdev_priv) || yk3_pdev_is_vf(pdev_priv)) {
		pdev_priv->lan->hw_addr = pdev_priv->bar_addr[YK3_BAR0];
		ret = yk3_lan_init_mbox(pdev_priv);
		if (ret != 0) {
			kfree(pdev_priv->lan);
			yk3_dev_err("yk3_lan_init_mbox() failed errno %d\n", ret);
			return ret;
		}
	} else {
		kfree(pdev_priv->lan);
		yk3_dev_err("yk3 lan meet unknown device %08x\n", pdev_priv->device);
		return -ENODEV;
	}
	pdev_priv->lan->dbgfs_lan_counter =
		debugfs_create_file("lan_counter", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_dbgfs_lan_counter_fops);
	if (!pdev_priv->lan->dbgfs_lan_counter)
		yk3_dev_err("failed to create debugfs lan counter file");
	yk3_dev_debug("%s ok!\n", __func__);

	return ret;
}

void yk3_lan_exit(struct yk3_pdev_priv *pdev_priv)
{
	int ret;

	ret = yk3_lan_uninit_mbox(pdev_priv);
	if (ret != 0)
		yk3_dev_err("yk3_lan_uninit_mbox() failed errno %d\n", ret);

	if (yk3_pdev_is_mgr(pdev_priv)) {
		ret = yk3_uninit_esw(pdev_priv);
		if (ret != 0)
			yk3_dev_err("yk3_uninit_esw() failed\n");
	}

	kfree(pdev_priv->lan);
}
