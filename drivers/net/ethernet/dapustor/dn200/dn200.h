/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2024, DapuStor Corporation.
 */

#ifndef __DN200_H__
#define __DN200_H__

#define SIMPLE_MODULE_VERSION
#define DN200_RESOURCE_NAME	"dn200"
#define DRV_MODULE_VERSION	"1.0.60"

#include <linux/clk.h>
#include <linux/dcbnl.h>
#include <linux/hrtimer.h>
#include <linux/if_vlan.h>
#include <linux/pci.h>
#include "common.h"
#include <linux/ptp_clock_kernel.h>
#include <linux/net_tstamp.h>
#include <linux/reset.h>
#include <net/page_pool/helpers.h>
#include <net/page_pool/types.h>
#include "dn200_self.h"
#include "dn200_sriov.h"
#include "dn200_spec_acc.h"
#include "dn200_dcb.h"
#include "dn200_iatu.h"
#include "dn200_pool.h"
#include "dn200_reg.h"

#define LINK_UP_SET BIT(0)
#define LINK_DOWN_SET BIT(1)
#define TXRX_ITR_PROCESS_SELF 0
#define TXRX_ITR_COMBINED 1

#undef HAVE_AF_XDP_ZC_SUPPORT
struct dn200_resources {
	void __iomem *addr;
	void __iomem *mail;
	void __iomem *ctrl_addr;
	u8 mac[ETH_ALEN];
	int lpi_irq;
	int irq;
	int sfty_ce_irq;
	int sfty_ue_irq;
	int xpcs_vec;
	int rx_irq[MTL_MAX_RX_QUEUES];
	int tx_irq[MTL_MAX_TX_QUEUES];
	struct ctrl_resource *ctrl;
};

enum dn200_txbuf_type {
	DN200_TXBUF_T_SKB,
	DN200_TXBUF_T_XDP_TX,
	DN200_TXBUF_T_XDP_NDO,
	DN200_TXBUF_T_XSK_TX,
};

enum dn200_txbuf_mem_type {
	DN200_NORMAL,
	DN200_DMA32,
};

struct dn200_tx_info {
	dma_addr_t buf;
	bool map_as_page;
	unsigned int len;
	bool last_segment;
	bool is_jumbo;
	enum dn200_txbuf_type buf_type;
	atomic_t *iatu_ref_ptr;
};

#define DN200_TBS_AVAIL BIT(0)
#define DN200_TBS_EN BIT(1)

/* DMA32 tx buffer used for:
 * 1. pure pf dma addr exceed 1TB;
 * 2. sriov pf dma addr exceed 896GB;
 * 3. sriov vf dma addr exceed 4GB
 */
struct dn200_tx_dma32_buff {
	enum dn200_txbuf_mem_type mem_type;
	struct page *page;
	dma_addr_t buf;
	int order;
	unsigned int len;
	atomic_t *iatu_ref_ptr;
};

/* Frequently used values are kept adjacent for cache effect */
struct dn200_tx_queue {
	u32 tx_count_frames;
	int tbs;
	struct work_struct tx_task;
	struct hrtimer txtimer;
	struct work_struct poll_tx_task;
	struct hrtimer poll_txtimer;
	u32 queue_index;
	struct dn200_priv *priv_data;
	struct dma_desc *dma_tx ____cacheline_aligned_in_smp;
	union {
		struct sk_buff **tx_skbuff;
		struct xdp_frame **xdpf;
	};
	struct dn200_tx_info *tx_skbuff_dma;
	struct dn200_tx_dma32_buff *tx_dma32_bufs;
	atomic_t tx_scheduling;
	struct dn200_queue_iatu_info iatu_info;
	struct xsk_buff_pool *xsk_pool;
	u32 xsk_frames_done;
	unsigned int cur_tx;
	unsigned int next_to_watch;
	unsigned int dirty_tx;
	dma_addr_t dma_tx_phy;
	dma_addr_t origin_dma_tx_phy;
	dma_addr_t tx_tail_addr;
	atomic_t txtimer_running;
	bool task_need_sch;
	bool txtimer_need_sch;
	unsigned int old_dirty_tx;
	u32 mss;
};

