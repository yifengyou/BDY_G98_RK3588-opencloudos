/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/phy.h>
#include <linux/module.h>
#define DN200_VLAN_TAG_USED
#include <linux/if_vlan.h>
#include "dn200_cfg.h"
#include "descs.h"
#include "hwif.h"
#include "mmc.h"

#define DN200_DCB_FEATURE_DISABLE	0
/*Total xmgac number*/
#define XGE_NUM	4

/* Chip Core versions */
#define	DWMAC_CORE_3_40		0x34
#define	DWMAC_CORE_3_50		0x35
#define	DWMAC_CORE_4_00		0x40
#define DWMAC_CORE_4_10		0x41
#define DWMAC_CORE_5_00		0x50
#define DWMAC_CORE_5_10		0x51
#define DWMAC_CORE_5_20		0x52
#define DWXGMAC_CORE_2_10	0x21
#define DWXLGMAC_CORE_2_00	0x20

/* Device ID */
#define DWXGMAC_ID		0x76
#define DWXLGMAC_ID		0x27

/* TX and RX Descriptor Length, these need to be power of two.
 * TX descriptor length less than 64 may cause transmit queue timed out error.
 * RX descriptor length less than 64 may cause inconsistent Rx chain error.
 */
#define DMA_MIN_TX_SIZE		64
#define DMA_MAX_TX_SIZE		4096
#define DMA_DEFAULT_TX_SIZE	2048
#define DMA_MIN_RX_SIZE		64
#define DMA_MAX_RX_SIZE \
	8192 // 16 rings, 8MB pkt buf per ring(2KB * 8192 * 2), total cost 256MB per port
#define DMA_DEFAULT_RX_SIZE	2048
#define DMA_DEFAULT_VF_RX_SIZE	512
#define DN200_GET_ENTRY(x, size) (((x) + 1) & ((size) - 1))
#define DN200_GET_PREVENTRY(x, size) (((x) + (size) - 1) & ((size) - 1))

#undef FRAME_FILTER_DEBUG
/* #define FRAME_FILTER_DEBUG */

/* Error messages */
#define DN200_PCIE_BAR_ERR "Access PCIe Bar failed! Please check HW or PCIe!"

#define DN200_FW_ERR_MSG "ADMIN COMMAND failed! Please check FW!"

enum DRV_TYPE {
	DRV_PURE_PF = 0,
	DRV_SRIOV_PF,
	DRV_VF,

	DRV_TYPE_MAX
};

struct dn200_txq_stats {
	u64 tx_pkt_n;
	u64 tx_normal_irq_n;
};

struct dn200_rxq_stats {
	u64 rx_pkt_n;
	u64 rx_normal_irq_n;
};

