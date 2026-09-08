/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UAPI_YK3_H
#define _UAPI_YK3_H
/******************************************************************************/
#include "yk3_common.h"

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <endian.h>

#define kmalloc(size, flag)		malloc(count, size)
#define kzalloc(size, flag)		calloc(1, size)
#define kcalloc(count, size, flag)	calloc(count, size)
#define kfree(ptr)			free(ptr)
#endif /* end __KERNEL__ */
/******************************************************************************/
/* base */

enum yk3_runmode {
	YK3_MODE_TCARD,
	YK3_MODE_ECARD,
	YK3_MODE_RCARD,
	YK3_MODE_PCARD,
	YK3_MODE_MAX,
};

enum yk3_ndev_type {
	YK3_NDEV_T_UPLINK,
	YK3_NDEV_T_PF,
	YK3_NDEV_T_VF,
	YK3_NDEV_T_REP,
	YK3_NDEV_T_SF,
	YK3_NDEV_T_MAX,
};

enum yk3_queue_type {
	YK3_QUEUE_T_LOCAL,  /* ndev queue, ndev : 0 -> num-1 */
	YK3_QUEUE_T_FUNC,   /* func queue, */
	YK3_QUEUE_T_PF,     /* pf queue(pf, vf), */
	YK3_QUEUE_T_GLOBAL, /* global queue */
	YK3_QUEUE_T_MAX,
};

enum yk3_mac_channel {
	YK3_MAC_CH_0,
	YK3_MAC_CH_1,
	YK3_MAC_CH_2,
	YK3_MAC_CH_3,
	YK3_MAC_CH_MAX,
};

enum yk3_chip {
	YK3_K3,
	YK3_K3MAX,

	YK3_CHIP_END
};

static inline const char *
yk3_chip_name(enum yk3_chip chip) {
	return (chip == YK3_K3) ? "k3" : "k3max";
}

enum yk3_arch {
	YK3_ASIC,
	YK3_FPGA,

	YK3_ARCH_END
};

static inline const char *
yk3_arch_name(enum yk3_arch arch) {
	return (arch == YK3_ASIC) ? "asic" : "fpga";
}

struct yk3_queuebase {
	u16 start;
	u16 num;
} __packed;

struct yk3_card_id {
	u16 domain;
	u8  bus;
	u8  devid;
};

struct yk3_bdf {
	u16 domain;
	u8  bus;
	u8  devid;
	u8  function;
	u8  r[3];
};

struct yk3_pci_addr {
	s32 domain;
	u8  bus;
	u8  devid;
	u8  function;
} __packed;

static inline bool
is_good_bdf_domain(unsigned int domain) {
	return domain < 0xffff;
}

static inline bool
is_good_bdf_bus(unsigned int bus) {
	return bus < 0xff;
}

static inline bool
is_good_bdf_devid(unsigned int devid) {
	return devid < 0xff;
}

static inline bool
is_good_bdf_function(unsigned int function) {
	return function < 0xf;
}

static inline bool
is_good_card_id(unsigned int domain, unsigned int bus, unsigned int devid) {
	return is_good_bdf_domain(domain) && is_good_bdf_bus(bus) && is_good_bdf_devid(devid);
}

static inline bool
is_good_bdf(unsigned int domain, unsigned int bus, unsigned int devid, unsigned int function) {
	return is_good_card_id(domain, bus, devid) && is_good_bdf_function(function);
}

#define YK3_BIT64(nr) (1UL << (nr))