/* one rx page can be used twice at the same time,
 * one for protocol statck rx pkt, another for hw rx dma
 */
#define RX_PAGE_MAX_USED_COUNT 2
enum {
	FIRST_PAGE,
	SECON_PAGE,
};

struct dn200_rx_buffer {
	union {
		struct {
			struct page *page;
			dma_addr_t kernel_addr;
			dma_addr_t desc_addr;
			__u32 page_offset;
			u16 rx_times;
			struct dn200_page_buf *pg_buf;
		};
		struct xdp_buff *xdp;
	};
	struct page *sec_page;
	dma_addr_t sec_addr;
	__u32 sec_page_offset; /* second page offsent sph */
	struct dn200_page_buf *sec_pg_buf;
};

struct dn200_rx_queue {
	u32 rx_count_frames;
	u32 queue_index;
	struct page_pool *page_pool;
	struct dn200_rx_buffer *buf_pool;
	struct dn200_priv *priv_data;
	struct dma_desc *dma_rx ____cacheline_aligned_in_smp;
	struct dn200_queue_iatu_info iatu_info;
	atomic_t rx_scheduling;
	unsigned int cur_rx;
	unsigned int dirty_rx;
	unsigned int alloc_rx;
	unsigned int buf_alloc_num;
	u32 rx_zeroc_thresh;
	dma_addr_t origin_dma_rx_phy;
	dma_addr_t dma_rx_phy;
	u32 rx_tail_addr;
	unsigned int state_saved;
	struct {
		struct sk_buff *skb;
		unsigned int len;
		unsigned int error;
	} state;
	struct dn200_bufpool *rx_pool;
	struct work_struct poll_rx_task;
	struct hrtimer poll_rxtimer;
} ____cacheline_internodealigned_in_smp;

#define ITR_COUNTDOWN_COUNT 3
#define DN200_ITR_MASK 0x1fff
#define DN200_ITR_MIN_INC 0x0002
#define DN200_ITR_MIN_USECS 0x0002
#define DN200_ITR_MIN_USECS_1G 0x14
#define DN200_ITR_MAX_USECS 0x70
#define DN200_ITR_MAX_RWT   0xff
#define DN200_ITR_RWT_BOUND   0x14

#define DN200_ITR_ADAPTIVE_LATENCY 0x8000
#define DN200_ITR_ADAPTIVE_BULK 0x0000
#define DN200_ITR_DYNAMIC_ITR 0x8000

struct dn200_itr_info {
	unsigned long next_update;
	u64 packet;
	u64 bytes;
	u32 target_itr;	 /* target ITR setting for ring(s) */
	u32 current_itr; /* current ITR setting for ring(s) */
	u8 itr_countdown;
	u16 itr_div;
	u16 itr_setting;
};

struct itr_update_ops {
	void (*dn200_rx_itr_update)(struct dn200_itr_info *itr,
				struct dn200_priv *priv, u8 chan);
};

struct dn200_channel {
	struct napi_struct rx_napi ____cacheline_aligned_in_smp;
	struct napi_struct tx_napi ____cacheline_aligned_in_smp;
	struct napi_struct agg_napi;
	struct napi_struct rxtx_napi;
	struct dn200_priv *priv_data;
	bool in_sch;
	struct dn200_rx_queue *rx_q;
	spinlock_t lock; /* lock for dma channel hw setting */
	u32 index;
};
struct dn200_tc_entry {
	bool in_use;
	bool in_hw;
	bool is_last;
	bool is_frag;
	void *frag_ptr;
	unsigned int table_pos;
	u32 handle;
	u32 prio;
	struct {
		u32 match_data;
		u32 match_en;
		u8 af : 1;
		u8 rf : 1;
		u8 im : 1;
		u8 nc : 1;
		u8 res1 : 4;
		u8 frame_offset;
		u8 ok_index;
		u8 dma_ch_no;
		u32 res2;
	} __packed val;
};

