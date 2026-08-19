/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _YK3_BASE_H
#define _YK3_BASE_H

#include "yk3_kcompat.h"

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/printk.h>
#include <linux/device.h>
#include <linux/netdevice.h>
#include <linux/debugfs.h>
#include <linux/atomic.h>
#include <linux/refcount.h>
#include <linux/idr.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/bitfield.h>
#include <linux/ethtool.h>
#include <linux/slab.h>
#include <linux/netlink.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/interval_tree_generic.h>
#include <linux/rbtree.h>
#include <linux/iommu.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/ptp_clock_kernel.h>
#include <linux/ptp_classify.h>

#include <net/devlink.h>
#include <net/udp_tunnel.h>
#include <net/dcbnl.h>

#include <linux/pkt_sched.h>
#include <net/pkt_sched.h>
#include <net/pkt_cls.h>
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
#include <net/vxlan.h>
#endif
#include "yk3_uapi.h"

// static inline struct llist_node *
// yk3_llist_first(const struct llist_head *head)
// {
//	return READ_ONCE(head->first);
// }
#define yk3_llist_first(head)	READ_ONCE((head)->first)

#define USING_YK3_DRIVER
#include "yk3_rdma.h"

#define PCI_VENDOR_ID_YUSUR	0x1f47
#if K2U
#define YK3_DEV_ID_PF		0x1101
#define YK3_DEV_ID_VF		0x1102
#define YK3_DEV_ID_MGR		0x1103
#else
#define YK3_DEV_ID_PF		0x1011
#define YK3_DEV_ID_VF		0x1012
#define YK3_DEV_ID_MGR		0x1013
#endif

#define YK3R_DEV_ID_PF		0x3011
#define YK3R_DEV_ID_VF		0x3012
#define YK3R_DEV_ID_MGR		0x3013

#define YK3_N_PF_MAX_FUNC	512
#define YK3_N_MAX_NETPF		8
#define YK3_N_MAX_MTU		9600
#define YK3_N_MIN_MTU		ETH_MIN_MTU

// 576B for ipv4 rfc791
#define YK3_N_RX_MIN_MTU	(576)

#define YK3_N_NAME_LEN		32
#define YK3_N_TOTAL_QNUM	1024

#define YK3_MAX_TRAFFIC_CLASS   8
#define YK3_MAX_NUM_CHANNELS    64

#define YK3_HW_NAME		KBUILD_MODNAME
enum { YK3_BAR0 = 0, YK3_BAR1, YK3_BAR2, YK3_BAR3, YK3_BAR4, YK3_BAR5, YK3_BAR_MAX };

#define YK3_HW_BIT_EDMA		BIT(0)
#define YK3_HW_BIT_LAN		BIT(1)
#define YK3_HW_BIT_UMAC		BIT(2)
#define YK3_HW_BIT_XMAC		BIT(3)
#define YK3_HW_BIT_DOE		BIT(4)
#define YK3_HW_BIT_NP		BIT(5)
#define YK3_HW_BIT_PPP		BIT(6)
#define YK3_HW_BIT_MBOX		BIT(7)

#define YK3_CAP_BIT_PPPFLOW	BIT(0)
#define YK3_CAP_BIT_NPFLOW	BIT(1)

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL

#define YK3_VXLAN_DEFAULT_PORT	4789
#define YK3_VXLAN_PORT_MAX	3
#define YK3_VXLAN_RESERVE_PORT	1
#define YK3_VF_VXLAN_PORT_MAX	1
#define YK3_GENEVE_DEFAULT_PORT	6081
#define YK3_GENEVE_PORT_MAX	1
#define YK3_MAX_PF_NUM		4

enum {
	YK3_SET_UDP_TUNNEL = 0,
	YK3_UNSET_UDP_TUNNEL,
};

enum {
	YK3_UDP_TNL_OK = 0,
	YK3_UDP_TNL_HW_ERR,
	YK3_UDP_TNL_NO_ENTRY,
	YK3_UDP_TNL_INVALID_PARAM,
	YK3_UDP_TNL_UNINIT,
};

struct yk3_udp_tnl_entry {
	u16 port;
	u32 ref_count;
};

struct yk3_udp_tunnel_port {
	struct yk3_udp_tnl_entry vxlan_entry[YK3_VXLAN_PORT_MAX + YK3_VXLAN_RESERVE_PORT];
	struct yk3_udp_tnl_entry geneve_entry[YK3_GENEVE_PORT_MAX];
};

struct yk3_udp_tunnel_mbox_msg {
	u16 port;
	u16 opcode;
	u16 tnl_type;
};
#endif

struct yk3_queueid {
	u16 g_id;	/* global id */
	u16 p_id;	/* pf queue id, now not used */
	u16 f_id;	/* function queue id */
	u16 l_id;	/* local queue id */
};

struct yk3_statbase {
	char *name;
	u64 base;
	u64 offset;
	u64 resv;
};

