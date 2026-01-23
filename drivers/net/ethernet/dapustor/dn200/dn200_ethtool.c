// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 */

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/interrupt.h>
#include <linux/mii.h>
#include <linux/net_tstamp.h>
#include <linux/io.h>
#include "dn200.h"
#include "dwxgmac_comm.h"
#include "dn200_phy.h"
#include "dn200_eprom.h"

#define DN200_PRIV_FLAGS_PTP_RXTX	BIT(0)
#define DN200_PRIV_FLAGS_FEC_EN		BIT(1)
struct dn200_priv_flags {
	char flag_string[ETH_GSTRING_LEN];
	u64 flag;
	bool read_only;
};

#define DN200_PRIV_FLAG(_name, _flag, _read_only) { \
	.flag_string = _name, \
	.flag = _flag, \
	.read_only = _read_only, \
}
static const struct dn200_priv_flags dn200_gstrings_priv_flags[] = {
	DN200_PRIV_FLAG("ptp-rxtx", DN200_PRIV_FLAGS_PTP_RXTX, 0),
	DN200_PRIV_FLAG("fec-en", DN200_PRIV_FLAGS_FEC_EN, 0),
};

#define DN200_PRIV_FLAGS_STR_LEN ARRAY_SIZE(dn200_gstrings_priv_flags)

#define REG_SPACE_SIZE	0x1060
#define GMAC4_REG_SPACE_SIZE	0x116C
#define GEN_PHY_REG_SPACE_SIZE	0x100
#define MAC100_ETHTOOL_NAME	"st_mac100"
#define GMAC_ETHTOOL_NAME	"st_gmac"
#define XGMAC_ETHTOOL_NAME	"dn200"

/* Same as DMA_CHAN_BASE_ADDR defined in dwmac4_dma.h
 *
 * It is here because dwmac_dma.h and dwmac4_dam.h can not be included at the
 * same time due to the conflicting macro names.
 */
#define GMAC4_DMA_CHAN_BASE_ADDR  0x00001100

#define ETHTOOL_DMA_OFFSET	55

#define MMC_DISPLAY_FLAG 1
struct dn200_stats {
	char stat_string[ETH_GSTRING_LEN];
	int sizeof_stat;
	int stat_offset;
};

#define DN200_STAT(name, m)	\
	{ name, sizeof_field(struct dn200_extra_stats, m),	\
	offsetof(struct dn200_priv, xstats.m)}

static const struct dn200_stats dn200_gstrings_stats[] = {
	DN200_STAT("rx_csum_err", rx_csum_err),
	/* Tx/Rx IRQ error info */
	DN200_STAT("tx_process_stopped_irq", tx_process_stopped_irq),
	DN200_STAT("rx_buf_unav_irq", rx_buf_unav_irq),
	DN200_STAT("fatal_bus_error_irq", fatal_bus_error_irq),

	/* Tx/Rx IRQ Events */
	DN200_STAT("tx_pkt_n", tx_pkt_n),
	DN200_STAT("rx_pkt_n", rx_pkt_n),
	DN200_STAT("normal_irq_n", normal_irq_n),
	DN200_STAT("rx_normal_irq_n", rx_normal_irq_n),
	DN200_STAT("napi_poll", napi_poll),
	DN200_STAT("tx_normal_irq_n", tx_normal_irq_n),
	DN200_STAT("tx_clean", tx_clean),
	DN200_STAT("tx_set_ic_bit", tx_set_ic_bit),
	DN200_STAT("irq_receive_pmt_irq_n", irq_receive_pmt_irq_n),

	/* EEE */
	DN200_STAT("irq_tx_path_in_lpi_mode_n", irq_tx_path_in_lpi_mode_n),
	DN200_STAT("irq_tx_path_exit_lpi_mode_n", irq_tx_path_exit_lpi_mode_n),
	DN200_STAT("irq_rx_path_in_lpi_mode_n", irq_rx_path_in_lpi_mode_n),
	DN200_STAT("irq_rx_path_exit_lpi_mode_n", irq_rx_path_exit_lpi_mode_n),
	DN200_STAT("phy_eee_wakeup_error_n", phy_eee_wakeup_error_n),

	/* TSO */
	DN200_STAT("tx_tso_frames", tx_tso_frames),
	DN200_STAT("tx_tso_nfrags", tx_tso_nfrags),

	/* PF/VF reset stats */
	DN200_STAT("rst_start_count", rst_start_count),
	DN200_STAT("rst_finish_count", rst_finish_count),
	DN200_STAT("rst_start_ok_count", rst_start_ok_count),
	DN200_STAT("rst_finish_ok_count", rst_finish_ok_count),
	DN200_STAT("rst_start_accept_count", rst_start_accept_count),
	DN200_STAT("rst_finish_accept_count", rst_finish_accept_count),
	DN200_STAT("normal_rst_count", normal_rst_count),
	DN200_STAT("tx_timeout_rst_count", tx_timeout_rst_count),
	DN200_STAT("dma_chan_err_rst_count", dma_chan_err_rst_count),

	DN200_STAT("tx_frames_129_to_256", tx_frames_129_to_256),
	DN200_STAT("tx_frames_65_to_128", tx_frames_65_to_128),
	DN200_STAT("tx_frames_33_to_64", tx_frames_33_to_64),
	DN200_STAT("tx_frames_17_to_32", tx_frames_17_to_32),
	DN200_STAT("tx_frames_16_below", tx_frames_16_below),
};

#define DN200_STATS_LEN ARRAY_SIZE(dn200_gstrings_stats)

/* HW MAC Management counters (if supported) */
#define DN200_MMC_STAT(name, m)	\
	{ name, sizeof_field(struct dn200_counters, m),	\
	offsetof(struct dn200_priv, mmc.m)}

/* SW MAC Management counters (if supported) */
#define DN200_SWC_STAT(name, m)       \
	{ name, sizeof_field(struct dn200_swcounters, m),      \
	offsetof(struct dn200_priv, swc.m)}

static const struct dn200_stats dn200_swc[] = {
	DN200_SWC_STAT("port.rx_vlan_strip", mmc_rx_vlan_strip),
	DN200_SWC_STAT("port.rx_fd_drop", mmc_rx_fd_drop),
	DN200_SWC_STAT("port.tx_vlan_insert", mmc_tx_vlan_insert),
	DN200_SWC_STAT("port.tx_mem_copy", tx_mem_copy),
	DN200_SWC_STAT("port.rx_mem_copy", rx_mem_copy),
	DN200_SWC_STAT("port.tx_iatu_hw_updt_cnt", tx_iatu_updt_cnt),
	DN200_SWC_STAT("port.tx_iatu_match_cnt", tx_iatu_match_cnt),
	DN200_SWC_STAT("port.tx_iatu_find_cnt", tx_iatu_find_cnt),
	DN200_SWC_STAT("port.tx_iatu_hw_recyc_cnt", tx_iatu_recyc_cnt),
	DN200_SWC_STAT("port.hw_lock_fail_cnt", hw_lock_fail_cnt),
	DN200_SWC_STAT("port.hw_lock_timeout", hw_lock_timeout),
	DN200_SWC_STAT("port.hw_lock_timeout", hw_lock_recfgs),
};