#define DN200_PPS_MAX 4
struct dn200_pps_cfg {
	bool available;
	struct timespec64 start;
	struct timespec64 period;
};

#define DN200_RSS_IP2TE 1
#define DN200_RSS_UDP4TE 2
#define DN200_RSS_TCP4TE 4

struct dn200_rss {
	int enable;
	u32 rss_flags;
	u8 key[DN200_RSS_HASH_KEY_SIZE];
	u32 table[DN200_RSS_MAX_TABLE_SIZE];
};

struct dn200_flow_entry {
	unsigned long cookie;
	unsigned long action;
	u8 ip_proto;
	int in_use;
	int idx;
	int is_l4;
};

/* Rx Frame Steering */
enum dn200_rfs_type {
	DN200_RFS_T_VLAN,
	DN200_RFS_T_MAX,
};

enum dn200_pf_rxp_set_type {
	RXP_SET_VLAN_FIL = BIT(0),
	RXP_SET_VLAN_ID = BIT(1),
	RXP_SET_UMAC = BIT(2),
	RXP_SET_FIL = BIT(3),
	RXP_CLEAR_VF_RXP = BIT(4),
};

struct dn200_rfs_entry {
	unsigned long cookie;
	int in_use;
	int type;
	int tc;
};

struct dn200_mem_info {
	struct page *page;
	int page_ref_bias;
	dma_addr_t dma_addr;
	dma_addr_t base_addr;
};

struct dn200_page_pool {
	struct device *device;
	int reserve_page;
	struct dn200_mem_info *mem_info;
	int page_order;
	int total_pages; /* total pages of the page pool */
	int alloced_pages;
};

struct dn200_priv {
	/* Frequently used values are kept adjacent for cache effect */
	u32 tx_coal_frames[MTL_MAX_TX_QUEUES];
	u32 tx_coal_timer[MTL_MAX_TX_QUEUES];
	u32 rx_coal_frames[MTL_MAX_RX_QUEUES];
	u32 tx_coal_frames_set[MTL_MAX_TX_QUEUES];
	u64 tx_mem_copy;
	int tx_coalesce;
	int hwts_tx_en;
	bool tx_path_in_lpi_mode;
	bool tso;
	int sph;
	int sph_cap;
	u32 sarc_type;
	u8 txrx_itr_combined;
	cpumask_t *cpu_mask;
	unsigned int dma_buf_sz;
	u32 rx_riwt[MTL_MAX_RX_QUEUES];
	u32 rx_rius[MTL_MAX_RX_QUEUES]; /* rx interrupt coalesce: rx-usecs */
	int hwts_rx_en;
	u32 rx_itr_usec; /* rx itr usecs based on mtu & rx desc ring size */
	u32 rx_itr_usec_min;
	const struct itr_update_ops *dn200_update_ops;
	void __iomem *ioaddr;
	struct net_device *dev;
	struct device *device;
	struct mac_device_info *hw;
	int (*hwif_quirks)(struct dn200_priv *priv);
	struct mutex lock; /* lock for eee setting */

	/* RX Queue */
	struct dn200_rx_queue rx_queue[MTL_MAX_RX_QUEUES];
	unsigned int dma_rx_size;

	/* TX Queue */
	struct dn200_tx_queue tx_queue[MTL_MAX_TX_QUEUES];
	unsigned int dma_tx_size;

	/* Generic channel for NAPI */
	struct dn200_channel channel[DN200_CH_MAX];

	/* RX Queue */
	struct dn200_itr_info rx_intr[MTL_MAX_RX_QUEUES];

	/* TX Queue */
	struct dn200_itr_info tx_intr[MTL_MAX_RX_QUEUES];