/* Extra statistic and debug information exposed by ethtool */
struct dn200_extra_stats {
	/* Transmit errors */
	u64 tx_underflow ____cacheline_aligned;
	u64 tx_carrier;
	u64 tx_losscarrier;
	u64 vlan_tag;
	u64 tx_deferred;
	u64 tx_vlan;
	u64 tx_jabber;
	u64 tx_frame_flushed;
	u64 tx_payload_error;
	u64 tx_ip_header_error;
	/* Receive errors */
	u64 rx_desc;
	u64 sa_filter_fail;
	u64 overflow_error;
	u64 ipc_csum_error;
	u64 rx_collision;
	u64 rx_crc_errors;
	u64 dribbling_bit;
	u64 rx_length;
	u64 rx_mii;
	u64 rx_multicast;
	u64 rx_gmac_overflow;
	u64 rx_watchdog;
	u64 da_rx_filter_fail;
	u64 sa_rx_filter_fail;
	u64 rx_missed_cntr;
	u64 rx_overflow_cntr;
	u64 rx_vlan;
	u64 rx_split_hdr_pkt_n;
	u64 rx_csum_err;
	/* Tx/Rx IRQ error info */
	u64 tx_undeflow_irq;
	u64 tx_process_stopped_irq;
	u64 tx_jabber_irq;
	u64 rx_overflow_irq;
	u64 rx_buf_unav_irq;
	u64 rx_process_stopped_irq;
	u64 rx_watchdog_irq;
	u64 tx_early_irq;
	u64 fatal_bus_error_irq;
	/* Tx/Rx IRQ Events */
	u64 rx_early_irq;
	u64 tx_pkt_n;
	u64 rx_pkt_n;
	u64 normal_irq_n;
	u64 rx_normal_irq_n;
	u64 napi_poll;
	u64 tx_normal_irq_n;
	u64 tx_clean;
	u64 tx_set_ic_bit;
	u64 irq_receive_pmt_irq_n;
	/* MMC info */
	u64 mmc_tx_irq_n;
	u64 mmc_rx_irq_n;
	u64 mmc_rx_csum_offload_irq_n;
	/* EEE */
	u64 irq_tx_path_in_lpi_mode_n;
	u64 irq_tx_path_exit_lpi_mode_n;
	u64 irq_rx_path_in_lpi_mode_n;
	u64 irq_rx_path_exit_lpi_mode_n;
	u64 phy_eee_wakeup_error_n;
	/* Extended RDES status */
	u64 ip_hdr_err;
	u64 ip_payload_err;
	u64 ip_csum_bypassed;
	u64 ipv4_pkt_rcvd;
	u64 ipv6_pkt_rcvd;
	u64 no_ptp_rx_msg_type_ext;
	u64 ptp_rx_msg_type_sync;
	u64 ptp_rx_msg_type_follow_up;
	u64 ptp_rx_msg_type_delay_req;
	u64 ptp_rx_msg_type_delay_resp;
	u64 ptp_rx_msg_type_pdelay_req;
	u64 ptp_rx_msg_type_pdelay_resp;
	u64 ptp_rx_msg_type_pdelay_follow_up;
	u64 ptp_rx_msg_type_announce;
	u64 ptp_rx_msg_type_management;
	u64 ptp_rx_msg_pkt_reserved_type;
	u64 ptp_frame_type;
	u64 ptp_ver;
	u64 timestamp_dropped;
	u64 av_pkt_rcvd;
	u64 av_tagged_pkt_rcvd;
	u64 vlan_tag_priority_val;
	u64 l3_filter_match;
	u64 l4_filter_match;
	u64 l3_l4_filter_no_match;
	/* PCS */
	u64 irq_pcs_ane_n;
	u64 irq_pcs_link_n;
	u64 irq_rgmii_n;
	u64 pcs_link;
	u64 pcs_duplex;
	u64 pcs_speed;
	/* debug register */
	u64 mtl_tx_status_fifo_full;
	u64 mtl_tx_fifo_not_empty;
	u64 mmtl_fifo_ctrl;
	u64 mtl_tx_fifo_read_ctrl_write;
	u64 mtl_tx_fifo_read_ctrl_wait;
	u64 mtl_tx_fifo_read_ctrl_read;
	u64 mtl_tx_fifo_read_ctrl_idle;
	u64 mac_tx_in_pause;
	u64 mac_tx_frame_ctrl_xfer;
	u64 mac_tx_frame_ctrl_idle;
	u64 mac_tx_frame_ctrl_wait;
	u64 mac_tx_frame_ctrl_pause;
	u64 mac_gmii_tx_proto_engine;
	u64 mtl_rx_fifo_fill_level_full;
	u64 mtl_rx_fifo_fill_above_thresh;
	u64 mtl_rx_fifo_fill_below_thresh;
	u64 mtl_rx_fifo_fill_level_empty;
	u64 mtl_rx_fifo_read_ctrl_flush;
	u64 mtl_rx_fifo_read_ctrl_read_data;
	u64 mtl_rx_fifo_read_ctrl_status;
	u64 mtl_rx_fifo_read_ctrl_idle;
	u64 mtl_rx_fifo_ctrl_active;
	u64 mac_rx_frame_ctrl_fifo;
	u64 mac_gmii_rx_proto_engine;
	/* TSO */
	u64 tx_tso_frames;
	u64 tx_tso_nfrags;
	/* EST */
	u64 mtl_est_cgce;
	u64 mtl_est_hlbs;
	u64 mtl_est_hlbf;
	u64 mtl_est_btre;
	u64 mtl_est_btrlm;