#define YK3_RXOLCAP_VLAN_STRIP       YK3_BIT64(0)
#define YK3_RXOLCAP_IPV4_CKSUM       YK3_BIT64(1)
#define YK3_RXOLCAP_UDP_CKSUM        YK3_BIT64(2)
#define YK3_RXOLCAP_TCP_CKSUM        YK3_BIT64(3)
#define YK3_RXOLCAP_TCP_LRO          YK3_BIT64(4)
#define YK3_RXOLCAP_QINQ_STRIP       YK3_BIT64(5)
#define YK3_RXOLCAP_OUTER_IPV4_CKSUM YK3_BIT64(6)
#define YK3_RXOLCAP_MACSEC_STRIP     YK3_BIT64(7)
#define YK3_RXOLCAP_HEADER_SPLIT     YK3_BIT64(8)
#define YK3_RXOLCAP_VLAN_FILTER      YK3_BIT64(9)
#define YK3_RXOLCAP_VLAN_EXTEND      YK3_BIT64(10)
#define YK3_RXOLCAP_SCATTER          YK3_BIT64(13)
#define YK3_RXOLCAP_TIMESTAMP        YK3_BIT64(14)
#define YK3_RXOLCAP_SECURITY         YK3_BIT64(15)
#define YK3_RXOLCAP_KEEP_CRC         YK3_BIT64(16)
#define YK3_RXOLCAP_SCTP_CKSUM       YK3_BIT64(17)
#define YK3_RXOLCAP_OUTER_UDP_CKSUM  YK3_BIT64(18)
#define YK3_RXOLCAP_RSS_HASH         YK3_BIT64(19)
#define YK3_RXOLCAP_BUFFER_SPLIT     YK3_BIT64(20)

#define YK3_TXOLCAP_VLAN_INSERT       YK3_BIT64(0)
#define YK3_TXOLCAP_IPV4_CKSUM        YK3_BIT64(1)
#define YK3_TXOLCAP_UDP_CKSUM         YK3_BIT64(2)
#define YK3_TXOLCAP_TCP_CKSUM         YK3_BIT64(3)
#define YK3_TXOLCAP_SCTP_CKSUM        YK3_BIT64(4)
#define YK3_TXOLCAP_TCP_TSO           YK3_BIT64(5)
#define YK3_TXOLCAP_UDP_TSO           YK3_BIT64(6)
#define YK3_TXOLCAP_OUTER_IPV4_CKSUM  YK3_BIT64(7) /**< Used for tunneling packet. */
#define YK3_TXOLCAP_QINQ_INSERT       YK3_BIT64(8)
#define YK3_TXOLCAP_VXLAN_TNL_TSO     YK3_BIT64(9)  /**< Used for tunneling packet. */
#define YK3_TXOLCAP_GRE_TNL_TSO       YK3_BIT64(10) /**< Used for tunneling packet. */
#define YK3_TXOLCAP_IPIP_TNL_TSO      YK3_BIT64(11) /**< Used for tunneling packet. */
#define YK3_TXOLCAP_GENEVE_TNL_TSO    YK3_BIT64(12) /**< Used for tunneling packet. */
#define YK3_TXOLCAP_MACSEC_INSERT     YK3_BIT64(13)
#define YK3_TXOLCAP_MT_LOCKFREE       YK3_BIT64(14)
#define YK3_TXOLCAP_MULTI_SEGS        YK3_BIT64(15)
#define YK3_TXOLCAP_MBUF_FAST_FREE    YK3_BIT64(16)
#define YK3_TXOLCAP_SECURITY          YK3_BIT64(17)
#define YK3_TXOLCAP_UDP_TNL_TSO       YK3_BIT64(18)
#define YK3_TXOLCAP_IP_TNL_TSO        YK3_BIT64(19)
#define YK3_TXOLCAP_OUTER_UDP_CKSUM   YK3_BIT64(20)
#define YK3_TXOLCAP_SEND_ON_TIMESTAMP YK3_BIT64(21)

struct yk3_net_offloadcap {
	u64 rxsupport;
	u64 txsupport;
	u64 rxconfig;
	u64 txconfig;
} __packed;

/* uapi cmd */
#define YK3_IOCTL_COMM_TYPE  'c'
#define YK3_IOCTL_NET_TYPE   'n'
#define YK3_IOCTL_DOE_TYPE   'd'
#define YK3_IOCTL_QOS_TYPE   'q'
#define YK3_IOCTL_LINK_TYPE  'l'
#define YK3_IOCTL_NP_TYPE    'p'
#define YK3_IOCTL_LAN_TYPE   'a'
#define YK3_IOCTL_METER_TYPE 'm'