#define YK3_STAT(_name, _base, _offset, _resv)	\
	{					\
		.name = _name,			\
		.base = _base,			\
		.offset = _offset,		\
		.resv = _resv,			\
	}

struct yk3_qset_table;
struct yk3_edma_info;
struct yk3_edma_priv;
struct yk3_emp_info;

struct yk3_card_sn {
	u8 num[20];
};

struct yk3_card {
	char name[YK3_N_NAME_LEN];
	u32 id;			/* card id */
	enum yk3_chip chip;	/* card chip : k3 or k3max */
	enum yk3_runmode mode;	/* card run mode */
	struct yk3_card_sn sn;	/* card serial number */
	u64 hw_bits;		/* card hardware bits */
	u64 cap_bits;		/* card capability bits */
	int numa;		/* card numa id */
	u16 qnum;		/* card queue number */
	u16 pf_num;		/* card pf number */
	u16 port_type;		/* enum yk3_port_type */
	u8 board_type;
	struct list_head pdev_priv_head;	/* pdev priv list */
	struct mutex pdev_priv_head_mlock;	/* pdev priv list mlock */
	struct yk3_pdev_priv *mgr_pdev_priv;	/* mgr_pdev_priv could be null, check it */

	cpumask_var_t local_cpumask;
	cpumask_var_t neigh_cpumask;
	cpumask_var_t remote_cpumask;
	u32 affinity_seqnum;

	/* qset */
	struct yk3_qset_table *qset_table;

	/* edma */
	u32 edma_clk;
	struct yk3_edma_info *edma_info;
	u16 pf_ndev_qnum;
	u16 vf_ndev_qnum;
	u32 coalesce_max_usecs;
	u32 coalesce_max_frames;
	u16 qsetid_base;
	u16 qsetid_num;

	/* np */
	struct yk3_np *np;

	/* mac */
	u16 mac_ch_num;
	enum yk3_mac_channel mac_chs[YK3_N_MAX_NETPF];

	struct yk3_emp_info *emp_info;

	struct timer_list check_emp;
	u32 emp_tick;
	u32 emp_temp_l2_alarm;
	u32 emp_vol_l2_alarm;
	bool emp_close_hw;

	u32 linkup_dbg;

	u32 link_speed_cfg;
	u8 link_speed_cfg_autoneg:1;

	/* debugfs */
	struct dentry *dbgfs_dir;
	struct dentry *dbgfs_info_file;
};

struct yk3_irq_table;
struct yk3_sriov_priv;
struct yk3_mbox;
struct yk3_ppp;
struct yk3_qos;
struct yk3_lan;
struct yk3_ptp;

struct yk3_pdev_priv {
	char name[YK3_N_NAME_LEN];
	/* relation */
	struct device *dev;
	struct pci_dev *pdev;
	struct yk3_card *card;			/* card info */
	struct list_head card_node;		/* card node */
	struct list_head ndev_priv_head;	/* ndev priv list */
	struct mutex ndev_priv_head_mlock;	/* ndev priv list mlock */

	/* property */
	u32 vendor;		/* PCI vendor id */
	u32 device;		/* PCI device id */
	u32 pf_id:8;
	u32 vf_id:16;

	/* bar */
	void __iomem *bar_addr[YK3_BAR_MAX];
	resource_size_t bar_size[YK3_BAR_MAX];
	resource_size_t bar_offset[YK3_BAR_MAX];
	resource_size_t bar_pa[YK3_BAR_MAX];

	u16 temperature;

	/* config */
	u16 evb_mode;

	/* irq */
	struct yk3_irq_table *irq_table;

	/* mbox */
	struct yk3_mbox *mbox;

	/* ppp */
	struct yk3_ppp *ppp;

	/* edma */
	struct yk3_edma_priv *edma_priv;
	struct yk3_sriov_priv *sriov_priv;

	/* mac, NOT support multiple mac channels to one pf */
	struct yk3_mac *mac;

	/* hqos */
	struct yk3_qos *qos;

	/* lan */
	struct yk3_lan *lan;

	/* ptp */
	struct yk3_ptp *ptp;

	/* doe */
	struct yk3_doe_priv *doe_priv;
#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	/* udp tunnel config */
	struct yk3_udp_tunnel_port *udp_tnl_port;
#endif
	/* rdma */
	struct platform_device *platform_dev;
	struct yk3_rdma_data *rdma_data;

	/* debugfs */
	struct dentry *dbgfs_dir;
	struct dentry *dbgfs_info_file;
	struct dentry *dbgfs_sriov_file;
};

struct yk3_vlan {
	u16 vlan_id;
	struct list_head list;
};

struct yk3_edma_qpair;

struct yk3_tc_info {
	u16 qoffset;
	u16 qcount_tx;
	u16 qcount_rx;
	u8 netdev_tc;
};

enum qos_tc_flags {
	YK3_QDISC_VALID,
	YK3_ETS_PENDING,
	YK3_ETS_VALID,
	YK3_FLAGS_TC_NBITS,
};