static const struct dn200_stats dn200_mmc[] = {
	DN200_MMC_STAT("port.tx_octetcount_gb", mmc_tx_octetcount_gb),
	DN200_MMC_STAT("port.tx_framecount_gb", mmc_tx_framecount_gb),
	DN200_MMC_STAT("port.tx_broadcastframe_g", mmc_tx_broadcastframe_g),
	DN200_MMC_STAT("port.tx_multicastframe_g", mmc_tx_multicastframe_g),
	DN200_MMC_STAT("port.tx_64_octets_gb", mmc_tx_64_octets_gb),
	DN200_MMC_STAT("port.tx_65_to_127_octets_gb",
		       mmc_tx_65_to_127_octets_gb),
	DN200_MMC_STAT("port.tx_128_to_255_octets_gb",
		       mmc_tx_128_to_255_octets_gb),
	DN200_MMC_STAT("port.tx_256_to_511_octets_gb",
		       mmc_tx_256_to_511_octets_gb),
	DN200_MMC_STAT("port.tx_512_to_1023_octets_gb",
		       mmc_tx_512_to_1023_octets_gb),
	DN200_MMC_STAT("port.tx_1024_to_max_octets_gb",
		       mmc_tx_1024_to_max_octets_gb),
	DN200_MMC_STAT("port.tx_unicast_gb", mmc_tx_unicast_gb),
	DN200_MMC_STAT("port.tx_multicast_gb", mmc_tx_multicast_gb),
	DN200_MMC_STAT("port.tx_broadcast_gb", mmc_tx_broadcast_gb),
	DN200_MMC_STAT("port.tx_underflow_error", mmc_tx_underflow_error),
	DN200_MMC_STAT("port.tx_octetcount_g", mmc_tx_octetcount_g),
	DN200_MMC_STAT("port.tx_framecount_g", mmc_tx_framecount_g),
	DN200_MMC_STAT("port.tx_pause_frame", mmc_tx_pause_frame),
	DN200_MMC_STAT("port.tx_vlan_frame_g", mmc_tx_vlan_frame_g),
	DN200_MMC_STAT("port.tx_lpi_usec", mmc_tx_lpi_usec),
	DN200_MMC_STAT("port.tx_lpi_tran", mmc_tx_lpi_tran),
	DN200_MMC_STAT("port.rx_framecount_gb", mmc_rx_framecount_gb),
	DN200_MMC_STAT("port.rx_octetcount_gb", mmc_rx_octetcount_gb),
	DN200_MMC_STAT("port.rx_octetcount_g", mmc_rx_octetcount_g),
	DN200_MMC_STAT("port.rx_broadcastframe_g", mmc_rx_broadcastframe_g),
	DN200_MMC_STAT("port.rx_multicastframe_g", mmc_rx_multicastframe_g),
	DN200_MMC_STAT("port.rx_crc_error", mmc_rx_crc_error),
	DN200_MMC_STAT("port.rx_run_error", mmc_rx_run_error),
	DN200_MMC_STAT("port.rx_jabber_error", mmc_rx_jabber_error),
	DN200_MMC_STAT("port.rx_undersize_g", mmc_rx_undersize_g),
	DN200_MMC_STAT("port.rx_oversize_g", mmc_rx_oversize_g),
	DN200_MMC_STAT("port.rx_64_octets_gb", mmc_rx_64_octets_gb),
	DN200_MMC_STAT("port.rx_65_to_127_octets_gb",
		       mmc_rx_65_to_127_octets_gb),
	DN200_MMC_STAT("port.rx_128_to_255_octets_gb",
		       mmc_rx_128_to_255_octets_gb),
	DN200_MMC_STAT("port.rx_256_to_511_octets_gb",
		       mmc_rx_256_to_511_octets_gb),
	DN200_MMC_STAT("port.rx_512_to_1023_octets_gb",
		       mmc_rx_512_to_1023_octets_gb),
	DN200_MMC_STAT("port.rx_1024_to_max_octets_gb",
		       mmc_rx_1024_to_max_octets_gb),
	DN200_MMC_STAT("port.rx_unicast_g", mmc_rx_unicast_g),
	DN200_MMC_STAT("port.rx_length_error", mmc_rx_length_error),
	DN200_MMC_STAT("port.rx_outofrangetype", mmc_rx_outofrangetype),
	DN200_MMC_STAT("port.rx_pause_frames", mmc_rx_pause_frames),
	DN200_MMC_STAT("port.rx_fifo_overflow", mmc_rx_fifo_overflow),
	DN200_MMC_STAT("port.rx_vlan_frames_gb", mmc_rx_vlan_frames_gb),
	DN200_MMC_STAT("port.rx_watchdog_error", mmc_rx_watchdog_error),
	DN200_MMC_STAT("port.rx_lpi_usec", mmc_rx_lpi_usec),
	DN200_MMC_STAT("port.rx_lpi_tran", mmc_rx_lpi_tran),
	DN200_MMC_STAT("port.rx_discard_pkt_gb", mmc_rx_discard_pkt_gb),
	DN200_MMC_STAT("port.rx_discard_oct_gb", mmc_rx_discard_oct_gb),
	DN200_MMC_STAT("port.rx_align_err", mmc_rx_align_err),
	DN200_MMC_STAT("port.rx_ipc_intr_mask", mmc_rx_ipc_intr_mask),
	DN200_MMC_STAT("port.rx_ipc_intr", mmc_rx_ipc_intr),
	DN200_MMC_STAT("port.rx_ipv4_gd", mmc_rx_ipv4_gd),
	DN200_MMC_STAT("port.rx_ipv4_hderr", mmc_rx_ipv4_hderr),
	DN200_MMC_STAT("port.rx_ipv4_nopay", mmc_rx_ipv4_nopay),
	DN200_MMC_STAT("port.rx_ipv4_frag", mmc_rx_ipv4_frag),
	DN200_MMC_STAT("port.rx_ipv4_udsbl", mmc_rx_ipv4_udsbl),
	DN200_MMC_STAT("port.rx_ipv4_gd_octets", mmc_rx_ipv4_gd_octets),
	DN200_MMC_STAT("port.rx_ipv4_hderr_octets", mmc_rx_ipv4_hderr_octets),
	DN200_MMC_STAT("port.rx_ipv4_nopay_octets", mmc_rx_ipv4_nopay_octets),
	DN200_MMC_STAT("port.rx_ipv4_frag_octets", mmc_rx_ipv4_frag_octets),
	DN200_MMC_STAT("port.rx_ipv4_udsbl_octets", mmc_rx_ipv4_udsbl_octets),
	DN200_MMC_STAT("port.rx_ipv6_gd_octets", mmc_rx_ipv6_gd_octets),
	DN200_MMC_STAT("port.rx_ipv6_hderr_octets", mmc_rx_ipv6_hderr_octets),
	DN200_MMC_STAT("port.rx_ipv6_nopay_octets", mmc_rx_ipv6_nopay_octets),
	DN200_MMC_STAT("port.rx_ipv6_gd", mmc_rx_ipv6_gd),
	DN200_MMC_STAT("port.rx_ipv6_hderr", mmc_rx_ipv6_hderr),
	DN200_MMC_STAT("port.rx_ipv6_nopay", mmc_rx_ipv6_nopay),
	DN200_MMC_STAT("port.rx_udp_gd", mmc_rx_udp_gd),
	DN200_MMC_STAT("port.rx_udp_err", mmc_rx_udp_err),
	DN200_MMC_STAT("port.rx_udp_gd_octets", mmc_rx_udp_gd_octets),
	DN200_MMC_STAT("port.rx_udp_err_octets", mmc_rx_udp_err_octets),
	DN200_MMC_STAT("port.rx_tcp_gd", mmc_rx_tcp_gd),
	DN200_MMC_STAT("port.rx_tcp_err", mmc_rx_tcp_err),
	DN200_MMC_STAT("port.rx_tcp_gd_octets", mmc_rx_tcp_gd_octets),
	DN200_MMC_STAT("port.rx_tcp_err_octets", mmc_rx_tcp_err_octets),
	DN200_MMC_STAT("port.rx_icmp_gd", mmc_rx_icmp_gd),
	DN200_MMC_STAT("port.rx_icmp_err", mmc_rx_icmp_err),
	DN200_MMC_STAT("port.rx_icmp_gd_octets", mmc_rx_icmp_gd_octets),
	DN200_MMC_STAT("port.rx_icmp_err_octets", mmc_rx_icmp_err_octets),
	DN200_MMC_STAT("port.tx_fpe_fragment_cntr", mmc_tx_fpe_fragment_cntr),
	DN200_MMC_STAT("port.tx_hold_req_cntr", mmc_tx_hold_req_cntr),
	DN200_MMC_STAT("port.rx_packet_assembly_err_cntr",
				mmc_rx_packet_assembly_err_cntr),
	DN200_MMC_STAT("port.rx_packet_assembly_ok_cntr",
				mmc_rx_packet_assembly_ok_cntr),
	DN200_MMC_STAT("port.rx_fpe_fragment_cntr", mmc_rx_fpe_fragment_cntr),
};

#define DN200_MMC_STATS_LEN ARRAY_SIZE(dn200_mmc)
#define DN200_SWC_STATS_LEN ARRAY_SIZE(dn200_swc)

static const char dn200_qstats_flag_string[][ETH_GSTRING_LEN] = {
	"mmc_all_function",
#define DN200_FLAG_STATS ARRAY_SIZE(dn200_qstats_flag_string)
};

static const char dn200_qstats_tx_string[][ETH_GSTRING_LEN] = {
	"tx_pkt_n",
#define DN200_TXQ_STATS ARRAY_SIZE(dn200_qstats_tx_string)
};

static const char dn200_qstats_rx_string[][ETH_GSTRING_LEN] = {
	"rx_pkt_n",
#define DN200_RXQ_STATS ARRAY_SIZE(dn200_qstats_rx_string)
};

static void dn200_ethtool_getdrvinfo(struct net_device *dev,
				     struct ethtool_drvinfo *info)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret;
	char str_ver[33] = "";
	struct dn200_ver dn200_ver;

	if (priv->plat->has_gmac || priv->plat->has_gmac4)
		strscpy(info->driver, GMAC_ETHTOOL_NAME, sizeof(info->driver));
	else if (priv->plat->has_xgmac)
		strscpy(info->driver, XGMAC_ETHTOOL_NAME, sizeof(info->driver));
	else
		strscpy(info->driver, MAC100_ETHTOOL_NAME,
			sizeof(info->driver));

	if (priv->plat_ex->pdev) {
		strscpy(info->bus_info, pci_name(priv->plat_ex->pdev),
			sizeof(info->bus_info));
	}
	strscpy(info->version, DRV_MODULE_VERSION, sizeof(info->version));

	if (!priv->plat_ex->nvme_supported) {
		ret = dn200_ctrl_ccena(priv->plat_ex->pdev, 0, 1, false);
		if (ret) {
			dev_err(&priv->plat_ex->pdev->dev,
				"func %s, line %d: ctrl cc enable timeout\n",
				__func__, __LINE__);
		}
	}
	if (PRIV_IS_VF(priv)) {
		dn200_sriov_ver_get(priv, &dn200_ver);
	} else {
		dn200_get_fw_ver(&priv->plat_ex->ctrl, &dn200_ver);
		if (PRIV_SRIOV_SUPPORT(priv))
			dn200_sriov_ver_set(priv, &dn200_ver);
	}

	sprintf(str_ver, "%c%c%c%c%c%c%c%c", dn200_ver.type, dn200_ver.product_type,
			 dn200_ver.rsv, dn200_ver.is_fw, dn200_ver.publish, dn200_ver.number0,
			 dn200_ver.number1, dn200_ver.number2);
	strscpy(info->fw_version, str_ver, sizeof(info->fw_version));
}

static int dn200_ethtool_get_link_ksettings(struct net_device *dev,
					    struct ethtool_link_ksettings *cmd)
{
	struct dn200_priv *priv = netdev_priv(dev);

	return PRIV_PHY_OPS(priv)->get_link_ksettings(dev, cmd);
}

static int
dn200_ethtool_set_link_ksettings(struct net_device *dev,
				 const struct ethtool_link_ksettings *cmd)
{
	struct dn200_priv *priv = netdev_priv(dev);

	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;
	return PRIV_PHY_OPS(priv)->set_link_ksettings(dev, cmd);
}

static u32 dn200_ethtool_getmsglevel(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	return priv->msg_enable;
}

static void dn200_ethtool_setmsglevel(struct net_device *dev, u32 level)
{
	struct dn200_priv *priv = netdev_priv(dev);

	priv->msg_enable = level;
}

static int dn200_ethtool_get_regs_len(struct net_device *dev)
{
	int len;
	struct dn200_priv *priv = netdev_priv(dev);

	if (priv->plat->has_xgmac)
		len = XGMAC_REGSIZE * 4;
	else
		len = REG_SPACE_SIZE;

	if (priv->mii) {
		len = ALIGN(len, 16);
		len += GEN_PHY_REG_SPACE_SIZE;
	}

	return len;
}

static void dn200_dump_phy_regs(struct dn200_priv *priv, u32 *reg_space)
{
	int offset;
	int len;
	int i = 0;
	int addr = priv->plat->phy_addr;
	struct phy_device *phydev;
	u32 ext_reg[] = {0xa0, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
					 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1,
					 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xa003};

	if (!priv->mii)
		return;

	len = dn200_ethtool_get_regs_len(priv->dev);

	for (offset = len - GEN_PHY_REG_SPACE_SIZE; offset < len - GEN_PHY_REG_SPACE_SIZE + 32 * 4;
	     offset += 4, i++) {
		reg_space[offset >> 2] =
		    mdiobus_read(priv->mii, priv->plat->phy_addr, i);
	}

	phydev = mdiobus_get_phy(priv->mii, addr);
	if (!phydev)
		return;

	offset = len - GEN_PHY_REG_SPACE_SIZE + 32 * 4;
	for (i = 0; i < ARRAY_SIZE(ext_reg); i++) {
		reg_space[offset >> 2] =
		    ytphy_read_ext(phydev, ext_reg[i]) | (ext_reg[i] << 16);
		offset += 4;
	}
}