	/* PF/VF Events */
	u64 rst_start_count;	/* reset start noitify count */
	u64 rst_finish_count;	/* reset finish notify count */
	u64 rst_start_ok_count;
	u64 rst_finish_ok_count;
	u64 rst_start_accept_count;
	u64 rst_finish_accept_count;
	u64 normal_rst_count;
	u64 tx_timeout_rst_count;
	u64 dma_chan_err_rst_count;

	u64 tx_frames_129_to_256;
	u64 tx_frames_65_to_128;
	u64 tx_frames_33_to_64;
	u64 tx_frames_17_to_32;
	u64 tx_frames_16_below;
	/* per queue statistics */
	struct dn200_txq_stats txq_stats[MTL_MAX_TX_QUEUES];
	struct dn200_rxq_stats rxq_stats[MTL_MAX_RX_QUEUES];
};

/* Safety Feature statistics exposed by ethtool */
struct dn200_safety_stats {
	u64 mac_errors[32];
	u64 mtl_errors[32];
	u64 dma_errors[32];
};

#define DN200_FLOW_ACTION_DROP		BIT(0)
#define DN200_FLOW_ACTION_ROUTE	BIT(1)

#define DN200_FLOW_TYPE_V4 BIT(0)
#define DN200_FLOW_TYPE_V6 BIT(1)
#define DN200_FLOW_TYPE_SA BIT(2)
#define DN200_FLOW_TYPE_DA BIT(3)
#define DN200_FLOW_TYPE_UDP BIT(4)
#define DN200_FLOW_TYPE_TCP BIT(5)
#define DN200_FLOW_TYPE_DPORT BIT(6)
#define DN200_FLOW_TYPE_SPORT BIT(7)

struct dn200_fdir_info {
	u32 l4_udp_count;
	u32 l4_tcp_count;
};

struct dn200_fdir_filter {
	/* filter ipnut set */
	u8 flow_type;
	/*enable */
	bool enable;
	/*action */
	u8 action;
	/*route dst */
	u8 queue;
	/*reg idx */
	u8 reg_idx;
	/* TX packet view of src and dst */
	u32 dst_ip;
	u32 dst_ip_mask;
	int xgmac_mask_dst;
	u32 src_ip;
	u32 src_ip_mask;
	int xgmac_mask_src;
	/*ip6 */
#define DN200_L3L4_IPV6_SA	BIT(1)
#define DN200_L3L4_IPV6_DA	BIT(2)
	u8 ip6_address;
	u32 ip6[4];
	u32 ip6_mask[4];
	u16 src_port;
	u16 dst_port;
};

/* Number of fields in Safety Stats */
#define DN200_SAFETY_FEAT_SIZE	\
	(sizeof(struct dn200_safety_stats) / sizeof(u64))

/* CSR Frequency Access Defines*/
#define CSR_F_35M	35000000
#define CSR_F_60M	60000000
#define CSR_F_100M	100000000
#define CSR_F_150M	150000000
#define CSR_F_250M	250000000
#define CSR_F_300M	300000000

#define	MAC_CSR_H_FRQ_MASK	0x20

#define HASH_TABLE_SIZE 64
#define PAUSE_TIME 0xffff

/* Flow Control defines */
#define FLOW_OFF	0
#define FLOW_RX		1
#define FLOW_TX		2
#define FLOW_AUTO	(FLOW_OFF)

/* PCS defines */
#define DN200_PCS_RGMII	BIT(0)
#define DN200_PCS_SGMII	BIT(1)
#define DN200_PCS_TBI	BIT(2)
#define DN200_PCS_RTBI	BIT(3)

#define SF_DMA_MODE 1		/* DMA STORE-AND-FORWARD Operation Mode */

