/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __MMC_H__
#define __MMC_H__

/* MMC control register */
/* When set, all counter are reset */
#define MMC_CNTRL_COUNTER_RESET		0x1
/* When set, do not roll over zero after reaching the max value*/
#define MMC_CNTRL_COUNTER_STOP_ROLLOVER	0x2
#define MMC_CNTRL_RESET_ON_READ		0x4	/* Reset after reading */
#define MMC_CNTRL_COUNTER_FREEZER	0x8	/* Freeze counter values to the current value.*/
#define MMC_CNTRL_PRESET		0x10
#define MMC_XGMAC_TX_PKT_GB		0x1c
#define MMC_CNTRL_FULL_HALF_PRESET	0x20

#define MMC_GMAC4_OFFSET		0x700
#define MMC_GMAC3_X_OFFSET		0x100
#define MMC_XGMAC_OFFSET		0x800

#define MMC_XGMAC_RX_PKT_GB		0x100

struct dn200_counters {
	u64 mmc_tx_octetcount_gb;
	u64 mmc_tx_framecount_gb;
	u64 mmc_tx_broadcastframe_g;
	u64 mmc_tx_multicastframe_g;
	u64 mmc_tx_64_octets_gb;
	u64 mmc_tx_65_to_127_octets_gb;
	u64 mmc_tx_128_to_255_octets_gb;
	u64 mmc_tx_256_to_511_octets_gb;
	u64 mmc_tx_512_to_1023_octets_gb;
	u64 mmc_tx_1024_to_max_octets_gb;
	u64 mmc_tx_unicast_gb;
	u64 mmc_tx_multicast_gb;
	u64 mmc_tx_broadcast_gb;
	u64 mmc_tx_underflow_error;
	u64 mmc_tx_octetcount_g;
	u64 mmc_tx_framecount_g;
	u64 mmc_tx_pause_frame;
	u64 mmc_tx_vlan_frame_g;
	u64 mmc_tx_vlan_insert;
	u64 mmc_tx_lpi_usec;
	u64 mmc_tx_lpi_tran;

	/* MMC RX counter registers */
	u64 mmc_rx_framecount_gb;
	u64 mmc_rx_octetcount_gb;
	u64 mmc_rx_octetcount_g;
	u64 mmc_rx_broadcastframe_g;
	u64 mmc_rx_multicastframe_g;
	u64 mmc_rx_crc_error;
	u64 mmc_rx_align_error;
	u32 mmc_rx_run_error;
	u32 mmc_rx_jabber_error;
	u32 mmc_rx_undersize_g;
	u32 mmc_rx_oversize_g;
	u64 mmc_rx_64_octets_gb;
	u64 mmc_rx_65_to_127_octets_gb;
	u64 mmc_rx_128_to_255_octets_gb;
	u64 mmc_rx_256_to_511_octets_gb;
	u64 mmc_rx_512_to_1023_octets_gb;
	u64 mmc_rx_1024_to_max_octets_gb;
	u64 mmc_rx_unicast_g;
	u64 mmc_rx_length_error;
	u64 mmc_rx_outofrangetype;
	u64 mmc_rx_pause_frames;
	u64 mmc_rx_fifo_overflow;
	u64 mmc_rx_vlan_frames_gb;
	u64 mmc_rx_vlan_strip;
	u64 mmc_rx_fd_drop;
	u64 mmc_rx_watchdog_error;
	unsigned int mmc_rx_lpi_usec;
	unsigned int mmc_rx_lpi_tran;
	u64 mmc_rx_discard_pkt_gb;
	u64 mmc_rx_discard_oct_gb;
	unsigned int mmc_rx_align_err;

	/* IPC */
	unsigned int mmc_rx_ipc_intr_mask;
	unsigned int mmc_rx_ipc_intr;
	/* IPv4 */
	u64 mmc_rx_ipv4_gd;
	u64 mmc_rx_ipv4_hderr;
	u64 mmc_rx_ipv4_nopay;
	u64 mmc_rx_ipv4_frag;
	u64 mmc_rx_ipv4_udsbl;

	u64 mmc_rx_ipv4_gd_octets;
	u64 mmc_rx_ipv4_hderr_octets;
	u64 mmc_rx_ipv4_nopay_octets;
	u64 mmc_rx_ipv4_frag_octets;
	u64 mmc_rx_ipv4_udsbl_octets;

	/* IPV6 */
	u64 mmc_rx_ipv6_gd_octets;
	u64 mmc_rx_ipv6_hderr_octets;
	u64 mmc_rx_ipv6_nopay_octets;

	u64 mmc_rx_ipv6_gd;
	u64 mmc_rx_ipv6_hderr;
	u64 mmc_rx_ipv6_nopay;

	/* Protocols */
	u64 mmc_rx_udp_gd;
	u64 mmc_rx_udp_err;
	u64 mmc_rx_tcp_gd;
	u64 mmc_rx_tcp_err;
	u64 mmc_rx_icmp_gd;
	u64 mmc_rx_icmp_err;

	u64 mmc_rx_udp_gd_octets;
	u64 mmc_rx_udp_err_octets;
	u64 mmc_rx_tcp_gd_octets;
	u64 mmc_rx_tcp_err_octets;
	u64 mmc_rx_icmp_gd_octets;
	u64 mmc_rx_icmp_err_octets;

	/* FPE */
	unsigned int mmc_tx_fpe_fragment_cntr;
	unsigned int mmc_tx_hold_req_cntr;
	unsigned int mmc_rx_packet_assembly_err_cntr;
	unsigned int mmc_rx_packet_assembly_ok_cntr;
	unsigned int mmc_rx_fpe_fragment_cntr;
};

struct dn200_swcounters {
	u64 mmc_rx_fd_drop;
	u64 mmc_tx_vlan_insert;
	u64 mmc_rx_vlan_strip;
	u64 rx_mem_copy;
	u64 tx_mem_copy;
	u64 tx_iatu_updt_cnt;
	u64 tx_iatu_match_cnt;
	u64 tx_iatu_find_cnt;
	u64 tx_iatu_recyc_cnt;
	u64 hw_lock_fail_cnt;
	u64 hw_lock_timeout;
	u32 hw_lock_recfgs;
};

void dwxgmac_read_mmc_reg(void __iomem *addr, u32 reg, u64 *dest);

#endif