static void dn200_ethtool_gregs(struct net_device *dev,
				struct ethtool_regs *regs, void *space)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 *reg_space = (u32 *) space;

	dn200_dump_mac_regs(priv, priv->hw, reg_space);
	dn200_dump_dma_regs(priv, priv->ioaddr, reg_space);
	dn200_dump_phy_regs(priv, reg_space);
}

static int dn200_nway_reset(struct net_device *dev)
{
	int retry = 0;
	struct dn200_priv *priv = netdev_priv(dev);

	if (!netif_running(dev))
		return -EBUSY;

	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;

	if (priv->mii) {
		while (test_and_set_bit(DN200_RESETING, &priv->state)) {
			usleep_range(1000, 2000);
			if (retry++ >= 3)
				return -EBUSY;
		}

		if (netif_running(dev)) {
			dev_close(dev);

			dev_open(dev, NULL);
			dev->netdev_ops->ndo_set_rx_mode(dev);
		}
		clear_bit(DN200_RESETING, &priv->state);
		return 0;
	} else
		return PRIV_PHY_OPS(priv)->nway_reset(PRIV_PHY_INFO(priv));
}

static void
dn200_get_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *ker,
		    struct netlink_ext_ack __always_unused *extack)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	ring->rx_max_pending = DMA_MAX_RX_SIZE;
	ring->tx_max_pending = DMA_MAX_TX_SIZE;
	ring->rx_pending = priv->dma_rx_size;
	ring->tx_pending = priv->dma_tx_size;
}

static int
dn200_set_ringparam(struct net_device *netdev,
		    struct ethtool_ringparam *ring,
		    struct kernel_ethtool_ringparam __always_unused *ker,
		    struct netlink_ext_ack __always_unused *extack)
{
	if (ring->rx_mini_pending || ring->rx_jumbo_pending ||
	    ring->rx_pending < DMA_MIN_RX_SIZE ||
	    ring->rx_pending > DMA_MAX_RX_SIZE ||
	    ring->tx_pending < DMA_MIN_TX_SIZE ||
	    ring->tx_pending > DMA_MAX_TX_SIZE)
		return -EINVAL;
	return dn200_reinit_ringparam(netdev, ring->rx_pending,
				      ring->tx_pending);
}

static void
dn200_get_pauseparam(struct net_device *netdev,
		     struct ethtool_pauseparam *pause)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	PRIV_PHY_OPS(priv)->get_phy_pauseparam(PRIV_PHY_INFO(priv), pause);
}

static int
dn200_set_pauseparam(struct net_device *netdev,
		     struct ethtool_pauseparam *pause)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct ieee_pfc *pfc = priv->pfc;

	if (PRIV_IS_VF(priv))
		return -EOPNOTSUPP;

	/*pfc feature also based on pause hw enable bit */
	if (pfc && pfc->pfc_en) {
		netdev_info(netdev,
			    "Priority flow control is enabled. Cannot set link flow control.\n");
		return -EOPNOTSUPP;
	}

	return PRIV_PHY_OPS(priv)->set_phy_pauseparam(PRIV_PHY_INFO(priv),
						      pause);
}

static void dn200_get_per_qstats(struct dn200_priv *priv, u64 *data,
				 int *count)
{
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	int q, stat;
	char *p;

	for (q = 0; q < tx_cnt; q++) {
		p = (char *)priv + offsetof(struct dn200_priv,
					    xstats.txq_stats[q].tx_pkt_n);
		for (stat = 0; stat < DN200_TXQ_STATS; stat++) {
			*data++ = (*(u64 *) p);
			p += sizeof(u64 *);
			*count = *count + 1;
		}
	}
	for (q = 0; q < rx_cnt; q++) {
		p = (char *)priv + offsetof(struct dn200_priv,
					    xstats.rxq_stats[q].rx_pkt_n);
		for (stat = 0; stat < DN200_RXQ_STATS; stat++) {
			*data++ = (*(u64 *) p);
			p += sizeof(u64 *);
			*count = *count + 1;
		}
	}
}

static void dn200_get_ethtool_stats(struct net_device *dev,
				    struct ethtool_stats *dummy, u64 *data)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int count = 0;
	int i, j = 0;
	char *p;

	for (i = 0; i < DN200_STATS_LEN; i++) {
		char *p = (char *)priv + dn200_gstrings_stats[i].stat_offset;

		data[j++] = (dn200_gstrings_stats[i].sizeof_stat ==
			     sizeof(u64)) ? (*(u64 *) p) : (*(u32 *) p);
	}
	dn200_get_per_qstats(priv, &data[j], &count);
	j = j + count;
	if (PRIV_SRIOV_SUPPORT(priv))
		data[j++] = MMC_DISPLAY_FLAG;

	if (priv->dma_cap.rmon) {
		if (!test_bit(DN200_SUSPENDED, &priv->state) && !test_bit(DN200_DOWN, &priv->state))
			dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);

		for (i = 0; i < DN200_MMC_STATS_LEN; i++) {
			p = (char *)priv + dn200_mmc[i].stat_offset;

			data[j++] = (dn200_mmc[i].sizeof_stat ==
				     sizeof(u64)) ? (*(u64 *) p) : (*(u32 *) p);
		}
	}
	for (i = 0; i < DN200_SWC_STATS_LEN; i++) {
		p = (char *)priv + dn200_swc[i].stat_offset;

		data[j++] = (dn200_swc[i].sizeof_stat ==
			     sizeof(u64)) ? (*(u64 *) p) : (*(u32 *) p);
	}
}