struct yk3_tc_cfg {
	u8 numtc; /* Total number of enabled TCs */
	u8 ena_tc; /* Tx map */
	DECLARE_BITMAP(flags, YK3_FLAGS_TC_NBITS);
	struct yk3_tc_info tc_info[YK3_MAX_TRAFFIC_CLASS];
	u16 tc2txq[YK3_MAX_TRAFFIC_CLASS][YK3_MAX_NUM_CHANNELS];
};

struct yk3_ndev_priv {
	/* relation */
	struct net_device *ndev;
	struct pci_dev *pdev;
	struct yk3_pdev_priv *pdev_priv;
	struct list_head pdev_node;		/* pdev node */

	/* property */
	char name[YK3_N_NAME_LEN];
	u16 qsetid;
	enum yk3_ndev_type type;
	u32 pf_id:8;
	u32 vf_id:16;
	struct yk3_queuebase qbase[YK3_QUEUE_T_MAX];
	u16 txq_depth;
	u16 rxq_depth;
	u16 txq_real_num;
	u16 rxq_real_num;

	/* resources */
	struct yk3_edma_qpair *qpair;
	struct mutex state_mlock;		/* state mlock */

	/* cdev */
	bool umd_enable;
	netdev_features_t cdev_features;

	/* ndev configs */
	u8 intf_mac[ETH_ALEN];
	bool lro_enable;
	u64 itr_rx_enable:1;
	u64 itr_tx_enable:1;
	u32 rx_coalesce_usecs;
	u32 rx_max_coalesced_frames;
	u32 tx_coalesce_usecs;
	u32 tx_max_coalesced_frames;
	struct list_head cvlan_list;
	struct list_head svlan_list;
	u32 rxfh_cfg;
	u32 netdev_flags;
	u32 ethtool_priv_flags;
	u32 mdrop_rx_rate;
	u32 hqos_tx_rate;
	u32 link_speed_cfg;
	u32 link_speed;
	u8 link_fec_cfg;
	u8 port;
	u8 link_duplex;
	u8 link_status:1;
	u8 link_speed_cfg_autoneg:1;

	/* statistics lock, when get statistics of net_device */
	spinlock_t statistics_lock;

#if defined YK3_HAVE_UDP_TUNNEL_NIC_INFO || defined YK3_HAVE_NDO_UDP_TUNNEL
	struct yk3_udp_tunnel_port *udp_tnl_port;
#endif

	/* debugfs */
	struct dentry *dbgfs_dir;
	struct dentry *dbgfs_info_file;
	struct yk3_fcdbg_params *fcdbg;

	/* dcbx */
#ifdef CONFIG_DCB
	struct yk3_dcbx *dcbx;
#endif

	struct yk3_tc_cfg tc_cfg;
	struct yk3_ets_cfg *etscfg;
	struct yk3_ets_cfg *etsrec;
};

static inline bool yk3_pdev_is_pf(struct yk3_pdev_priv *priv)
{
	return (priv->device == YK3_DEV_ID_PF ||
		priv->device == YK3R_DEV_ID_PF);
}

static inline bool yk3_pdev_is_vf(struct yk3_pdev_priv *priv)
{
	return (priv->device == YK3_DEV_ID_VF ||
		priv->device == YK3R_DEV_ID_VF);
}

static inline bool yk3_pdev_is_mgr(struct yk3_pdev_priv *priv)
{
	return (priv->device == YK3_DEV_ID_MGR ||
		priv->device == YK3R_DEV_ID_MGR);
}

static inline bool yk3_ndev_is_pf(struct yk3_ndev_priv *priv)
{
	return (priv->type == YK3_NDEV_T_PF);
}

static inline bool yk3_ndev_is_vf(struct yk3_ndev_priv *priv)
{
	return (priv->type == YK3_NDEV_T_VF);
}

static inline bool yk3_pdev_is_rdma(struct yk3_pdev_priv *priv)
{
	return (priv->device == YK3R_DEV_ID_PF ||
		priv->device == YK3R_DEV_ID_VF ||
		priv->device == YK3R_DEV_ID_MGR);
}

/* vf_id : for rep ndev */
static inline struct yk3_ndev_priv *
yk3_pdev_get_ndev_priv(struct yk3_pdev_priv *pdev_priv, enum yk3_ndev_type type, int vf_id)
{
	struct yk3_ndev_priv *ndev_priv;

	mutex_lock(&pdev_priv->ndev_priv_head_mlock);
	list_for_each_entry(ndev_priv, &pdev_priv->ndev_priv_head, pdev_node) {
		if (ndev_priv->type == type) {
			mutex_unlock(&pdev_priv->ndev_priv_head_mlock);
			return ndev_priv;
		}
	}
	mutex_unlock(&pdev_priv->ndev_priv_head_mlock);

	return NULL;
}

#endif /* _YK3_BASE_H */