/****** common ******/
enum yk3_comm_devtype {
	YK3_COMM_DEVTYPE_PCI = 1,
	YK3_COMM_DEVTYPE_REP,
	YK3_COMM_DEVTYPE_NDEV,
};

struct yk3_comm_devid {
	enum yk3_comm_devtype type;
	union {
		struct yk3_pci_addr pci;
		struct yk3_rep_devid {
			struct yk3_pci_addr pci;
			u16                 rep_id;
		} rep;
		struct yk3_ndev_devid {
			u32 ifindex;
		} ndev;
	};
} __packed;

struct yk3_comm_sysinfo {
	bool iommu;  /* iommu enabled */
	bool domain; /* iommu domain exist */
	bool pt;     /* iommu passthrough */
} __packed;

struct yk3_comm_cardinfo {
	u32              id;
	enum yk3_chip    chip;
	enum yk3_runmode mode;
	int              numa;
	int              qnum;
	int              pf_num;    /* pf number */
	int              vf_maxnum; /* vf max number per pf */
} __packed;

enum {
	YK3_COMM_DEVBIND,
	YK3_COMM_SYSINFO,
	YK3_COMM_CARDINFO, /* should be used after YK3_COMM_DEVBIND */
	YK3_COMM_MAX,
};

#define YK3_COMM_DEV_BIND     _IOW(YK3_IOCTL_COMM_TYPE, YK3_COMM_DEVBIND, struct yk3_comm_devid)
#define YK3_COMM_SYSINFO_GET  _IOR(YK3_IOCTL_COMM_TYPE, YK3_COMM_SYSINFO, struct yk3_comm_sysinfo)
#define YK3_COMM_CARDINFO_GET _IOR(YK3_IOCTL_COMM_TYPE, YK3_COMM_CARDINFO, struct yk3_comm_cardinfo)

/****** net ******/
struct yk3_net_ndevinfo {
	u32                  ifindex;
	enum yk3_ndev_type   type;
	u16                  qsetid;
	u16                  resv;
	u32                  min_mtu;
	u32                  max_mtu;
	u32                  min_qdepth;
	u32                  max_qdepth;
	struct yk3_queuebase tx_qbase;
	struct yk3_queuebase rx_qbase;
	struct yk3_queuebase own_qbase[YK3_QUEUE_T_MAX];
} __packed;

struct yk3_net_pdevinfo {
	u32                  pf_id : 8;
	u32                  vf_id : 16;
	u32                  vf_num;
	struct yk3_queuebase own_qbase[YK3_QUEUE_T_MAX];
} __packed;

struct yk3_net_devinfo {
	struct yk3_net_pdevinfo pdev; /* pcie dev info */
	struct yk3_net_ndevinfo ndev; /* according devbind type */
} __packed;

struct yk3_net_umd {
	bool enable;
} __packed;

struct yk3_net_pcibar {
	struct {
		u16 bar_idx;
	} req;
	struct {
		u64 bar_addr;
		u64 bar_size;
		u64 bar_offset;
	} rsp;
} __packed;

struct yk3_net_macaddr {
	u8 addr[6];
} __packed;

struct yk3_net_mtu {
	u16 size;
} __packed;

struct yk3_net_linkinfo {
	u32 link_speed;
	u16 link_duplex : 1;
	u16 link_autoneg : 1;
	u16 link_status : 1;
} __packed;

struct yk3_net_rxmode {
	bool enable;
} __packed;

struct yk3_net_dmamap {
	u64 vaddr;
	u64 len;
	u64 iova;
} __packed;

struct yk3_net_start {
	bool enable;
	u16  txqnum;
	u16  rxqnum;
} __packed;

struct yk3_net_vlan_config {
	u16  vlan_id;
	bool enable;
} __packed;

struct yk3_net_rss_hf {
	u64 rss_hf;
} __packed;

#define YK3_RSS_KEY_SIZE      40
#define YK3_MAX_RSS_RETA_SIZE 64
struct yk3_net_rss_key {
	u8 rss_key[YK3_RSS_KEY_SIZE];
	u8 key_len;
} __packed;