static int dn200_get_sset_count(struct net_device *netdev, int sset)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	int len;

	switch (sset) {
	case ETH_SS_STATS:
		len = DN200_STATS_LEN +
		    DN200_TXQ_STATS * tx_cnt + DN200_RXQ_STATS * rx_cnt;
		if (PRIV_SRIOV_SUPPORT(priv))
			len += DN200_FLAG_STATS;
		if (priv->dma_cap.rmon)
			len += DN200_MMC_STATS_LEN;
		len += DN200_SWC_STATS_LEN;
		return len;
	case ETH_SS_TEST:
		if (PRIV_IS_VF(priv))
			return -EOPNOTSUPP;
		return dn200_selftest_get_count(priv);
	case ETH_SS_PRIV_FLAGS:
		if (PRIV_IS_VF(priv))
			return -EOPNOTSUPP;
		return DN200_PRIV_FLAGS_STR_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

static void dn200_get_qstats_string(struct dn200_priv *priv, u8 *data,
				    int *count)
{
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	int q, stat;

	for (q = 0; q < tx_cnt; q++) {
		for (stat = 0; stat < DN200_TXQ_STATS; stat++) {
			snprintf(data, ETH_GSTRING_LEN, "q%d_%s", q,
				 dn200_qstats_tx_string[stat]);
			data += ETH_GSTRING_LEN;
			*count += ETH_GSTRING_LEN;
		}
	}
	for (q = 0; q < rx_cnt; q++) {
		for (stat = 0; stat < DN200_RXQ_STATS; stat++) {
			snprintf(data, ETH_GSTRING_LEN, "q%d_%s", q,
				 dn200_qstats_rx_string[stat]);
			data += ETH_GSTRING_LEN;
			*count += ETH_GSTRING_LEN;
		}
	}
}

static void dn200_get_priv_flag_strings(struct dn200_priv *priv, u8 *data)
{

	char *p = (char *)data;
	unsigned int i;

	if (PRIV_IS_VF(priv))
		return;
	for (i = 0; i < DN200_PRIV_FLAGS_STR_LEN; i++) {
		snprintf(p, ETH_GSTRING_LEN, "%s",
			 dn200_gstrings_priv_flags[i].flag_string);
		p += ETH_GSTRING_LEN;
	}
}

static void dn200_get_strings(struct net_device *dev, u32 stringset, u8 *data)
{
	int i;
	u8 *p = data;
	int count = 0;
	struct dn200_priv *priv = netdev_priv(dev);

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < DN200_STATS_LEN; i++) {
			memcpy(p, dn200_gstrings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		dn200_get_qstats_string(priv, p, &count);
		p += count;
		if (PRIV_SRIOV_SUPPORT(priv)) {
			memcpy(p, dn200_qstats_flag_string, ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		if (priv->dma_cap.rmon) {
			for (i = 0; i < DN200_MMC_STATS_LEN; i++) {
				memcpy(p, dn200_mmc[i].stat_string,
				       ETH_GSTRING_LEN);
				p += ETH_GSTRING_LEN;
			}
		}
		for (i = 0; i < DN200_SWC_STATS_LEN; i++) {
			memcpy(p, dn200_swc[i].stat_string, ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		break;
	case ETH_SS_TEST:
		if (!PRIV_IS_VF(priv))
			dn200_selftest_get_strings(priv, p);
		break;
	case ETH_SS_PRIV_FLAGS:
		dn200_get_priv_flag_strings(priv, p);
		break;
	default:
		WARN_ON(1);
		break;
	}
}

static int dn200_ethtool_op_get_eee(struct net_device *dev,
				    struct ethtool_eee *edata)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret;

	if (!priv->dma_cap.eee)
		return -EOPNOTSUPP;

	edata->eee_enabled = priv->eee_enabled;
	edata->eee_active = priv->eee_active;
	edata->tx_lpi_timer = priv->tx_lpi_timer;
	edata->tx_lpi_enabled = priv->tx_lpi_enabled;

	ret = PRIV_PHY_OPS(priv)->get_eee(PRIV_PHY_INFO(priv), edata);
	if (priv->mii) {
		edata->tx_lpi_timer = 0;
		priv->tx_lpi_enabled = edata->eee_enabled;
		edata->tx_lpi_enabled = priv->tx_lpi_enabled;
	}

	return ret;
}

static int dn200_ethtool_op_set_eee(struct net_device *dev,
				    struct ethtool_eee *edata)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret;

	if (!priv->dma_cap.eee)
		return -EOPNOTSUPP;

	if (!priv->mii && netif_running(dev))
		return -EBUSY;
	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;

	if (priv->mii) {
		if (priv->tx_lpi_enabled != edata->tx_lpi_enabled) {
			netdev_err(dev, "Setting EEE tx-lpi is not supported\n");
			return -EINVAL;
		}
		if (edata->tx_lpi_timer) {
			netdev_err(dev, "Setting EEE Tx LPI timer is not supported\n");
			return -EINVAL;
		}
	}

	if (priv->tx_lpi_enabled != edata->tx_lpi_enabled)
		netdev_warn(priv->dev, "Setting EEE tx-lpi is not supported\n");

	if (!edata->eee_enabled)
		dn200_disable_eee_mode(priv);

	ret = PRIV_PHY_OPS(priv)->set_eee(PRIV_PHY_INFO(priv), edata);
	if (ret)
		return ret;

	if (edata->eee_enabled && priv->tx_lpi_timer != edata->tx_lpi_timer) {
		priv->tx_lpi_timer = edata->tx_lpi_timer;
		dn200_eee_init(priv);
	}

	return 0;
}

static int __dn200_get_coalesce(struct net_device *dev,
				struct ethtool_coalesce *ec, int queue)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 max_cnt;
	u32 rx_cnt;
	u32 tx_cnt;

	rx_cnt = priv->plat->rx_queues_to_use;
	tx_cnt = priv->plat->tx_queues_to_use;
	max_cnt = max(rx_cnt, tx_cnt);

	if (queue < 0)
		queue = 0;
	else if (queue >= max_cnt)
		return -EINVAL;

	if (queue < tx_cnt) {
		ec->tx_coalesce_usecs = priv->tx_coal_timer[queue];
		ec->tx_max_coalesced_frames = priv->tx_coal_frames_set[queue];
		if (!!
		    (priv->tx_intr[queue].itr_setting & DN200_ITR_DYNAMIC_ITR))
			ec->use_adaptive_tx_coalesce = 1;
	} else {
		ec->tx_coalesce_usecs = 0;
		ec->tx_max_coalesced_frames = 0;
	}

	if (priv->use_riwt && queue < rx_cnt) {
		ec->rx_max_coalesced_frames = priv->rx_coal_frames[queue];
		ec->rx_coalesce_usecs = priv->rx_rius[queue];
		if (!!
		    (priv->rx_intr[queue].itr_setting & DN200_ITR_DYNAMIC_ITR))
			ec->use_adaptive_rx_coalesce = 1;
	} else {
		ec->rx_max_coalesced_frames = 0;
		ec->rx_coalesce_usecs = 0;
	}

	return 0;
}

static int dn200_get_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce __maybe_unused *kec,
			      struct netlink_ext_ack __maybe_unused *extack)
{
	return __dn200_get_coalesce(dev, ec, -1);
}

static int dn200_get_per_queue_coalesce(struct net_device *dev, u32 queue,
					struct ethtool_coalesce *ec)
{
	return __dn200_get_coalesce(dev, ec, queue);
}

static int __dn200_set_coalesce(struct net_device *dev,
				struct ethtool_coalesce *ec, int queue)
{
	struct dn200_priv *priv = netdev_priv(dev);
	bool all_queues = false;
	unsigned int rx_riwt;
	u32 max_cnt;
	u32 rx_cnt;
	u32 tx_cnt;
	u32 tx_frame;
	int i;

	rx_cnt = priv->plat->rx_queues_to_use;
	tx_cnt = priv->plat->tx_queues_to_use;
	max_cnt = max(rx_cnt, tx_cnt);

	if (queue < 0)
		all_queues = true;
	else if (queue >= max_cnt)
		return -EINVAL;

	if (ec->rx_max_coalesced_frames >=
	    (priv->dma_rx_size - dn200_rx_refill_size(priv))) {
		netdev_err(dev,
			   "rx frams plus 32 need to be less than ring_size (now %d)\n",
			   priv->dma_rx_size);
		return -EINVAL;
	}
	if (ec->tx_max_coalesced_frames > (priv->dma_tx_size >> 1)) {
		netdev_err(dev,
			   "TX frams need to be less than ring_size/2 (now ring_size is %d)\n",
			   priv->dma_rx_size);
		return -EINVAL;
	}

	if ((ec->rx_coalesce_usecs != priv->rx_rius[queue < 0 ? 0 : queue] ||
		ec->rx_max_coalesced_frames != priv->rx_coal_frames[queue < 0 ? 0 : queue]) &&
		ec->use_adaptive_rx_coalesce) {
		netdev_err(dev,
			"RX interrupt moderation cannot be changed if adaptive-rx is enabled.\n");
		return -EINVAL;
	}

	if (ec->rx_coalesce_usecs > DN200_MAX_COAL_RX_TICK)
		return -EINVAL;

	rx_riwt = dn200_usec2riwt(ec->rx_coalesce_usecs, priv);
	if (rx_riwt > MAX_DMA_RIWT)
		rx_riwt = MAX_DMA_RIWT;

	if (rx_riwt < MIN_DMA_RIWT)
		rx_riwt = MIN_DMA_RIWT;

	if (priv->use_riwt && ec->use_adaptive_rx_coalesce) {
		if (all_queues) {
			for (i = 0; i < rx_cnt; i++) {
				priv->rx_riwt[i] = rx_riwt;
				priv->rx_rius[i] = ec->rx_coalesce_usecs;
				priv->rx_intr[i].target_itr =
				    ec->rx_coalesce_usecs;
				dn200_rx_watchdog(priv, priv->ioaddr, rx_riwt,
						  i, priv->hw);
				priv->rx_intr[i].itr_setting |=
				    DN200_ITR_DYNAMIC_ITR;
				priv->rx_coal_frames[i] =
				    ec->rx_max_coalesced_frames;
			}
		} else if (queue < rx_cnt) {
			priv->rx_riwt[queue] = rx_riwt;
			priv->rx_rius[queue] = ec->rx_coalesce_usecs;
			priv->rx_intr[queue].target_itr = ec->rx_coalesce_usecs;
			priv->rx_coal_frames[queue] =
			    ec->rx_max_coalesced_frames;
			priv->rx_intr[queue].itr_setting |=
			    DN200_ITR_DYNAMIC_ITR;
			dn200_rx_watchdog(priv, priv->ioaddr, rx_riwt, queue,
					  priv->hw);

		}
	} else if (priv->use_riwt) {
		if (all_queues) {
			for (i = 0; i < rx_cnt; i++) {
				priv->rx_riwt[i] = rx_riwt;
				priv->rx_rius[i] = ec->rx_coalesce_usecs;
				priv->rx_intr[i].itr_setting &=
				    ~DN200_ITR_DYNAMIC_ITR;
				priv->rx_intr[i].target_itr =
					dn200_riwt2usec(rx_riwt, priv);
				/* prevent rx_riwt to 0 after exec dn200_usec2riwt */
				if (priv->rx_intr[i].target_itr < 2)
					priv->rx_intr[i].target_itr = 2;
				dn200_rx_watchdog(priv, priv->ioaddr, rx_riwt,
						  i, priv->hw);
				priv->rx_coal_frames[i] =
				    ec->rx_max_coalesced_frames;
			}
		} else if (queue < rx_cnt) {
			priv->rx_riwt[queue] = rx_riwt;
			priv->rx_rius[queue] = ec->rx_coalesce_usecs;
			priv->rx_intr[queue].itr_setting &=
			    ~DN200_ITR_DYNAMIC_ITR;
			priv->rx_intr[queue].target_itr =
				dn200_riwt2usec(rx_riwt, priv);
			/* prevent rx_riwt to 0 after exec dn200_usec2riwt */
			if (priv->rx_intr[queue].target_itr < 2)
				priv->rx_intr[queue].target_itr = 2;
			dn200_rx_watchdog(priv, priv->ioaddr,
					  rx_riwt, queue, priv->hw);
			priv->rx_coal_frames[queue] =
			    ec->rx_max_coalesced_frames;
		}
	}

	if ((ec->tx_coalesce_usecs != priv->tx_coal_timer[queue < 0 ? 0 : queue] ||
		ec->tx_max_coalesced_frames != priv->tx_coal_frames_set[queue < 0 ? 0 : queue]) &&
		ec->use_adaptive_tx_coalesce) {
		netdev_err(dev,
			"TX interrupt moderation cannot be changed if adaptive-tx is enabled.\n");
		return -EINVAL;
	}

	if (ec->tx_coalesce_usecs > DN200_MAX_COAL_TX_TICK)
		return -EINVAL;

	if (ec->use_adaptive_tx_coalesce) {
		if (all_queues) {
			int i;

			for (i = 0; i < tx_cnt; i++) {
				priv->tx_coal_frames_set[i] =
				    ec->tx_max_coalesced_frames;
				priv->tx_coal_frames[i] =
				    ec->tx_max_coalesced_frames;
				priv->tx_coal_timer[i] = ec->tx_coalesce_usecs;
				priv->tx_intr[i].itr_setting |=
				    DN200_ITR_DYNAMIC_ITR;
				priv->tx_intr[i].target_itr =
				    ec->tx_max_coalesced_frames;
			}
		} else if (queue < tx_cnt) {
			priv->tx_coal_frames_set[queue] =
			    ec->tx_max_coalesced_frames;
			priv->tx_coal_frames[queue] =
			    ec->tx_max_coalesced_frames;
			priv->tx_coal_timer[queue] = ec->tx_coalesce_usecs;
			priv->tx_intr[queue].itr_setting |=
			    DN200_ITR_DYNAMIC_ITR;
			priv->tx_intr[queue].target_itr =
			    ec->tx_max_coalesced_frames;
		}
	} else {
		if (ec->tx_max_coalesced_frames > DN200_TX_MAX_FRAMES)
			tx_frame = DN200_TX_MAX_FRAMES;
		else if (ec->tx_max_coalesced_frames <= 0)
			tx_frame = 1;
		else
			tx_frame = ec->tx_max_coalesced_frames;

		if (all_queues) {
			int i;

			for (i = 0; i < tx_cnt; i++) {
				priv->tx_coal_frames[i] = tx_frame;
				priv->tx_coal_frames_set[i] =
				    ec->tx_max_coalesced_frames;
				priv->tx_coal_timer[i] = ec->tx_coalesce_usecs;
				priv->tx_intr[i].itr_setting &=
				    ~DN200_ITR_DYNAMIC_ITR;
				priv->tx_intr[i].target_itr = tx_frame;

			}
		} else if (queue < tx_cnt) {
			priv->tx_coal_frames_set[queue] =
			    ec->tx_max_coalesced_frames;
			priv->tx_coal_frames[queue] = tx_frame;
			priv->tx_coal_timer[queue] = ec->tx_coalesce_usecs;
			priv->tx_intr[queue].itr_setting &=
			    ~DN200_ITR_DYNAMIC_ITR;
			priv->tx_intr[queue].target_itr = tx_frame;
		}
	}

	return 0;
}

static int dn200_set_coalesce(struct net_device *dev,
			      struct ethtool_coalesce *ec,
			      struct kernel_ethtool_coalesce __maybe_unused *kec,
			      struct netlink_ext_ack __maybe_unused *extack)
{
	return __dn200_set_coalesce(dev, ec, -1);
}

static int dn200_set_per_queue_coalesce(struct net_device *dev, u32 queue,
					struct ethtool_coalesce *ec)
{
	if (netif_running(dev))
		return -EBUSY;

	return __dn200_set_coalesce(dev, ec, queue);
}

static u32 dn200_get_rxfh_key_size(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	return sizeof(priv->rss.key);
}

static u32 dn200_get_rxfh_indir_size(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	return ARRAY_SIZE(priv->rss.table);
}

static int dn200_get_rxfh(struct net_device *dev, u32 *indir, u8 *key,
			  u8 *hfunc)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int i;

	if (indir) {
		for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
			indir[i] = priv->rss.table[i];
	}

	if (key)
		memcpy(key, priv->rss.key, sizeof(priv->rss.key));
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;

	return 0;
}