	struct dn200_priv_iatu_info iatu_info;
	bool dma32_iatu_used;

	int speed;
	u32 max_usecs;
	u32 min_usecs;
	bool flow_ctrl_an;
	unsigned int flow_ctrl;
	unsigned int pause;
	unsigned int duplex;
	struct mii_bus *mii;
	int mii_irq[PHY_MAX_ADDR];

	struct dn200_extra_stats xstats ____cacheline_aligned_in_smp;
	struct dn200_safety_stats sstats;
	struct plat_dn200enet_data *plat;
	struct plat_dn200_data *plat_ex;
	struct dma_features dma_cap;
	struct dn200_counters mmc;
	struct dn200_swcounters swc;
	int hw_cap_support;
	int chip_id;
	u32 msg_enable;
	int clk_csr;
	struct timer_list eee_ctrl_timer;
	struct timer_list reset_timer;
	struct timer_list keepalive_timer;
	struct timer_list upgrade_timer;
	int lpi_irq;
	int eee_enabled;
	int eee_active;
	int tx_lpi_timer;
	int tx_lpi_enabled;
	int eee_tw_timer;
	bool eee_sw_timer_en;
	unsigned int mode;
	struct hwtstamp_config tstamp_config;
	struct ptp_clock *ptp_clock;
	struct ptp_clock_info ptp_clock_ops;
	unsigned int default_addend;
	u32 sub_second_inc;
	u32 systime_flags;
	u32 adv_ts;
	int use_riwt;
	spinlock_t ptp_lock; /* ptp lock for reg setting */
	/* Protects auxiliary snapshot registers from concurrent access. */
	struct mutex aux_ts_lock;

	void __iomem *mmcaddr;
	void __iomem *ptpaddr;
	unsigned long active_vlans[BITS_TO_LONGS(VLAN_N_VID)];
	int sfty_ce_irq;
	int sfty_ue_irq;
	int xpcs_irq;
	int rx_irq[MTL_MAX_RX_QUEUES];
	int tx_irq[MTL_MAX_TX_QUEUES];
	/*irq name */
	char int_name_mac[IFNAMSIZ + 9];
	char int_name_xpcs[IFNAMSIZ + 9];
	char int_name_wol[IFNAMSIZ + 9];
	char int_name_lpi[IFNAMSIZ + 9];
	char int_name_sfty_ce[IFNAMSIZ + 10];
	char int_name_sfty_ue[IFNAMSIZ + 10];
	char int_name_rx_irq[MTL_MAX_RX_QUEUES][IFNAMSIZ + 14];
	char int_name_tx_irq[MTL_MAX_TX_QUEUES][IFNAMSIZ + 18];

	struct dentry *dbgfs_dir;

	unsigned long state;
	struct workqueue_struct *wq;
	struct workqueue_struct *tx_wq;
	struct work_struct service_task;
	struct work_struct reconfig_task;
	struct work_struct retask;
	struct work_struct vf_process_task;
	struct work_struct vf_linkset_task;
	struct work_struct rxp_task;
	/* Workqueue for handling FPE hand-shaking */
	unsigned long fpe_task_state;
	struct workqueue_struct *fpe_wq;
	struct work_struct fpe_task;
	char wq_name[IFNAMSIZ + 4];

	/* TC Handling */
	unsigned int tc_entries_max;
	unsigned int tc_off_max;
	struct dn200_tc_entry *tc_entries;
	unsigned int flow_entries_max;
	u32 fdir_map;
	u32 fdir_counts;
	struct dn200_flow_entry *flow_entries;
	struct dn200_fdir_filter *fdir_enties;
	struct dn200_fdir_info fdir_info;

	/* Pulse Per Second output */
	struct dn200_pps_cfg pps[DN200_PPS_MAX];

	/* Receive Side Scaling */
	struct dn200_rss rss;