/* DAM HW feature register fields */
#define DMA_HW_FEAT_MIISEL	0x00000001	/* 10/100 Mbps Support */
#define DMA_HW_FEAT_GMIISEL	0x00000002	/* 1000 Mbps Support */
#define DMA_HW_FEAT_HDSEL	0x00000004	/* Half-Duplex Support */
#define DMA_HW_FEAT_EXTHASHEN	0x00000008	/* Expanded DA Hash Filter */
#define DMA_HW_FEAT_HASHSEL	0x00000010	/* HASH Filter */
#define DMA_HW_FEAT_ADDMAC	0x00000020	/* Multiple MAC Addr Reg */
#define DMA_HW_FEAT_PCSSEL	0x00000040	/* PCS registers */
#define DMA_HW_FEAT_L3L4FLTREN	0x00000080	/* Layer 3 & Layer 4 Feature */
#define DMA_HW_FEAT_SMASEL	0x00000100	/* SMA(MDIO) Interface */
#define DMA_HW_FEAT_RWKSEL	0x00000200	/* PMT Remote Wakeup */
#define DMA_HW_FEAT_MGKSEL	0x00000400	/* PMT Magic Packet */
#define DMA_HW_FEAT_MMCSEL	0x00000800	/* RMON Module */
#define DMA_HW_FEAT_TSVER1SEL	0x00001000	/* Only IEEE 1588-2002 */
#define DMA_HW_FEAT_TSVER2SEL	0x00002000	/* IEEE 1588-2008 PTPv2 */
#define DMA_HW_FEAT_EEESEL	0x00004000	/* Energy Efficient Ethernet */
#define DMA_HW_FEAT_AVSEL	0x00008000	/* AV Feature */
#define DMA_HW_FEAT_TXCOESEL	0x00010000	/* Checksum Offload in Tx */
#define DMA_HW_FEAT_RXTYP1COE	0x00020000	/* IP COE (Type 1) in Rx */
#define DMA_HW_FEAT_RXTYP2COE	0x00040000	/* IP COE (Type 2) in Rx */
#define DMA_HW_FEAT_RXFIFOSIZE	0x00080000	/* Rx FIFO > 2048 Bytes */
#define DMA_HW_FEAT_RXCHCNT	0x00300000	/* No. additional Rx Channels */
#define DMA_HW_FEAT_TXCHCNT	0x00c00000	/* No. additional Tx Channels */
#define DMA_HW_FEAT_ENHDESSEL	0x01000000	/* Alternate Descriptor */
/* Timestamping with Internal System Time */
#define DMA_HW_FEAT_INTTSEN	0x02000000
#define DMA_HW_FEAT_FLEXIPPSEN	0x04000000	/* Flexible PPS Output */
#define DMA_HW_FEAT_SAVLANINS	0x08000000	/* Source Addr or VLAN */
#define DMA_HW_FEAT_ACTPHYIF	0x70000000	/* Active/selected PHY iface */
#define DEFAULT_DMA_PBL		8

/* MSI defines */
#define DN200_MSI_VEC_MAX	32

/* PCS status and mask defines */
#define	PCS_ANE_IRQ		BIT(2)	/* PCS Auto-Negotiation */
#define	PCS_LINK_IRQ		BIT(1)	/* PCS Link */
#define	PCS_RGSMIIIS_IRQ	BIT(0)	/* RGMII or SMII Interrupt */

/* Max/Min RI Watchdog Timer count value */
#define MAX_DMA_RIWT	0xff
#define MIN_DMA_RIWT	0x01	/* min is 500 ns, but the CPU loading will be higher */
/* 112 us, frequency is 500MHz,
 * total us = (1000 000/500 000 000) * 512 * 0x6E = 112us
 */
#define DEF_DMA_RIWT	0x6E
/* Tx coalesce parameters */
/* for 10% of 10G 64byte RFC2544, default ring size 512,
 * total us (1 * 1000000) * 512 / (1480000) ~= 345us
 */