static int dn200_set_rxfh(struct net_device *dev, const u32 *indir,
			  const u8 *key, const u8 hfunc)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int i;

	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;

	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;

	if (indir) {
		for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
			priv->rss.table[i] = indir[i];
	}

	if (key)
		memcpy(priv->rss.key, key, sizeof(priv->rss.key));

	return dn200_rss_configure(priv, priv->hw, &priv->rss,
				   priv->plat->rx_queues_to_use);
}

static void dn200_get_max_channels(struct dn200_priv *priv,
				   unsigned int *rx, unsigned int *tx)
{
	unsigned int rx_max, tx_max;

	if (priv->plat_ex->pf_id == 0 || priv->plat_ex->pf_id == 1) {
		rx_max = 8;
		tx_max = 8;
	}
	if (priv->plat_ex->pf_id == 2 || priv->plat_ex->pf_id == 3) {
		if (PRIV_SRIOV_SUPPORT(priv)) {
			rx_max = 1;
			tx_max = 1;
		} else {
			rx_max = 2;
			tx_max = 2;
		}
	}
	if (PRIV_IS_VF(priv)) {
		rx_max =
		    priv->plat_ex->rx_queues_reserved / priv->plat_ex->max_vfs;
		tx_max =
		    priv->plat_ex->tx_queues_reserved / priv->plat_ex->max_vfs;
		tx_max = min_t(unsigned int, DN200_MAX_QPS_PER_VF, tx_max);
		tx_max = min_t(unsigned int, DN200_MAX_QPS_PER_VF, rx_max);
	}
	*rx = rx_max;
	*tx = tx_max;
}

static void dn200_get_channels(struct net_device *dev,
			       struct ethtool_channels *chan)
{
	struct dn200_priv *priv = netdev_priv(dev);
	unsigned int rx, tx, combined;

	dn200_get_max_channels(priv, &rx, &tx);
	combined = min(rx, tx);
	chan->max_combined = combined;
	rx = priv->plat->rx_queues_to_use;
	tx = priv->plat->tx_queues_to_use;
	combined = min(rx, tx);
	chan->combined_count = combined;
}

static int dn200_set_channels(struct net_device *dev,
			      struct ethtool_channels *chan)
{
	unsigned int rx, tx, cur_rx, cur_tx, rx_max, tx_max, combined_max;
	struct dn200_priv *priv = netdev_priv(dev);
	int i = 0;
	struct dn200_fdir_filter *input;
	int err = 0;

	dn200_get_max_channels(priv, &rx_max, &tx_max);
	combined_max = min(rx_max, tx_max);
	/* Should not be setting other count */
	if (chan->other_count) {
		netdev_err(dev, "other channel count must be zero\n");
		return -EINVAL;
	}

	/* Require at least one Combined (Rx and Tx) channel */
	if (!chan->combined_count) {
		netdev_err(dev,
			   "at least one combined Rx/Tx channel is required\n");
		return -EINVAL;
	}

	/* Check combined channels */
	if (chan->combined_count > combined_max) {
		netdev_err(dev,
			   "combined channel count cannot exceed %u\n",
			   combined_max);
		return -EINVAL;
	}

	/* Can have some Rx-only or Tx-only channels, but not both */
	if (chan->rx_count || chan->tx_count) {
		netdev_err(dev, "cannot specify Rx or Tx channels\n");
		return -EINVAL;
	}

	for (; i < priv->flow_entries_max - 4; i++) {
		input = &priv->fdir_enties[i];
		if (input && input->enable &&
		    (input->action & DN200_FLOW_ACTION_ROUTE) &&
		    input->queue >= chan->combined_count) {
			netdev_warn(dev,
				    "Existing user defined filter %d assigns flow to queue %d\n",
				    i, input->queue);
			err = -EINVAL;
		}
	}
	if (err) {
		netdev_err(dev,
			   "Existing filter rules must be deleted to reduce combined channel count to %d\n",
			   chan->combined_count);
		return err;
	}

	rx = chan->combined_count;
	tx = chan->combined_count;

	cur_rx = priv->plat->rx_queues_to_use;
	cur_tx = priv->plat->tx_queues_to_use;

	if (rx == cur_rx && tx == cur_tx)
		goto out;
	return dn200_reinit_queues(dev, rx, tx);
out:
	return 0;
}

static int dn200_get_ts_info(struct net_device *dev,
			     struct ethtool_ts_info *info)
{
	struct dn200_priv *priv = netdev_priv(dev);

	if ((!PRIV_IS_VF(priv))
	    && (priv->dma_cap.time_stamp || priv->dma_cap.atime_stamp)) {

		info->so_timestamping = SOF_TIMESTAMPING_TX_SOFTWARE |
		    SOF_TIMESTAMPING_TX_HARDWARE |
		    SOF_TIMESTAMPING_RX_SOFTWARE |
		    SOF_TIMESTAMPING_RX_HARDWARE |
		    SOF_TIMESTAMPING_SOFTWARE | SOF_TIMESTAMPING_RAW_HARDWARE;

		if (priv->ptp_clock)
			info->phc_index = ptp_clock_index(priv->ptp_clock);

		info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);

		info->rx_filters = ((1 << HWTSTAMP_FILTER_NONE) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_EVENT) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_SYNC) |
				    (1 << HWTSTAMP_FILTER_PTP_V2_DELAY_REQ) |
				    (1 << HWTSTAMP_FILTER_ALL));
		return 0;
	} else {
		return ethtool_op_get_ts_info(dev, info);
	}
}

static int dn200_set_priv_flags(struct net_device *dev, u32 flags)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 i, orig_flags, new_flags, changed_flags;
	int ret = 0;

	/*needs according to priv->flags to operate */
	orig_flags = priv->eth_priv_flags;
	new_flags = orig_flags;
	if (PRIV_IS_VF(priv))
		return -EOPNOTSUPP;
	for (i = 0; i < DN200_PRIV_FLAGS_STR_LEN; i++) {
		const struct dn200_priv_flags *priv_flags;

		priv_flags = &dn200_gstrings_priv_flags[i];

		if (flags & BIT(i))
			new_flags |= priv_flags->flag;
		else
			new_flags &= ~(priv_flags->flag);

		/* If this is a read-only flag, it can't be changed */
		if (priv_flags->read_only &&
		    ((orig_flags ^ new_flags) & ~BIT(i)))
			return -EOPNOTSUPP;
	}
	changed_flags = orig_flags ^ new_flags;

	if (!changed_flags)
		return 0;

	if (changed_flags & DN200_PRIV_FLAGS_FEC_EN) {
		ret = dn200_phy_fec_enable(dev,
			!!(new_flags & DN200_PRIV_FLAGS_FEC_EN));
		if (ret)
			return ret;

		if (new_flags & DN200_PRIV_FLAGS_FEC_EN)
			priv->eth_priv_flags |= DN200_PRIV_FLAGS_FEC_EN;
		else
			priv->eth_priv_flags &= ~DN200_PRIV_FLAGS_FEC_EN;
	}

	if (changed_flags & DN200_PRIV_FLAGS_PTP_RXTX) {
		ret = dn200_reinit_hwts(dev, !(orig_flags & DN200_PRIV_FLAGS_PTP_RXTX),
				      new_flags);
	}

	return ret;
}

static u32 dn200_get_priv_flags(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 i, ret_flags = 0;

	if (PRIV_IS_VF(priv))
		return -EOPNOTSUPP;

	for (i = 0; i < DN200_PRIV_FLAGS_STR_LEN; i++) {
		const struct dn200_priv_flags *priv_flags;

		priv_flags = &dn200_gstrings_priv_flags[i];

		if (priv_flags->flag & priv->eth_priv_flags)
			ret_flags |= BIT(i);
	}
	return ret_flags;
}

static int dn200_set_phys_id(struct net_device *dev,
			     enum ethtool_phys_id_state state)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);

	if (!phy_info->phy_ops->led_control)
		return -EOPNOTSUPP;
	if (!phy_info->phy_ops->blink_control)
		return -EOPNOTSUPP;
	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		return 2;
	case ETHTOOL_ID_ON:
		if (phy_info->phydev)
			extern_phy_force_led(phy_info->phydev, priv, 1, 0);
		else
			dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_ENABLE);
		break;
	case ETHTOOL_ID_OFF:
		if (phy_info->phydev)
			extern_phy_force_led(phy_info->phydev, priv, 1, 1);
		break;
	case ETHTOOL_ID_INACTIVE:
		if (netif_running(dev)) {
			priv->blink_state_last = BLINK_ENABLE;
			if (!phy_info->phydev)
				dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_ENABLE);
		} else
			if (!phy_info->phydev)
				dn200_led_blink_ctrl(&priv->plat_ex->ctrl, BLINK_DISABLE);

		if (phy_info->phydev) {
			if (phy_info->phydev->link && phy_info->phydev->speed == SPEED_1000)
				extern_phy_force_led(phy_info->phydev, priv, 1, 0);
			else if (phy_info->phydev->link && phy_info->phydev->speed == SPEED_100)
				extern_phy_force_led(phy_info->phydev, priv, 2, 0);
			else
				extern_phy_force_led(phy_info->phydev, priv, 1, 1);
		}
		break;
	}
	return 0;
}