struct yk3_net_rss_reta {
	u32 indir[YK3_MAX_RSS_RETA_SIZE];
	u32 qcount;
} __packed;

struct yk3_net_flow_ctrl {
	u32 autoneg;
	u32 rx_pause;
	u32 tx_pause;
} __packed;

struct yk3_net_vf_mac_anti_spoof {
	bool enable;
} __packed;

struct yk3_net_pvid_config {
	u16 pvid;
	__be16 tpid;
} __packed;

struct yk3_net_queue_rate_limit {
	u16 queue_idx;
	u32 tx_rate; /* in Mbps */
} __packed;

#ifndef __user
#define __user
#endif

struct yk3_net_module_eeprom {
	u32 offset;
	u32 len;
	void __user *data;
} __packed;

struct yk3_net_module_info {
	u32 type;
	u32 eeprom_len;
} __packed;

struct yk3_net_phys_id {
	u32 state;  /* 0=off, 1=on */
} __packed;

struct yk3_net_coalesce {
	u32 rx_coalesce_usecs;
	u32 rx_max_coalesced_frames;
	u32 tx_coalesce_usecs;
	u32 tx_max_coalesced_frames;
} __packed;

struct yk3_net_set_ifg {
	u8 ifg_num;
} __packed;

struct yk3_net_set_l1vq {
	u8 mode;
} __packed;

struct yk3_pfc_pcie_l1 {
	u32 pcie_l1_xoff;
	u32 pcie_l1_xon;
} __packed;

enum yk3_dpdk_l1vq_mode {
	DPDK_MODE_PERFORMANCE  = 0x0,
	DPDK_MODE_LATENCY      = 0x1,
};

enum {
	YK3_NET_DEVINFO,
	YK3_NET_UMD,
	YK3_NET_PCIBAR,
	YK3_NET_MACADDR,
	YK3_NET_MTU,
	YK3_NET_LINKINFO,
	YK3_NET_PROMISC,
	YK3_NET_ALLMULTI,
	YK3_NET_DMAMAP,
	YK3_NET_DMAUNMAP,
	YK3_NET_START,
	YK3_NET_OFFLOADCAP,
	YK3_NET_VLAN_FILTER,
	YK3_NET_RSS_HF,
	YK3_NET_RSS_HASH_KEY,
	YK3_NET_RSS_RETA,
	YK3_NET_FLOW_CTRL,
	YK3_NET_VF_CONF,
	YK3_NET_VF_MAC_ANTI_SPOOF,
	YK3_NET_PVID,
	YK3_NET_TX_PVID,
	YK3_NET_QUEUE_RATE_LIMIT,
	YK3_NET_MODULE_EEPROM,
	YK3_NET_MODULE_INFO,
	YK3_NET_PHYS_ID,
	YK3_NET_COALESCE,
	YK3_NET_SET_IFG,
	YK3_NET_PFC_PCIE_L1,
	YK3_NET_MAX,
};

#define YK3_NET_DEVINFO_GET     _IOR(YK3_IOCTL_NET_TYPE, YK3_NET_DEVINFO, struct yk3_net_devinfo)
#define YK3_NET_UMD_SET         _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_UMD, struct yk3_net_umd)
#define YK3_NET_PCIBAR_GET      _IOWR(YK3_IOCTL_NET_TYPE, YK3_NET_PCIBAR, struct yk3_net_pcibar)
#define YK3_NET_MACADDR_GET     _IOR(YK3_IOCTL_NET_TYPE, YK3_NET_MACADDR, struct yk3_net_macaddr)
#define YK3_NET_MACADDR_SET     _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_MACADDR, struct yk3_net_macaddr)
#define YK3_NET_MTU_GET         _IOR(YK3_IOCTL_NET_TYPE, YK3_NET_MTU, struct yk3_net_mtu)
#define YK3_NET_MTU_SET         _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_MTU, struct yk3_net_mtu)
#define YK3_NET_LINKINFO_GET    _IOR(YK3_IOCTL_NET_TYPE, YK3_NET_LINKINFO, struct yk3_net_linkinfo)
#define YK3_NET_PROMISC_SET     _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_PROMISC, struct yk3_net_rxmode)
#define YK3_NET_ALLMULTI_SET    _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_ALLMULTI, struct yk3_net_rxmode)
#define YK3_NET_DMA_MAP         _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_DMAMAP, struct yk3_net_dmamap)
#define YK3_NET_DMA_UNMAP       _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_DMAUNMAP, struct yk3_net_dmamap)
#define YK3_NET_START_SET       _IOW(YK3_IOCTL_NET_TYPE, YK3_NET_START, struct yk3_net_start)
#define YK3_NET_OFFLOADCAP_GET  \
	_IOR(YK3_IOCTL_NET_TYPE, YK3_NET_OFFLOADCAP, struct yk3_net_offloadcap)