#define DN200_COAL_TX_TIMER	200
#define DN200_MAX_COAL_TX_TICK	100000
#define DN200_MAX_COAL_RX_TICK	8160
#define DN200_MIN_CLAL_TX_TIME	10
#define DN200_TX_MAX_FRAMES	256
#define DN200_TX_FRAMES \
	64 /*change from 25 to 14, as the tx performace is too lower is 3.14 kernel */

/* Rx coalesce parameters */
#define DN200_RX_MAX_FRAMES	256
#define DN200_RX_FRAMES		48
#define DN200_RX_MIN_FRAMES	16
#define DN200_RX_MAX_REFILL_SIZE	(64)
/* Packets types */
enum packets_types {
	PACKET_AVCPQ = 0x1,	/* AV Untagged Control packets */
	PACKET_PTPQ = 0x2,	/* PTP Packets */
	PACKET_DCBCPQ = 0x3,	/* DCB Control Packets */
	PACKET_UPQ = 0x4,	/* Untagged Packets */
	PACKET_MCBCQ = 0x5,	/* Multicast & Broadcast Packets */
};

/* Rx IPC status */
enum rx_frame_status {
	good_frame = 0x0,
	discard_frame = BIT(0),
	csum_none = BIT(1),
	llc_snap = BIT(2),
	dma_own = BIT(3),
	rx_not_ls = BIT(4),
	buf_len_err = BIT(5),
};

/* Tx status */
enum tx_frame_status {
	tx_done = 0x0,
	tx_not_ls = 0x1,
	tx_err = 0x2,
	tx_dma_own = 0x4,
};

enum rx_frame_err_types {
	watchdog_timeout_err = 0x1,
	invalid_code_err = 0x2,
	crc_err = 0x3,
	giant_packet_err = 0x4,
	ip_header_err = 0x5,
	l4_chksum_err = 0x6,
	overflow_err = 0x7,
	bus_err = 0x8,
	length_err = 0x9,
	good_runt_packet_err = 0xa,
	dribble_err = 0xc,
	safety_err = 0xf,
};

enum tunnel_rx_frame_err_types {
	outer_ip_header_err = 0x5,
	outer_l4_chksum_err = 0x6,
	inner_ip_header_err = 0x9,
	inner_l4_chksum_err = 0xa,
	invalid_tunnel_header_field = 0xb,
};

enum dma_irq_status {
	tx_hard_error = 0x1,
	tx_hard_error_bump_tc = 0x2,
	handle_rx = 0x4,
	handle_tx = 0x8,
};

enum dma_irq_dir {
	DMA_DIR_RX = 0x1,
	DMA_DIR_TX = 0x2,
	DMA_DIR_RXTX = 0x3,
};

enum request_irq_err {
	REQ_IRQ_ERR_ALL,
	REQ_IRQ_ERR_RXTX,
	REQ_IRQ_ERR_TX,
	REQ_IRQ_ERR_RX,
	REQ_IRQ_ERR_SFTY_UE,
	REQ_IRQ_ERR_SFTY_CE,
	REQ_IRQ_ERR_LPI,
	REQ_IRQ_ERR_MAC,
	REQ_IRQ_ERR_NO,
};

/* EEE and LPI defines */
#define	CORE_IRQ_TX_PATH_IN_LPI_MODE	BIT(0)
#define	CORE_IRQ_TX_PATH_EXIT_LPI_MODE	BIT(1)
#define	CORE_IRQ_RX_PATH_IN_LPI_MODE	BIT(2)
#define	CORE_IRQ_RX_PATH_EXIT_LPI_MODE	BIT(3)

/* FPE defines */
#define FPE_EVENT_UNKNOWN		0
#define FPE_EVENT_TRSP			BIT(0)
#define FPE_EVENT_TVER			BIT(1)
#define FPE_EVENT_RRSP			BIT(2)
#define FPE_EVENT_RVER			BIT(3)

#define CORE_IRQ_MTL_RX_OVERFLOW	BIT(8)