static int dn200_get_rss_hash_opts(struct dn200_priv *priv,
				   struct ethtool_rxnfc *cmd)
{
	u32 flags = priv->rss.rss_flags;

	cmd->data = 0;

	/* Report default options for RSS on dn200 */
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (flags & DN200_RSS_TCP4TE)
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		fallthrough;
	case IPV4_FLOW:
		if (flags & DN200_RSS_IP2TE)
			cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	case UDP_V6_FLOW:
	case UDP_V4_FLOW:
		if (flags & DN200_RSS_UDP4TE)
			cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
		fallthrough;
	case IPV6_FLOW:
		if (flags & DN200_RSS_IP2TE)
			cmd->data |= RXH_IP_SRC | RXH_IP_DST;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int dn200_get_fdir_entry(struct dn200_priv *priv,
				struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp;
	struct dn200_fdir_filter *input;

	fsp = (struct ethtool_rx_flow_spec *)&cmd->fs;

	if (fsp->location >= (priv->flow_entries_max - 4))
		return -EINVAL;
	input = &priv->fdir_enties[fsp->location];
	if (!input->enable)
		return -EINVAL;

	/* set flow type field */
	if (input->flow_type & DN200_FLOW_TYPE_V4) {
		if (input->flow_type & DN200_FLOW_TYPE_UDP)
			fsp->flow_type = UDP_V4_FLOW;
		else if (input->flow_type & DN200_FLOW_TYPE_TCP)
			fsp->flow_type = TCP_V4_FLOW;
		else
			fsp->flow_type = IPV4_USER_FLOW;
		fsp->h_u.tcp_ip4_spec.psrc = input->src_port;
		fsp->h_u.tcp_ip4_spec.pdst = input->dst_port;
		fsp->m_u.tcp_ip4_spec.psrc = 0;
		fsp->m_u.tcp_ip4_spec.pdst = 0;
		fsp->h_u.tcp_ip4_spec.ip4src = input->src_ip;
		fsp->m_u.tcp_ip4_spec.ip4src = input->src_ip_mask;
		fsp->h_u.tcp_ip4_spec.ip4dst = input->dst_ip;
		fsp->m_u.tcp_ip4_spec.ip4dst = input->dst_ip_mask;
		fsp->h_u.usr_ip4_spec.ip_ver = ETH_RX_NFC_IP4;
	} else {
		if (input->flow_type & DN200_FLOW_TYPE_UDP)
			fsp->flow_type = UDP_V6_FLOW;
		else if (input->flow_type & DN200_FLOW_TYPE_TCP)
			fsp->flow_type = TCP_V6_FLOW;
		else
			fsp->flow_type = IPV6_USER_FLOW;
		memset(fsp->h_u.tcp_ip6_spec.ip6src, 0, sizeof(__be32) * 4);
		memset(fsp->m_u.tcp_ip6_spec.ip6src, 0, sizeof(__be32) * 4);
		memset(fsp->h_u.tcp_ip6_spec.ip6dst, 0, sizeof(__be32) * 4);
		memset(fsp->m_u.tcp_ip6_spec.ip6dst, 0, sizeof(__be32) * 4);
		fsp->h_u.tcp_ip6_spec.psrc = input->src_port;
		fsp->h_u.tcp_ip6_spec.pdst = input->dst_port;

		if ((input->flow_type & DN200_FLOW_TYPE_V6)
		    && (input->flow_type & DN200_FLOW_TYPE_SA)) {
			memcpy(fsp->h_u.tcp_ip6_spec.ip6src, input->ip6,
			       sizeof(__be32) * 4);
			memcpy(fsp->m_u.tcp_ip6_spec.ip6src, input->ip6_mask,
			       sizeof(__be32) * 4);
		} else if ((input->flow_type & DN200_FLOW_TYPE_V6)
			   && (input->flow_type & DN200_FLOW_TYPE_DA)) {
			memcpy(fsp->h_u.tcp_ip6_spec.ip6dst, input->ip6,
			       sizeof(__be32) * 4);
			memcpy(fsp->m_u.tcp_ip6_spec.ip6dst, input->ip6_mask,
			       sizeof(__be32) * 4);
		}
	}
	/* record action */
	if (input->action == DN200_FLOW_ACTION_DROP)
		fsp->ring_cookie = RX_CLS_FLOW_DISC;
	else
		fsp->ring_cookie = input->queue;
	return 0;
}

static int dn200_get_fdir_all(struct dn200_priv *priv,
			      struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct dn200_fdir_filter *input;
	int cnt = 0;
	int i = priv->flow_entries_max - 5;

	/* report total rule count */
	cmd->data = priv->flow_entries_max - 4;
	for (; i >= 0; i--) {
		input = &priv->fdir_enties[i];
		if (input->enable) {
			rule_locs[cnt] = i;
			cnt++;
		}
	}
	cmd->rule_cnt = cnt;
	return 0;
}

static int dn200_get_rxnfc(struct net_device *netdev,
			   struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = priv->plat->rx_queues_to_use;
		return 0;
	case ETHTOOL_GRXFH:
		return dn200_get_rss_hash_opts(priv, cmd);
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = priv->fdir_counts;
		return 0;
	case ETHTOOL_GRXCLSRULE:
		return dn200_get_fdir_entry(priv, cmd);
	case ETHTOOL_GRXCLSRLALL:
		return dn200_get_fdir_all(priv, cmd, rule_locs);
	default:
		return -EOPNOTSUPP;
	}
}

static int dn200_set_rss_hash_opt(struct dn200_priv *priv,
				  struct ethtool_rxnfc *nfc)
{
	u32 flags = priv->rss.rss_flags;

	/**
	 * RSS does not support anything other than hashing
	 * to queues on src and dst IPs and ports
	 */
	if (nfc->data & ~(RXH_IP_SRC | RXH_IP_DST |
			  RXH_L4_B_0_1 | RXH_L4_B_2_3))
		return -EINVAL;

	switch (nfc->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (!(nfc->data & (RXH_IP_SRC | RXH_IP_DST)) &&
		    !(nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)))
			return -EINVAL;
		flags &= ~(DN200_RSS_IP2TE | DN200_RSS_TCP4TE);
		if (nfc->data & (RXH_IP_SRC | RXH_IP_DST))
			flags |= DN200_RSS_IP2TE;
		if (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3))
			flags |= DN200_RSS_TCP4TE;
		break;
	case UDP_V4_FLOW:
	case UDP_V6_FLOW:
		if (!(nfc->data & (RXH_IP_SRC | RXH_IP_DST)) &&
		    !(nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)))
			return -EINVAL;
		flags &= ~(DN200_RSS_IP2TE | DN200_RSS_UDP4TE);
		if (nfc->data & (RXH_IP_SRC | RXH_IP_DST))
			flags |= DN200_RSS_IP2TE;
		if (nfc->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3))
			flags |= DN200_RSS_UDP4TE;
		break;
	default:
		return -EINVAL;
	}

	/* if we changed something we need to update flags */
	if (flags != priv->rss.rss_flags) {
		priv->rss.rss_flags = flags;
		dn200_rss_configure(priv, priv->hw, &priv->rss,
				    priv->plat->rx_queues_to_use);
	}

	return 0;
}

static int dn200_check_flow_type_supported(struct ethtool_rx_flow_spec *fsp)
{
	switch (fsp->flow_type) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
	case IPV4_USER_FLOW:
	case IPV6_USER_FLOW:
		return 1;
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
		return 1;
	default:
		return 0;
	}

	return 0;
}

static int dn200_check_flow_type_conflict(struct ethtool_rx_flow_spec *fsp,
					  struct dn200_fdir_info *fdir_info)
{
	switch (fsp->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
		if (fdir_info->l4_udp_count)
			return 1;
		break;
	case UDP_V6_FLOW:
	case UDP_V4_FLOW:
		if (fdir_info->l4_tcp_count)
			return 1;
		break;
	default:
		return 0;
	}

	return 0;
}

static int dn200_ethtool_mask_to_xgmac_mask(struct dn200_priv *priv, u32 mask)
{
	u32 mask_tmp = 0xffffffff;
	u32 i = 0;

	if (!mask)
		return 32;
	for (; mask_tmp;) {
		if (mask == mask_tmp)
			return i;
		i++;
		mask_tmp = (mask_tmp << 1);
	}
	netdev_err(priv->dev, "Unsupported ip mask\n");
	return -1;
}

static bool dn200_match_fdir_filter(struct dn200_fdir_filter *a,
				    struct dn200_fdir_filter *b)
{
	/* The filters do not much if any of these criteria differ. */
	if (a->dst_port != b->dst_port ||
	    a->src_port != b->src_port ||
	    a->flow_type != b->flow_type ||
	    ((a->flow_type & DN200_FLOW_TYPE_V4) &&
	     (a->dst_ip != b->dst_ip ||
	      a->src_ip != b->src_ip ||
	      a->xgmac_mask_src != b->xgmac_mask_src ||
	      a->xgmac_mask_dst != b->xgmac_mask_dst)) ||
	    ((a->flow_type & DN200_FLOW_TYPE_V6) &&
	     (a->ip6_address != b->ip6_address ||
	      a->ip6[0] != b->ip6[0] ||
	      a->ip6[1] != b->ip6[1] ||
	      a->ip6[2] != b->ip6[2] ||
	      a->ip6[3] != b->ip6[3] ||
	      a->ip6_mask[0] != b->ip6_mask[0] ||
	      a->ip6_mask[1] != b->ip6_mask[1] ||
	      a->ip6_mask[2] != b->ip6_mask[2] ||
	      a->ip6_mask[3] != b->ip6_mask[3])))
		return false;

	return true;
}

static int dn200_disallow_matching_filters(struct dn200_priv *priv,
					   struct dn200_fdir_filter *input)
{
	struct dn200_fdir_filter *tmp;
	int i = priv->flow_entries_max - 5;