	/* XDP BPF Program */
	unsigned long *af_xdp_zc_qps;
	struct bpf_prog *xdp_prog;
	u64 eth_priv_flags;
	unsigned int vxlan_port;
	bool rec_all;
	/* DCB support */
	struct ieee_ets *ets;
	struct ieee_pfc *pfc;
	unsigned int q2tc_map[MTL_MAX_TX_QUEUES];
	unsigned int prio2q_map[IEEE_8021QAZ_MAX_TCS];
	unsigned int pfcq[MTL_MAX_TX_QUEUES];
	u8 num_tcs;
	u8 prio2tc_bitmap[IEEE_8021QAZ_MAX_TCS];
	u8 dscp_app_cnt;
	u8 dscp2up[DN200_TRUST_DSCP];
	struct dn200_page_pool page_pool;
	struct dn200_bufpool buf_pool;
	struct dn200_vf_rxp_async_info async_info[DN200_MAX_VF_NUM];
	enum dn200_pf_rxp_set_type pf_rxp_set;
	bool vlan_fil_enable;
	u8 clear_vf_rxp_bitmap;
	int numa_node;
	u8 speed_cmd;

	u8 vf_link_forced[DN200_MAX_VF_NUM];
	u8 vf_link_action;
	bool blink_state_last;
	int mtl_queue_fifo_avg;
	int mtl_queue_fifo_more;
	int tx_fifo_queue_0;
	int vf_tx_fifo_size;
	struct device_attribute *temp_attr;
	int upgrade_time;
	bool flag_upgrade;
	bool vf_sw_close_flag;
	bool update_fail;
};

enum dn200_state {
	DN200_DOWN,
	DN200_RESET_REQUESTED,
	DN200_RESETING,
	DN200_SERVICE_SCHED,
	DN200_DCB_DOWN,
	DN200_SUSPENDED,
	DN200_NET_SUSPENDED,
	DN200_SFP_IN_INIT,
	DN200_ERR_RESET,
	DN200_VF_NOTIFY_PF_RESET,
	DN200_VF_FLOW_STATE_SET,
	DN200_IN_REMOVE,
	DN200_DEV_ERR_CLOSE,
	DN200_VF_IN_STOP,
	DN200_DEV_INIT,		/* can't set rxp in dev init state */
	DN200_IATU_INIT,
	DN200_PCIE_UNAVAILD,
	DN200_RXP_SETTING = 17,
	DN200_RXP_NEED_CHECK = 18,
	DN200_MAC_LINK_DOWN = 19,
	DN200_IN_TASK = 20,
	DN200_VF_FLOW_OPEN = 21,
	DN200_VF_FLOW_CLOSE = 22,
	DN200_VF_FLOW_OPEN_SET = 23,
	DN200_PF_NORMAL_CLOSE = 24,
	DN200_PF_NORMAL_OPEN = 25,
	DN200_PF_FLOW_NORMAL_SET = 26,
	DN200_PF_DOWN_UPGRADE = 27,
	DN200_SYS_SUSPENDED = 28,
	DN200_IS_BONDING = 29,
	DN200_PROBE_FINISHED = 30,
	DN200_UP = 31,
};

enum dn200_err_rst_type {
	DN200_TX_TIMEOUT = 0,
	DN200_SAFETY_FEAT_INT = 1,
	DN200_DMA_CHAN_ERR = 2,
	DN200_TX_RESET = 3,
	DN200_VF_TO_PF = 4,
	DN200_NORMAL_RESET = 5,
	POLL_FW_CQ_TIMEOUT = 6,
	DN200_PCIE_UNAVAILD_ERR = 7,
	DN200_PHY_MPLLA_UNLOCK = 8,
	DN200_DMA_DEBUG_ERR = 9,
	DN200_FC_VF_STOP = 10,
};

int dn200_page_buf_alloc(struct dn200_priv *priv);
int dn200_page_buf_get(struct dn200_priv *priv, u32 queue);
int dn200_page_buf_put(struct dn200_priv *priv, u32 queue);