#define YK3_NET_OFFLOADCAP_SET  \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_OFFLOADCAP, struct yk3_net_offloadcap)
#define YK3_NET_VLAN_FILTER_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_VLAN_FILTER, struct yk3_net_vlan_config)
#define YK3_NET_RSS_HF_SET      \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_RSS_HF, struct yk3_net_rss_hf)
#define YK3_NET_RSS_KEY_SET     \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_RSS_HASH_KEY, struct yk3_net_rss_key)
#define YK3_NET_RSS_RETA_SET    \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_RSS_RETA, struct yk3_net_rss_reta)
#define YK3_NET_FLOW_CTRL_GET \
	_IOR(YK3_IOCTL_NET_TYPE, YK3_NET_FLOW_CTRL, struct yk3_net_flow_ctrl)
#define YK3_NET_FLOW_CTRL_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_FLOW_CTRL, struct yk3_net_flow_ctrl)
#define YK3_NET_VF_MAC_ANTI_SPOOF_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_VF_MAC_ANTI_SPOOF, struct yk3_net_vf_mac_anti_spoof)
#define YK3_NET_PVID_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_PVID, struct yk3_net_pvid_config)
#define YK3_NET_TX_PVID_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_TX_PVID, struct yk3_net_pvid_config)
#define YK3_NET_QUEUE_RATE_LIMIT_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_QUEUE_RATE_LIMIT, struct yk3_net_queue_rate_limit)
#define YK3_NET_MODULE_EEPROM_GET \
	_IOWR(YK3_IOCTL_NET_TYPE, YK3_NET_MODULE_EEPROM, struct yk3_net_module_eeprom)
#define YK3_NET_MODULE_INFO_GET \
	_IOR(YK3_IOCTL_NET_TYPE, YK3_NET_MODULE_INFO, struct yk3_net_module_info)
#define YK3_NET_PHYS_ID_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_PHYS_ID, struct yk3_net_phys_id)
#define YK3_NET_COALESCE_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_COALESCE, struct yk3_net_coalesce)
#define YK3_NET_IFG_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_SET_IFG, struct yk3_net_set_ifg)
#define YK3_NET_L1VQ_SET \
	_IOW(YK3_IOCTL_NET_TYPE, YK3_NET_PFC_PCIE_L1, struct yk3_net_set_l1vq)
/******************************************************************************/

/* meter */
struct ysc_meter_rate {
	u32 rate; /* mbps */
} __packed;

enum {
	YK3_METER_RX,
	YK3_METER_TX,
	YK3_METER_MAX,
};

#define YK3_METER_RX_SET  _IOW(YK3_IOCTL_METER_TYPE, YK3_METER_RX, struct ysc_meter_rate)
#define YK3_METER_RX_GET  _IOR(YK3_IOCTL_METER_TYPE, YK3_METER_RX, struct ysc_meter_rate)
#define YK3_METER_TX_SET  _IOW(YK3_IOCTL_METER_TYPE, YK3_METER_TX, struct ysc_meter_rate)
#define YK3_METER_TX_GET  _IOR(YK3_IOCTL_METER_TYPE, YK3_METER_TX, struct ysc_meter_rate)
/******************************************************************************/
struct yk3_doe_priv_test_perf {
	u8 tbl_type;
	u8 tbl_id;
	u8 flags;
	u8 n_thread;
	u8 call_mode;
	u8 key_len;
	u8 value_len;
	u32 depth;
};