	for (; i >= 0; i--) {
		tmp = &priv->fdir_enties[i];
		if (i == input->reg_idx)
			continue;
		if (input->enable)
			continue;
		if (dn200_match_fdir_filter(tmp, input)) {
			netdev_err(priv->dev,
				"Existing user defined filter %d already matches this flow.\n", i);
			return -EINVAL;
		}
	}
	return 0;
}

#define XGMAC_NTUPLE_MAX_V4_ADDR_MASK_BITS (31)
#define XGMAC_NTUPLE_MAX_V6_ADDR_MASK_BITS (127)
static int dn200_add_fdir_ethtool(struct dn200_priv *priv,
				  struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp;
	u8 queue = 0;
	u8 action = DN200_FLOW_ACTION_ROUTE;
	struct dn200_fdir_filter *input;
	u8 ip6_address = 0;
	int mask;
	int ret = 0;

	if (!(priv->dev->features & NETIF_F_NTUPLE)) {
		netdev_err(priv->dev, "Cannot configure new rule when ntuple is disabled\n");
		return -EOPNOTSUPP;
	}
	fsp = (struct ethtool_rx_flow_spec *)&cmd->fs;

	/* Extended MAC field or vlan field are not supported */
	if ((fsp->flow_type & FLOW_MAC_EXT) || (fsp->flow_type & FLOW_EXT))
		return -EINVAL;
	/* Don't allow indexes to exist outside of available space */
	if (fsp->location >= (priv->flow_entries_max - 4)) {
		netdev_err(priv->dev, "Location out of range\n");
		return -EINVAL;
	}

	if (fsp->ring_cookie == RX_CLS_FLOW_DISC) {
		action = DN200_FLOW_ACTION_DROP;
	} else {
		u32 ring = ethtool_get_flow_spec_ring(fsp->ring_cookie);

		if (ring >= priv->plat->rx_queues_to_use)
			return -EINVAL;
		queue = ring;
	}

	/* record flow type */
	if (!dn200_check_flow_type_supported(fsp)) {
		netdev_err(priv->dev, "Unrecognized flow type: %d\n",
			   fsp->flow_type);
		return -EINVAL;
	}

	/*Whether a type conflict exists */
	if (dn200_check_flow_type_conflict(fsp, &priv->fdir_info)) {
		netdev_err(priv->dev, "Conflict flow type existed\n");
		return -EINVAL;
	}
	input = &priv->fdir_enties[fsp->location];
	if (input->enable) {
		netdev_err(priv->dev, "Location(%d) is enabled\n",
			   fsp->location);
		return -EINVAL;
	}
	memset(input, 0, sizeof(*input));
	if (fsp->flow_type == UDP_V6_FLOW || fsp->flow_type == UDP_V4_FLOW) {
		input->flow_type |= DN200_FLOW_TYPE_UDP;
	} else if (fsp->flow_type == TCP_V6_FLOW
		   || fsp->flow_type == TCP_V4_FLOW) {
		input->flow_type |= DN200_FLOW_TYPE_TCP;
	}
	if (fsp->flow_type == UDP_V6_FLOW ||
	    fsp->flow_type == TCP_V6_FLOW || fsp->flow_type == IPV6_USER_FLOW) {
		/* Reverse the src and dest notion, since the HW expects them
		 * to be from Tx perspective where as the input from user is
		 * from Rx filter view.
		 */
		ip6_address = 0;
		if ((fsp->h_u.tcp_ip6_spec.ip6src[0] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6src[1] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6src[2] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6src[3] != 0)
		    ) {
			ip6_address |= DN200_L3L4_IPV6_SA;
		}

		if ((fsp->h_u.tcp_ip6_spec.ip6dst[0] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6dst[1] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6dst[2] != 0 ||
		     fsp->h_u.tcp_ip6_spec.ip6dst[3] != 0)
		    ) {
			ip6_address |= DN200_L3L4_IPV6_DA;
		}
		if (ip6_address == (DN200_L3L4_IPV6_SA | DN200_L3L4_IPV6_DA)) {
			netdev_err(priv->dev, "Ipv6 only support source or dest ip address\n");
			return -EINVAL;
		}
		input->flow_type |= DN200_FLOW_TYPE_V6;
		input->reg_idx = fsp->location;
		if (fsp->flow_type == IPV6_USER_FLOW) {
			if (fsp->h_u.usr_ip6_spec.l4_4_bytes) {
				netdev_err(priv->dev, "Ipv6 User spec not support L4 config\n");
				return -EOPNOTSUPP;
			}
		} else {
			input->dst_port = fsp->h_u.tcp_ip6_spec.pdst;
			input->src_port = fsp->h_u.tcp_ip6_spec.psrc;
		}
		if (input->dst_port) {
			if (fsp->m_u.tcp_ip6_spec.pdst != 0xFFFF) {
				netdev_err(priv->dev,
					   "The port mask is unsupported\n");
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_DPORT;
		}
		if (input->src_port) {
			if (fsp->m_u.tcp_ip6_spec.psrc != 0xFFFF) {
				netdev_err(priv->dev,
					   "The port mask is unsupported\n");
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_SPORT;
		}

		if (ip6_address & DN200_L3L4_IPV6_SA) {
			input->flow_type |= DN200_FLOW_TYPE_SA;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6src[3]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src = mask;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6src[2]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;

			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6src[1]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6src[0]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;
			if (input->xgmac_mask_src >
			    XGMAC_NTUPLE_MAX_V6_ADDR_MASK_BITS)
				netdev_err(priv->dev, "Mask is not supported at the highest bit\n");
			memcpy(input->ip6, fsp->h_u.tcp_ip6_spec.ip6src,
			       sizeof(__be32) * 4);
			memcpy(input->ip6_mask, fsp->m_u.tcp_ip6_spec.ip6src,
			       sizeof(__be32) * 4);
		} else if (ip6_address & DN200_L3L4_IPV6_DA) {
			input->flow_type |= DN200_FLOW_TYPE_DA;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6dst[3]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src = mask;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6dst[2]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;

			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6dst[1]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;
			mask = dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(fsp->m_u.tcp_ip6_spec.ip6dst[0]));
			if (mask == -1)
				return -EINVAL;
			input->xgmac_mask_src += mask;
			if (input->xgmac_mask_src >
			    XGMAC_NTUPLE_MAX_V6_ADDR_MASK_BITS)
				netdev_err(priv->dev, "Mask is not supported at the highest bit\n");
			memcpy(input->ip6, fsp->h_u.tcp_ip6_spec.ip6dst,
			       sizeof(__be32) * 4);
			memcpy(input->ip6_mask, fsp->m_u.tcp_ip6_spec.ip6dst,
			       sizeof(__be32) * 4);
		}
	} else {
		input->flow_type |= DN200_FLOW_TYPE_V4;
		if (fsp->flow_type == IPV4_USER_FLOW) {
			if (fsp->h_u.usr_ip4_spec.l4_4_bytes) {
				netdev_err(priv->dev, "Ipv4 User spec not support L4 config\n");
				return -EOPNOTSUPP;
			}
		} else {
			input->dst_port = fsp->h_u.tcp_ip4_spec.pdst;
			input->src_port = fsp->h_u.tcp_ip4_spec.psrc;
		}
		input->reg_idx = fsp->location;
		input->dst_ip = (fsp->h_u.tcp_ip4_spec.ip4dst);
		input->dst_ip_mask = (fsp->m_u.tcp_ip4_spec.ip4dst);
		input->src_ip = (fsp->h_u.tcp_ip4_spec.ip4src);
		input->src_ip_mask = (fsp->m_u.tcp_ip4_spec.ip4src);
		if (input->dst_port) {
			if (fsp->m_u.tcp_ip4_spec.pdst != 0xFFFF) {
				netdev_err(priv->dev,
					   "The port mask is unsupported\n");
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_DPORT;
		}
		if (input->src_port) {
			if (fsp->m_u.tcp_ip4_spec.psrc != 0xFFFF) {
				netdev_err(priv->dev,
					   "The port mask is unsupported\n");
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_SPORT;
		}
		if (input->dst_ip) {
			input->xgmac_mask_dst =
				dn200_ethtool_mask_to_xgmac_mask(priv, ntohl(input->dst_ip_mask));
			if (input->xgmac_mask_dst >
				    XGMAC_NTUPLE_MAX_V4_ADDR_MASK_BITS ||
					input->xgmac_mask_dst == -1) {
				if (input->xgmac_mask_dst >
				    XGMAC_NTUPLE_MAX_V4_ADDR_MASK_BITS)
					netdev_err(priv->dev, "Mask is not supported at the highest bit\n");
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_DA;
		}
		if (input->src_ip) {
			input->xgmac_mask_src =
			    dn200_ethtool_mask_to_xgmac_mask(priv,
							     ntohl(input->src_ip_mask));
			if (input->xgmac_mask_src > XGMAC_NTUPLE_MAX_V4_ADDR_MASK_BITS
			    || input->xgmac_mask_src == -1) {
				if (input->xgmac_mask_src >
				    XGMAC_NTUPLE_MAX_V4_ADDR_MASK_BITS) {
					netdev_err(priv->dev, "Mask is not supported at the highest bit\n");
				}
				return -EINVAL;
			}
			input->flow_type |= DN200_FLOW_TYPE_SA;
		}
	}
	if (fsp->flow_type == UDP_V6_FLOW ||
	    fsp->flow_type == TCP_V6_FLOW ||
	    fsp->flow_type == UDP_V4_FLOW || fsp->flow_type == TCP_V4_FLOW) {
		if (!(input->flow_type & (DN200_FLOW_TYPE_DPORT |
				  DN200_FLOW_TYPE_SPORT))) {
			netdev_err(priv->dev,
				   "L4 requires src-port/dst-port configuration\n");
			return -EINVAL;
		}
	}
	ret = dn200_disallow_matching_filters(priv, input);
	if (ret) {
		memset(input, 0, sizeof(*input));
		return ret;
	}
	input->action = action;
	input->queue = queue;
	ret = dn200_config_ntuple_filter(priv, priv->hw, fsp->location, input,
				       true);
	if (ret) {
		memset(input, 0, sizeof(*input));
		return ret;
	}
	if (action == DN200_FLOW_ACTION_DROP)
		priv->fdir_map |= (1 << fsp->location);

	if (input->flow_type & DN200_FLOW_TYPE_UDP)
		priv->fdir_info.l4_udp_count++;
	else if (input->flow_type & DN200_FLOW_TYPE_TCP)
		priv->fdir_info.l4_tcp_count++;

	input->enable = true;
	priv->fdir_counts++;
	return 0;
}

static int dn200_del_fdir_ethtool(struct dn200_priv *priv,
				  struct ethtool_rxnfc *cmd)
{
	struct ethtool_rx_flow_spec *fsp;
	struct dn200_fdir_filter *input;
	int ret = 0;

	fsp = (struct ethtool_rx_flow_spec *)&cmd->fs;

	/* Don't allow indexes to exist outside of available space */
	if (fsp->location >= (priv->flow_entries_max - 4)) {
		netdev_err(priv->dev, "Location out of range\n");
		return -EINVAL;
	}

	input = &priv->fdir_enties[fsp->location];
	if (!input->enable) {
		netdev_err(priv->dev, "Location %d is not enabled\n",
			   fsp->location);
		return -EINVAL;
	}

	ret = dn200_config_ntuple_filter(priv, priv->hw, fsp->location, input,
				       false);
	if (ret)
		return ret;
	/*Delete the udp/tcp entry count */
	if (input->flow_type & DN200_FLOW_TYPE_UDP)
		priv->fdir_info.l4_udp_count--;
	else if (input->flow_type & DN200_FLOW_TYPE_TCP)
		priv->fdir_info.l4_tcp_count--;

	memset(input, 0, sizeof(*input));
	priv->fdir_map &= ~(1 << fsp->location);
	priv->fdir_counts--;
	return 0;
}

static int dn200_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd)
{
	struct dn200_priv *priv = netdev_priv(netdev);

	if (HW_IS_VF(priv->hw))
		return -EOPNOTSUPP;

	switch (cmd->cmd) {
	case ETHTOOL_SRXFH:
		return dn200_set_rss_hash_opt(priv, cmd);
	case ETHTOOL_SRXCLSRLINS:
		return dn200_add_fdir_ethtool(priv, cmd);
	case ETHTOOL_SRXCLSRLDEL:
		return dn200_del_fdir_ethtool(priv, cmd);
	default:
		return -EOPNOTSUPP;
	}
}

static int dn200_get_module_info(struct net_device *netdev,
				 struct ethtool_modinfo *modinfo)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	int status;
	u8 sff8472_rev, addr_mode;
	bool page_swap = false;

	/*VF do not support to access i2c */
	if (HW_IS_VF(priv->hw) || phy_info->phydev)
		return -EOPNOTSUPP;
	/*SFP module absent */
	if (phy_info->sfp_mod_absent) {
		netdev_err(netdev, "sfp module not present\n");
		return -EOPNOTSUPP;
	}
	/*only fibre interface support get sfp module info */
	if (!phy_info->xpcs || !phy_info->xpcs_sfp_valid) {
		netdev_err(netdev, "XPCS I2C not valid\n");
		return -EOPNOTSUPP;
	}
	/* Check whether we support SFF-8472 or not */
	status = phy_info->phy_ops->read_i2c_eeprom(phy_info,
		DN200_SFF_SFF_8472_COMP, &sff8472_rev);
	if (status)
		return -EIO;

	/* addressing mode is not supported */
	status = phy_info->phy_ops->read_i2c_eeprom(phy_info,
		DN200_SFF_SFF_8472_SWAP, &addr_mode);
	if (status)
		return -EIO;

	if (addr_mode & DN200_SFF_ADDRESSING_MODE) {
		netdev_err(priv->dev,
			   "Address change required to access page 0xA2, but not supported. Please report the module type to the driver maintainers.\n");
		page_swap = true;
	}

	if (sff8472_rev == DN200_SFF_SFF_8472_UNSUP || page_swap ||
	    !(addr_mode & DN200_SFF_DDM_IMPLEMENTED)) {
		/* We have a SFP, but it does not support SFF-8472 */
		modinfo->type = ETH_MODULE_SFF_8079;
		modinfo->eeprom_len = ETH_MODULE_SFF_8079_LEN;
	} else {
		/* We have a SFP which supports a revision of SFF-8472. */
		modinfo->type = ETH_MODULE_SFF_8472;
		modinfo->eeprom_len = ETH_MODULE_SFF_8472_LEN;
	}

	return 0;
}

static int dn200_get_module_eeprom(struct net_device *netdev,
				   struct ethtool_eeprom *ee, u8 *data)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	int status = 0;
	u8 databyte = 0xFF;
	int i = 0;

	/*VF do not support access i2c */
	if (HW_IS_VF(priv->hw) || phy_info->phydev)
		return -EOPNOTSUPP;
	/*SFP module absent */
	if (phy_info->sfp_mod_absent) {
		netdev_err(netdev, "sfp module not present\n");
		return -EOPNOTSUPP;
	}
	/*only fibre interface support get sfp module info */
	if (!phy_info->xpcs || !phy_info->xpcs_sfp_valid) {
		netdev_err(netdev, "XPCS I2C not valid\n");
		return -EOPNOTSUPP;
	}

	for (i = ee->offset; i < ee->offset + ee->len; i++) {
		/* I2C reads can take long time */
		if (test_bit(DN200_SFP_IN_INIT, &priv->state))
			return -EBUSY;

		if (i < ETH_MODULE_SFF_8079_LEN)
			status = phy_info->phy_ops->read_i2c_eeprom(phy_info, i,
							       &databyte);
		else
			status = phy_info->phy_ops->read_i2c_sff8472(phy_info, i,
								&databyte);

		if (status)
			return -EIO;

		data[i - ee->offset] = databyte;
	}
	return 0;
}

static void dn200_get_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	wol->wolopts = 0;
	wol->supported = 0;
}