int dn200_mdio_unregister(struct net_device *ndev);
int dn200_mdio_register(struct net_device *ndev);
int dn200_mdio_reset(struct mii_bus *mii);
int dn200_xpcs_setup(struct mii_bus *mii);
void dn200_set_ethtool_ops(struct net_device *netdev);
int dn200_init_tstamp_counter(struct dn200_priv *priv, u32 systime_flags);
void dn200_ptp_register(struct dn200_priv *priv);
void dn200_ptp_unregister(struct dn200_priv *priv);
int dn200_xdp_open(struct net_device *dev);
void dn200_xdp_release(struct net_device *dev);
int dn200_resume(struct device *dev);
int dn200_suspend(struct device *dev);
int dn200_dvr_remove(struct device *dev);
int dn200_dvr_probe(struct device *device,
					struct plat_dn200enet_data *plat_dat,
					struct plat_dn200_data *plat_ex,
					struct dn200_resources *res);
void dn200_disable_eee_mode(struct dn200_priv *priv);
bool dn200_eee_init(struct dn200_priv *priv);
int dn200_reinit_queues(struct net_device *dev, u32 rx_cnt, u32 tx_cnt);
int dn200_reinit_ringparam(struct net_device *dev, u32 rx_size, u32 tx_size);
void dn200_fpe_handshake(struct dn200_priv *priv, bool enable);
int dn200_reinit_hwts(struct net_device *dev, bool initial, u32 new_flags);
static inline bool dn200_xdp_is_enabled(struct dn200_priv *priv)
{
	return !!priv->xdp_prog;
}
int dn200_dev_event(struct notifier_block *unused,
					unsigned long event, void *ptr);
void self_reset(struct dn200_priv *priv);
void dn200_disable_rx_queue(struct dn200_priv *priv, u32 queue);
void dn200_enable_rx_queue(struct dn200_priv *priv, u32 queue);
void dn200_disable_tx_queue(struct dn200_priv *priv, u32 queue);
void dn200_enable_tx_queue(struct dn200_priv *priv, u32 queue);
int dn200_xsk_wakeup(struct net_device *dev, u32 queue, u32 flags);
void dn200_dma_operation_mode(struct dn200_priv *priv);
void dn200_enable_all_queues(struct dn200_priv *priv);
int dn200_tx_clean(struct dn200_priv *priv, int budget, u32 queue);
void dn200_tx_iatu_ref_clean(struct dn200_priv *priv, struct dn200_tx_queue *tx_q);
void dn200_normal_reset(struct dn200_priv *priv);
void dn200_fw_err_dev_close(struct dn200_priv *priv);
void dn200_global_err(struct dn200_priv *priv, enum dn200_err_rst_type err_type);
void dn200_vf_work(struct dn200_priv *priv);
void dn200_async_rxp_work(struct dn200_priv *priv);
void dn200_vf_mac_change(struct dn200_priv *priv);
void dn200_vf_link_set(struct dn200_priv *priv, u8 link_reset);
/*#if IS_ENABLED(CONFIG_DN200_SELFTESTS)*/
#define DN200_SELFTEST
void dn200_selftest_run(struct net_device *dev,
						 struct ethtool_test *etest, u64 *buf);
void dn200_selftest_get_strings(struct dn200_priv *priv, u8 *data);
int dn200_selftest_get_count(struct dn200_priv *priv);

#define DEFAULT_BUFSIZE 1536
static inline u32 dn200_usec2riwt(u64 usec, struct dn200_priv *priv)
{
	unsigned long clk;

	clk = priv->plat->clk_ref_rate;
	if (!clk)
		return 0;
	/* rwtu(rx watchdog timer count unit) use 512, so right shift 9*/
	return (usec * (clk / 1000000)) >> 9;
}