/* Physical Coding Sublayer */
struct rgmii_adv {
	unsigned int pause;
	unsigned int duplex;
	unsigned int lp_pause;
	unsigned int lp_duplex;
};

#define DN200_PCS_PAUSE	1
#define DN200_PCS_ASYM_PAUSE	2

/* DMA HW capabilities */
struct dma_features {
	unsigned int mbps_10_100;
	unsigned int mbps_1000;
	unsigned int half_duplex;
	unsigned int hash_filter;
	unsigned int multi_addr;
	unsigned int pcs;
	unsigned int sma_mdio;
	unsigned int pmt_remote_wake_up;
	unsigned int pmt_magic_frame;
	unsigned int rmon;
	/* IEEE 1588-2002 */
	unsigned int time_stamp;
	/* IEEE 1588-2008 */
	unsigned int atime_stamp;
	/* 802.3az - Energy-Efficient Ethernet (EEE) */
	unsigned int eee;
	unsigned int av;
	unsigned int hash_tb_sz;
	unsigned int tsoen;
	unsigned int dcben;
	/* TX and RX csum */
	unsigned int tx_coe;
	unsigned int rx_coe;
	unsigned int rx_coe_type1;
	unsigned int rx_coe_type2;
	unsigned int rxfifo_over_2048;
	/* TX and RX number of channels */
	unsigned int number_rx_channel;
	unsigned int number_tx_channel;
	/* TX and RX number of queues */
	unsigned int number_rx_queues;
	unsigned int number_tx_queues;
	/* PPS output */
	unsigned int pps_out_num;
	/* Alternate (enhanced) DESC mode */
	unsigned int enh_desc;
	/* TX and RX FIFO sizes */
	unsigned int tx_fifo_size;
	unsigned int rx_fifo_size;
	/* Automotive Safety Package */
	unsigned int asp;
	/* RX Parser */
	unsigned int frpsel;
	unsigned int frpbs;
	unsigned int frpes;
	unsigned int addr64;
	unsigned int rssen;
	unsigned int vlhash;
	unsigned int sphen;
	unsigned int vlins;
	unsigned int dvlan;
	unsigned int l3l4fnum;
	unsigned int arpoffsel;
	/* TSN Features */
	unsigned int estwid;
	unsigned int estdep;
	unsigned int estsel;
	unsigned int fpesel;
	unsigned int tbssel;
	/* Numbers of Auxiliary Snapshot Inputs */
	unsigned int aux_snapshot_n;
	/* DCB */
	unsigned int tc_cnt;
};

/* RX Buffer size must be multiple of 4/8/16 bytes */
#define BUF_SIZE_16KiB 16368
#define BUF_SIZE_8KiB 8188
#define BUF_SIZE_4KiB 4096
#define BUF_SIZE_3KiB 3072
#define BUF_SIZE_2KiB 2048

/* Common MAC defines */
#define MAC_CTRL_REG		0x00000000	/* MAC Control */
#define MAC_ENABLE_TX		0x00000008	/* Transmitter Enable */
#define MAC_ENABLE_RX		0x00000004	/* Receiver Enable */

/* Default LPI timers */
#define DN200_DEFAULT_LIT_LS	0x3E8
#define DN200_DEFAULT_TWT_LS	0x1E
#define DN200_ET_MAX		0xFFFFF

#define DN200_CHAIN_MODE	0x1
#define DN200_RING_MODE	0x2

#define JUMBO_LEN		9000

/* Receive Side Scaling */
#define DN200_RSS_HASH_KEY_SIZE	40
#define DN200_RSS_MAX_TABLE_SIZE	256

/* VLAN */
#define DN200_VLAN_NONE	0x0
#define DN200_VLAN_REMOVE	0x1
#define DN200_VLAN_INSERT	0x2
#define DN200_VLAN_REPLACE	0x3

/* TSO Desc flag */
#define TSO_DESC_IS_FIRST	0x1
#define TSO_DESC_IS_TUNNEL	0x2