enum {
	DOE_MEASURE_LEVEL_MIN,
	DOE_MEASURE_LEVEL_MAX,
	DOE_MEASURE_LEVEL_AVG,
	DOE_MEASURE_LEVEL_LT4,

	DOE_MEASURE_LEVEL_GE4,
	DOE_MEASURE_LEVEL_GE10,
	DOE_MEASURE_LEVEL_GE20,
	DOE_MEASURE_LEVEL_GE30,
	DOE_MEASURE_LEVEL_GE40,
	DOE_MEASURE_LEVEL_GE50,
	DOE_MEASURE_LEVEL_GE60,
	DOE_MEASURE_LEVEL_GE70,
	DOE_MEASURE_LEVEL_GE80,
	DOE_MEASURE_LEVEL_GE90,
	DOE_MEASURE_LEVEL_GE100,
	DOE_MEASURE_LEVEL_GE200,
	DOE_MEASURE_LEVEL_GE300,
	DOE_MEASURE_LEVEL_GE400,
	DOE_MEASURE_LEVEL_GE500,
	DOE_MEASURE_LEVEL_GE600,
	DOE_MEASURE_LEVEL_GE700,
	DOE_MEASURE_LEVEL_GE800,
	DOE_MEASURE_LEVEL_GE900,
	DOE_MEASURE_LEVEL_GE1000,
	DOE_MEASURE_LEVEL_GE2000,

	DOE_MEASURE_LEVEL_END
};

struct yk3_doe_measure {
	u64 times;

	union {
		u64 total;
		u64 value;
	};
};

struct yk3_doe_priv_measure {
	int op; // 0: write, 1: read

	struct yk3_doe_measure level[DOE_MEASURE_LEVEL_END];
};

enum {
	YK3_DOE_CREATE_TABLE	= 0,
	YK3_DOE_DELETE_TABLE	= 1,

	YK3_DOE_ADD_ENTRY	= 2,
	YK3_DOE_DELETE_ENTRY	= 3,
	YK3_DOE_UPDATE_ENTRY	= 4,
	YK3_DOE_QUERY_ENTRY	= 5,

	YK3_DOE_GET_CTRL	= 6,
	YK3_DOE_SET_CTRL	= 7,

	YK3_DOE_PRIV_TEST	= 8,
	YK3_DOE_PRIV_MEASURE	= 9,
	YK3_DOE_CMD_END
};

#define YK3_DOE_IOCTL_CREATE_TABLE \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_CREATE_TABLE, struct yk3_tbl_cfg)
#define YK3_DOE_IOCTL_DELETE_TABLE \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_DELETE_TABLE, int)

#define YK3_DOE_IOCTL_ADD_ENTRY    \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_ADD_ENTRY, struct yk3_tbl_entry)
#define YK3_DOE_IOCTL_DELETE_ENTRY \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_DELETE_ENTRY, struct yk3_tbl_entry)
#define YK3_DOE_IOCTL_UPDATE_ENTRY \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_UPDATE_ENTRY, struct yk3_tbl_entry)
#define YK3_DOE_IOCTL_QUERY_ENTRY  \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_QUERY_ENTRY, struct yk3_tbl_entry)

#define YK3_DOE_IOCTL_GET_CTRL _IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_GET_CTRL, struct yk3_tbl_ctl)
#define YK3_DOE_IOCTL_SET_CTRL _IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_SET_CTRL, struct yk3_tbl_ctl)

#define YK3_DOE_IOCTL_TEST_PERF \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_PRIV_TEST, struct yk3_doe_priv_test_perf)
#define YK3_DOE_IOCTL_MEASURE \
	_IOWR(YK3_IOCTL_DOE_TYPE, YK3_DOE_PRIV_MEASURE, struct yk3_doe_priv_measure)
/******************************************************************************/
#endif /* _UAPI_YK3_H */