static inline unsigned int dn200_get_bfsize(void)
{
	int ret = 0;

	/* We use a 1536 buffer size for standard Ethernet mtu or jumbo frame.
	 * This gives us enough room for shared info(320) and 192 bytes of padding.
	 * When (NET_SKB_PAD + 1536) bigger than (2048- sizeof shared info(320)),
	 * use suitable buff size
	 */
	if ((NET_SKB_PAD + DEFAULT_BUFSIZE) <= SKB_WITH_OVERHEAD(BUF_SIZE_2KiB))
		ret = DEFAULT_BUFSIZE - NET_IP_ALIGN;
	else
		ret = SKB_WITH_OVERHEAD(BUF_SIZE_2KiB) - NET_SKB_PAD;

	ret = ALIGN_DOWN(ret, 16);
	return ret;
}

static inline unsigned int dn200_rx_offset(struct dn200_priv *priv)
{
	unsigned int page_size, pad_size, buf_len;


	buf_len = dn200_get_bfsize();
	page_size = ALIGN(buf_len, DN200_RX_BUF_SIZE);
	pad_size = SKB_WITH_OVERHEAD(page_size) - buf_len;
	return pad_size;
}

/* page order always use zero as we use page pool now */
#define dn200_rx_pg_order_get(p) (0)

static inline int dn200_rx_pg_size_get(struct dn200_priv *priv)
{
	return (PAGE_SIZE << dn200_rx_pg_order_get(priv));
}

static inline int dn200_rx_refill_size(struct dn200_priv *priv)
{
	return (priv->dma_rx_size >= 128) ? DN200_RX_MAX_REFILL_SIZE : (priv->dma_rx_size >> 1);
}

int dn200_tx_iatu_find(u64 tar_addr, struct dn200_tx_queue *tx_q, atomic_t **iatu_ref_ptr, u64 *base_addr);
int dn200_rx_iatu_find(u64 tar_addr, struct dn200_priv *priv, u64 *base_addr);
int dn200_iatu_init(struct dn200_priv *priv);
void dn200_iatu_uninit(struct dn200_priv *priv);
void dn200_axi_uninit_for_raid(struct dn200_priv *priv);
void dn200_axi_init_for_raid(struct dn200_priv *priv);
void dn200_iatu_display(struct dn200_priv *priv, struct seq_file *seq);
int dn200_add_rx_iatu2tx(struct dn200_priv *priv);
int dn200_datapath_close(struct dn200_priv *priv);
void dn200_datapath_open(struct dn200_priv *priv);
int dn200_vf_flow_state_process(struct dn200_priv *priv);
void extern_phy_force_led(struct phy_device *phydev, struct dn200_priv *priv, u32 index, u32 mode);
void extern_phy_init(struct phy_device *phydev, u8 hw_type);
int extern_phy_read_status(struct phy_device *phydev);
int extern_phy_pause_autoneg_result(struct phy_device *phydev, bool *tx_pause, bool *rx_pause);
int extern_phy_mdix_status_get(struct phy_device *phydev, u8 *mdix, u8 *mdix_ctrl);
int extern_phy_mdix_status_set(struct phy_device *phydev, u8 ctrl);
int ytphy_read_ext(struct phy_device *phydev, u32 regnum);
int ytphy_write_ext(struct phy_device *phydev, u32 regnum, u16 val);
void dn200_start_all_dma(struct dn200_priv *priv);
void dn200_stop_all_dma(struct dn200_priv *priv);
int dn200_clean_all_tx_queues(struct dn200_priv *priv, u8 tx_queue_num);
int dn200_clean_all_rx_queues(struct dn200_priv *priv);
u32 dn200_riwt2usec(u32 riwt, struct dn200_priv *priv);
void dn200_xgmac_clock_ctl(struct dn200_priv *priv);
void dn200_normal_close_open(struct dn200_priv *priv);
int dn200_config_interrupt(struct pci_dev *pdev,
				  struct plat_dn200enet_data *plat,
				  struct plat_dn200_data *plat_ex,
				  struct dn200_resources *res, bool is_nvme_pf);

#endif