extern const struct dn200_desc_ops enh_desc_ops;
extern const struct dn200_desc_ops ndesc_ops;

struct mac_device_info;

extern const struct dn200_hwtimestamp dn200_ptp;

struct mac_link {
	u32 speed_mask;
	u32 speed10;
	u32 speed100;
	u32 speed1000;
	u32 speed2500;
	u32 duplex;
	struct {
		u32 speed2500;
		u32 speed5000;
		u32 speed10000;
	} xgmii;
	struct {
		u32 speed25000;
		u32 speed40000;
		u32 speed50000;
		u32 speed100000;
	} xlgmii;
};

struct mii_regs {
	unsigned int addr;	/* MII Address */
	unsigned int data;	/* MII Data */
	unsigned int addr_shift;	/* MII address shift */
	unsigned int reg_shift;	/* MII reg shift */
	unsigned int addr_mask;	/* MII address mask */
	unsigned int reg_mask;	/* MII reg mask */
	unsigned int clk_csr_shift;
	unsigned int clk_csr_mask;
};

struct dn200_set_state {
	bool is_promisc;
	bool is_allmuslt;
	int uc_num;
	int mc_num;
};

struct mac_device_info {
	struct dn200_priv *priv;
	const struct dn200_ops *mac;
	const struct dn200_desc_ops *desc;
	const struct dn200_dma_ops *dma;
	const struct dn200_mode_ops *mode;
	const struct dn200_hwtimestamp *ptp;
	const struct dn200_tc_ops *tc;
	const struct dn200_mmc_ops *mmc;
	struct dw_xpcs *xpcs;
	struct mii_regs mii;	/* MII register Addresses */
	struct mac_link link;
	void __iomem *pcsr;	/* vpointer to device CSRs */
	void __iomem *pmail;	/* vpointer to device mailbox */
	unsigned int multicast_filter_bins;
	unsigned int unicast_filter_entries;
	unsigned int mcast_bits_log2;
	unsigned int rx_csum;
	unsigned int pcs;
	unsigned int ps;
	unsigned int xlgmac;
	unsigned int max_vlan_num;
	u32 vlan_filter[32];
	unsigned int promisc;
	bool vlan_fail_q_en;
	u8 vlan_fail_q;
	struct dn200_set_state set_state;
	u64 reconfig_times;
	u32 cfg_rxp_seq;
};

struct dn200_rx_routing {
	u32 reg_mask;
	u32 reg_shift;
};

enum dn200_txrx_mode_dir {
	DN200_SET_NONE = BIT(0),
	DN200_SET_TX_MODE = BIT(1),
	DN200_SET_RX_MODE = BIT(2),
	DN200_SET_TXRX_MODE = (DN200_SET_TX_MODE | DN200_SET_RX_MODE),
};

int dwmac100_setup(struct dn200_priv *priv);
int dwmac1000_setup(struct dn200_priv *priv);
int dwmac4_setup(struct dn200_priv *priv);
int dwxgmac2_setup(struct dn200_priv *priv);
int dpxgmac_setup(struct dn200_priv *priv);
int dwxlgmac2_setup(struct dn200_priv *priv);

void dn200_set_mac_addr(void __iomem *ioaddr, u8 addr[6],
			unsigned int high, unsigned int low);
void dn200_get_mac_addr(void __iomem *ioaddr, unsigned char *addr,
			unsigned int high, unsigned int low);
void dn200_set_mac(void __iomem *ioaddr, bool enable);

void dn200_dwmac4_set_mac_addr(void __iomem *ioaddr, u8 addr[6],
			       unsigned int high, unsigned int low);
void dn200_dwmac4_get_mac_addr(void __iomem *ioaddr, unsigned char *addr,
			       unsigned int high, unsigned int low);
void dn200_dwmac4_set_mac(void __iomem *ioaddr, bool enable);

extern const struct dn200_mode_ops ring_mode_ops;
extern const struct dn200_mode_ops chain_mode_ops;
extern const struct dn200_desc_ops dwmac4_desc_ops;

#endif