static int dn200_set_wol(struct net_device *netdev, struct ethtool_wolinfo *wol)
{
	return -EOPNOTSUPP;
}

#define DN200_EEPROM_SIZE 512
static int dn200_get_eeprom_len(struct net_device *netdev)
{
	return DN200_EEPROM_SIZE;
}

static int dn200_get_eeprom(struct net_device *netdev,
		struct ethtool_eeprom *eeprom, u8 *bytes)
{
	int ret;
	u8 *data;
	struct pci_dev *pdev;
	struct dn200_priv *priv = netdev_priv(netdev);

	if (!eeprom->len || eeprom->offset + eeprom->len > DN200_EEPROM_SIZE)
		return -EINVAL;

	pdev = container_of(priv->device, struct pci_dev, dev);
	eeprom->magic = pdev->vendor | (pdev->device << 16);

	data = kmalloc(DN200_EEPROM_SIZE, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = dn200_eeprom_read(&priv->plat_ex->ctrl, 0, DN200_EEPROM_SIZE, data);
	if (!ret)
		memcpy(bytes, data + eeprom->offset, eeprom->len);

	kfree(data);

	return ret;
}

static int dn200_set_eeprom(struct net_device *netdev,
		struct ethtool_eeprom *eeprom, u8 *bytes)
{
	int ret;
	u8 *data;
	struct pci_dev *pdev;
	struct dn200_priv *priv = netdev_priv(netdev);

	if (!eeprom->len || eeprom->offset + eeprom->len > DN200_EEPROM_SIZE)
		return -EINVAL;

	pdev = container_of(priv->device, struct pci_dev, dev);
	if (eeprom->magic != (pdev->vendor | (pdev->device << 16)))
		return -EFAULT;

	data = kmalloc(DN200_EEPROM_SIZE, GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = dn200_eeprom_read(&priv->plat_ex->ctrl, 0, DN200_EEPROM_SIZE, data);
	if (ret) {
		kfree(data);
		return ret;
	}

	memcpy(data + eeprom->offset, bytes, eeprom->len);

	ret = dn200_eeprom_write(&priv->plat_ex->ctrl, 0, DN200_EEPROM_SIZE, data);
	kfree(data);

	return ret;
}

static const struct ethtool_ops dn200_ethtool_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
	    ETHTOOL_COALESCE_MAX_FRAMES |
	    ETHTOOL_COALESCE_USE_ADAPTIVE_RX | ETHTOOL_COALESCE_USE_ADAPTIVE_TX,
	.get_drvinfo = dn200_ethtool_getdrvinfo,
	.get_msglevel = dn200_ethtool_getmsglevel,
	.set_msglevel = dn200_ethtool_setmsglevel,
	.get_regs = dn200_ethtool_gregs,
	.get_regs_len = dn200_ethtool_get_regs_len,
	.get_link = ethtool_op_get_link,
	.nway_reset = dn200_nway_reset,
	.get_ringparam = dn200_get_ringparam,
	.set_ringparam = dn200_set_ringparam,
	.get_pauseparam = dn200_get_pauseparam,
	.set_pauseparam = dn200_set_pauseparam,
	.self_test = dn200_selftest_run,
	.set_phys_id = dn200_set_phys_id,
	.get_ethtool_stats = dn200_get_ethtool_stats,
	.get_strings = dn200_get_strings,
	.get_eee = dn200_ethtool_op_get_eee,
	.set_eee = dn200_ethtool_op_set_eee,
	.get_sset_count = dn200_get_sset_count,
	.get_rxnfc = dn200_get_rxnfc,
	.set_rxnfc = dn200_set_rxnfc,
	.get_rxfh_key_size = dn200_get_rxfh_key_size,
	.get_rxfh_indir_size = dn200_get_rxfh_indir_size,
	.get_rxfh = dn200_get_rxfh,
	.set_rxfh = dn200_set_rxfh,
	.get_ts_info = dn200_get_ts_info,
	.get_coalesce = dn200_get_coalesce,
	.set_coalesce = dn200_set_coalesce,
	.get_per_queue_coalesce = dn200_get_per_queue_coalesce,
	.set_per_queue_coalesce = dn200_set_per_queue_coalesce,
	.get_channels = dn200_get_channels,
	.set_channels = dn200_set_channels,
	.get_priv_flags = dn200_get_priv_flags,
	.set_priv_flags = dn200_set_priv_flags,
	.get_link_ksettings = dn200_ethtool_get_link_ksettings,
	.set_link_ksettings = dn200_ethtool_set_link_ksettings,
	.get_module_info = dn200_get_module_info,
	.get_module_eeprom = dn200_get_module_eeprom,
	.flash_device = dn200_load_firmware,
	.get_wol = dn200_get_wol,
	.set_wol = dn200_set_wol,
	.get_eeprom_len = dn200_get_eeprom_len,
	.get_eeprom = dn200_get_eeprom,
	.set_eeprom = dn200_set_eeprom,
};

void dn200_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &dn200_ethtool_ops;
}
