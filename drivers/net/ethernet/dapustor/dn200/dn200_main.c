// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *This is the driver for the Dapustor DN200 10/100/1000/10G Ethernet controllers.
 *
 * Copyright (c) 2024 DapuStor Corporation.
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/if.h>
#include <linux/if_vlan.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/prefetch.h>
#include <linux/pinctrl/consumer.h>
#include <net/ipv6.h>
#include <net/ip6_route.h>
#include <net/addrconf.h>
#include <net/pkt_sched.h>
#include <net/vxlan.h>
#include <net/udp_tunnel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/net_tstamp.h>
#include <linux/udp.h>
#include <linux/bpf_trace.h>
#include <net/pkt_cls.h>
#include "dn200_ptp.h"
#include "dn200.h"
#include <linux/reset.h>
#include <linux/of_mdio.h>
#include <linux/netdevice.h>

#include "dwxgmac_comm.h"
#include "hwif.h"
#include "dn200_phy.h"
#include "dn200_sriov.h"
#include "dn200_cfg.h"
#include "dn200_dcb.h"
#include "dn200_pool.h"
#include "dn200_eprom.h"

/* As long as the interface is active, we keep the timestamping counter enabled
 * with fine resolution and binary rollover. This avoid non-monotonic behavior
 * (clock jumps) when changing timestamping settings at runtime.
 */
#define DN200_HWTS_ACTIVE (PTP_TCR_TSENA | PTP_TCR_TSCFUPDT | \
							PTP_TCR_TSCTRLSSR)

#define DN200_ALIGN(x) ALIGN(ALIGN(x, SMP_CACHE_BYTES), 16)
#define TSO_MAX_BUFF_SIZE (SZ_16K - 1)

/* Module parameters */
#define TX_TIMEO 5000
#define DN200_TX_THRESH(x) ((x)->dma_tx_size / 4)
#define DN200_RX_THRESH(x) ((x)->dma_rx_size / 4)

/* Limit to make sure XDP TX and slow path can coexist */
#define DN200_XSK_TX_BUDGET_MAX 256
#define DN200_TX_XSK_AVAIL 16
#define DN200_RX_FILL_BATCH 32

#define DN200_XDP_PASS 0
#define DN200_XDP_CONSUMED BIT(0)
#define DN200_XDP_TX BIT(1)
#define DN200_XDP_REDIRECT BIT(2)

/* default msg level always output to dmesg after driver probe
 * if we don't want to output in default state, should not include them
 * you can use ethtool -s enpxxx msglvl xxx to open
 * NETIF_MSG_TX_QUEUED, NETIF_MSG_PKTDATA, NETIF_MSG_RX_STATUS, etc
 */
static const u32 default_msg_level = (NETIF_MSG_PROBE | NETIF_MSG_IFUP);

#define DN200_DEFAULT_LPI_TIMER 1000
#define DN200_LPI_T(x) (jiffies + usecs_to_jiffies(x))

static irqreturn_t dn200_interrupt(int irq, void *dev_id);
/* For MSI interrupts handling */
static irqreturn_t dn200_mac_interrupt(int irq, void *dev_id);
static irqreturn_t dn200_safety_interrupt(int irq, void *dev_id);
static irqreturn_t dn200_msi_intr_tx(int irq, void *data);
static irqreturn_t dn200_msi_intr_rx(int irq, void *data);
static irqreturn_t dn200_msi_intr_rxtx(int irq, void *data);
static void dn200_tx_timer_arm(struct dn200_priv *priv, u32 queue);
static void dn200_flush_tx_descriptors(struct dn200_priv *priv, int queue);
static void dn200_heartbeat(struct timer_list *t);
static void dn200_disable_all_queues(struct dn200_priv *priv);
static void dn200_xgmac_rx_ext_clk_set(struct dn200_priv *priv, bool external);

static const struct net_device_ops dn200_netdev_ops;
static const struct net_device_ops dn200_vf_netdev_ops;
static void dn200_init_fs(struct net_device *dev);
static void dn200_exit_fs(struct net_device *dev);

static void dn200_napi_add(struct net_device *dev);
static void dn200_napi_del(struct net_device *dev);
static void dn200_vxlan_set(struct net_device *netdev)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u32 value;

	writel(priv->vxlan_port, priv->ioaddr + XGMAC_TUNNEL_IDENTIFIER);
	value = readl(priv->ioaddr + XGMAC_PACKET_FILTER);
	value |= XGMAC_FILTER_VUCC;
	writel(value, priv->ioaddr + XGMAC_PACKET_FILTER);

	value = readl(priv->ioaddr + XGMAC_TX_CONFIG);
	value &= ~XGMAC_CONFIG_VNM;	// VXLAN
	value |= XGMAC_CONFIG_VNE;
	writel(value, priv->ioaddr + XGMAC_TX_CONFIG);
}

static int dn200_vxlan_unset(struct net_device *netdev)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u32 value;

	value = readl(priv->ioaddr + XGMAC_PACKET_FILTER);
	value &= ~XGMAC_FILTER_VUCC;
	writel(value, priv->ioaddr + XGMAC_PACKET_FILTER);

	value = readl(priv->ioaddr + XGMAC_TX_CONFIG);
	value &= ~(XGMAC_CONFIG_VNM | XGMAC_CONFIG_VNE);
	writel(value, priv->ioaddr + XGMAC_TX_CONFIG);
	priv->vxlan_port = 0;
	writel(0, priv->ioaddr + XGMAC_TUNNEL_IDENTIFIER);
	return 0;
}

#define DN200_COAL_TIMER(x) (ns_to_ktime((x) * NSEC_PER_USEC))
#define DN200_POLL_TIMER(x) (ns_to_ktime((x) * NSEC_PER_MSEC))


static int dn200_vxlan_set_port(struct net_device *netdev, unsigned int table,
				unsigned int entry, struct udp_tunnel_info *ti)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u16 status = 0, flag;

	priv->vxlan_port = be16_to_cpu(ti->port);
	if (!HW_IS_PUREPF(priv->hw)) {
		status = dn200_get_vxlan_status(priv->hw);
		if (!status)
			dn200_vxlan_set(netdev);
		flag = PRIV_IS_VF(priv) ? (DN200_VF_OFFSET_GET(priv->hw) + 1) :
					  0;
		status = status | (1 << flag);
		dn200_set_vxlan_status(priv->hw, status);
	} else {
		dn200_vxlan_set(netdev);
	}
	return 0;
}

static int dn200_vxlan_unset_port(struct net_device *netdev, unsigned int table,
				  unsigned int entry,
				  struct udp_tunnel_info *ti)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	u16 status = 0, flag;

	priv->vxlan_port = 0;
	if (!PRIV_IS_VF(priv))
		dn200_vxlan_unset(netdev);
	if (!HW_IS_PUREPF(priv->hw)) {
		status = dn200_get_vxlan_status(priv->hw);
		flag = PRIV_IS_VF(priv) ? (DN200_VF_OFFSET_GET(priv->hw) + 1) :
					  0;
		status = status & (~(1 << flag));
		if (!status)
			dn200_vxlan_unset(netdev);
		dn200_set_vxlan_status(priv->hw, status);
	} else {
		dn200_vxlan_unset(netdev);
	}
	return 0;
}

const struct udp_tunnel_nic_info dn200_udp_tunnels = {
	.set_port = dn200_vxlan_set_port,
	.unset_port = dn200_vxlan_unset_port,
	.flags = UDP_TUNNEL_NIC_INFO_OPEN_ONLY,
	.tables = {
		{
			.n_entries = 1,
			.tunnel_types = UDP_TUNNEL_TYPE_VXLAN,
		},
	},
};


static void dn200_init_ndev_tunnel(struct net_device *ndev)
{
	netdev_features_t tso_features;

	tso_features = NETIF_F_GSO_UDP_TUNNEL |
	    NETIF_F_GSO_UDP_TUNNEL_CSUM | NETIF_F_GSO_PARTIAL;

	ndev->hw_features |= tso_features;
	ndev->features |= tso_features;
	ndev->hw_enc_features |= ndev->features;
	ndev->vlan_features |= tso_features;
	/*support gso partial features to calc tunnel out udp csum in kernel */
	ndev->gso_partial_features |= NETIF_F_GSO_UDP_TUNNEL_CSUM;
	ndev->udp_tunnel_nic_info = &dn200_udp_tunnels;
}

static void __dn200_disable_all_queues(struct dn200_priv *priv)
{
	u32 rx_queues_cnt = priv->plat->rx_queues_to_use;
	u32 tx_queues_cnt = priv->plat->tx_queues_to_use;
	u32 maxq = max(rx_queues_cnt, tx_queues_cnt);
	u32 queue;

	for (queue = 0; queue < maxq; queue++) {
		struct dn200_channel *ch = &priv->channel[queue];

		if (queue < rx_queues_cnt && queue < tx_queues_cnt &&
			 priv->txrx_itr_combined) {
			napi_disable(&ch->agg_napi);
		} else {
			if (queue < rx_queues_cnt)
				napi_disable(&ch->rx_napi);
			if (queue < tx_queues_cnt)
				napi_disable(&ch->tx_napi);
		}
	}
}

void dn200_normal_reset(struct dn200_priv *priv)
{
	dn200_global_err(priv, DN200_NORMAL_RESET);
}

void dn200_fw_err_dev_close(struct dn200_priv *priv)
{
	if (!PRIV_IS_VF(priv))
		DN200_SET_LRAM_MAILBOX_MEMBER(priv->hw, pf_fw_err_states, 1);
	set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
	dn200_global_err(priv, POLL_FW_CQ_TIMEOUT);
}

/* do hw reset and netdev rlease and open
 * for pf: notify all vfs to release and open
 * for vf: notify pf to do global err reset (pf will notify all vfs)
 */
void dn200_global_err(struct dn200_priv *priv, enum dn200_err_rst_type err_type)
{
	u8 states = 0;

	netif_carrier_off(priv->dev);
	if (err_type == DN200_TX_TIMEOUT)
		priv->xstats.tx_timeout_rst_count++;
	else if (err_type == DN200_DMA_CHAN_ERR)
		priv->xstats.dma_chan_err_rst_count++;

	if (err_type != DN200_NORMAL_RESET && !test_bit(DN200_DEV_ERR_CLOSE, &priv->state))
		netdev_info(priv->dev, "%s, %d, global reset type:%d.\n", __func__,
				__LINE__, err_type);

	if (err_type == POLL_FW_CQ_TIMEOUT || err_type == DN200_PCIE_UNAVAILD_ERR ||
		err_type == DN200_PHY_MPLLA_UNLOCK) {
		if (!test_bit(DN200_DOWN, &priv->state) &&
			!test_and_set_bit(DN200_DEV_ERR_CLOSE, &priv->state)) {
			/* schedule task to do pf sw & hw reset,
			 * and notify vf to stop & open netdev
			 */
			queue_work(priv->wq, &priv->service_task);
			return;
		}
	}

	/* for critical err, notify pf to do reset */
	if (PRIV_IS_VF(priv) && err_type != DN200_NORMAL_RESET) {
		dev_dbg(priv->device,
			"%s, %d, vf send err rst request to pf.\n", __func__,
			__LINE__);
		/* if PF is down, no need to notify pf do reset */
		DN200_GET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, &states);
		if (!states)
			return;

		/* within vf driver, global_err will be called in timer
		 * processing context when tx timeout,
		 * can't call dn200_vf_glb_err_rst_notify & irq_peer_notify directly,
		 * as irq_peer_notify will sleep
		 */
		if (!test_and_set_bit(DN200_VF_NOTIFY_PF_RESET, &priv->state))
			dn200_vf_work(priv);
		return;
	}

	/* shedule work queue to do DN200_ERR_RESET */
	if (!test_bit(DN200_DOWN, &priv->state) &&
	    !test_and_set_bit(DN200_ERR_RESET, &priv->state)) {
		dev_dbg(priv->device,
			"%s, %d, schedule task to do pf sw & hw reset, and notify vf to stop & open netdev.\n",
			__func__, __LINE__);
		/* schedule task to do pf sw & hw reset, and notify vf to stop & open netdev */

		queue_work(priv->wq, &priv->service_task);
	}
}

void dn200_vf_work(struct dn200_priv *priv)
{
	queue_work(priv->wq, &priv->vf_process_task);
}

/**
 * dn200_clk_csr_set - dynamically set the MDC clock
 * @priv: driver private structure
 * Description: this is to dynamically set the MDC clock according to the csr
 * clock input.
 * Note:
 *	If a specific clk_csr value is passed from the platform
 *	this means that the CSR Clock Range selection cannot be
 *	changed at run-time and it is fixed (as reported in the driver
 *	documentation). Viceversa the driver will try to set the MDC
 *	clock dynamically according to the actual clock input.
 */
static void dn200_clk_csr_set(struct dn200_priv *priv)
{
	u32 clk_rate;

	clk_rate = clk_get_rate(priv->plat->dn200_clk);

	/* Platform provided default clk_csr would be assumed valid
	 * for all other cases except for the below mentioned ones.
	 * For values higher than the IEEE 802.3 specified frequency
	 * we can not estimate the proper divider as it is not known
	 * the frequency of clk_csr_i. So we do not change the default
	 * divider.
	 */
	if (!(priv->clk_csr & MAC_CSR_H_FRQ_MASK)) {
		if (clk_rate < CSR_F_35M)
			priv->clk_csr = DN200_CSR_20_35M;
		else if ((clk_rate >= CSR_F_35M) && (clk_rate < CSR_F_60M))
			priv->clk_csr = DN200_CSR_35_60M;
		else if ((clk_rate >= CSR_F_60M) && (clk_rate < CSR_F_100M))
			priv->clk_csr = DN200_CSR_60_100M;
		else if ((clk_rate >= CSR_F_100M) && (clk_rate < CSR_F_150M))
			priv->clk_csr = DN200_CSR_100_150M;
		else if ((clk_rate >= CSR_F_150M) && (clk_rate < CSR_F_250M))
			priv->clk_csr = DN200_CSR_150_250M;
		else if ((clk_rate >= CSR_F_250M) && (clk_rate <= CSR_F_300M))
			priv->clk_csr = DN200_CSR_250_300M;
	}

	if (priv->plat->has_xgmac) {
		if (clk_rate > 400000000)
			priv->clk_csr = 0x5;
		else if (clk_rate > 350000000)
			priv->clk_csr = 0x4;
		else if (clk_rate > 300000000)
			priv->clk_csr = 0x3;
		else if (clk_rate > 250000000)
			priv->clk_csr = 0x2;
		else if (clk_rate > 150000000)
			priv->clk_csr = 0x1;
		else
			priv->clk_csr = 0x0;
	}
}

static void print_pkt(unsigned char *buf, int len)
{
	pr_debug("len = %d byte, buf addr: 0x%p\n", len, buf);
	print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, buf, len);
}

static inline u32 dn200_ring_entries_calc(unsigned int ring_size,
	unsigned int start_idx, unsigned int end_idx)
{
	u32 entry_num;

	if (end_idx >= start_idx)
		entry_num = end_idx - start_idx + 1;
	else
		entry_num = ring_size - start_idx + end_idx + 1;

	return entry_num;
}

static inline u32 dn200_tx_avail(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	u32 avail;

	if (tx_q->dirty_tx > tx_q->cur_tx)
		avail = tx_q->dirty_tx - tx_q->cur_tx - 1;
	else
		avail = priv->dma_tx_size - tx_q->cur_tx + tx_q->dirty_tx - 1;

	return avail;
}

/**
 * dn200_rx_dirty - Get RX queue dirty
 * @priv: driver private structure
 * @queue: RX queue index
 */
static inline u32 dn200_rx_dirty(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	u32 dirty = 0;

	if (rx_q->dirty_rx < rx_q->cur_rx) {
		dirty = rx_q->cur_rx - rx_q->dirty_rx - 1;
	} else if (rx_q->dirty_rx > rx_q->cur_rx) {
		/* dirty_rx point to the end of ring at the beginning,
		 * when curr_rx equal to dirty_rx means the ring is empty
		 */
		dirty = priv->dma_rx_size - rx_q->dirty_rx + rx_q->cur_rx - 1;
	} else {
		dev_err(priv->device,
			"%s, %d, rx ring %d is abormal, as dirty_rx can't equal cur_rx, drity:%d, cur:%d",
			__func__, __LINE__, queue, rx_q->dirty_rx,
			rx_q->cur_rx);
		dirty = 0;
	}

	return dirty;
}

static void dn200_lpi_entry_timer_config(struct dn200_priv *priv, bool en)
{
	int tx_lpi_timer;

	/* Clear/set the SW EEE timer flag based on LPI ET enablement */
	priv->eee_sw_timer_en = en ? 0 : 1;
	tx_lpi_timer = en ? priv->tx_lpi_timer : 0;
	dn200_set_eee_lpi_timer(priv, priv->hw, tx_lpi_timer);
}

/**
 * dn200_enable_eee_mode - check and enter in LPI mode
 * @priv: driver private structure
 * Description: this function is to verify and enter in LPI mode in case of
 * EEE.
 */
static int dn200_enable_eee_mode(struct dn200_priv *priv)
{
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	u32 queue;

	/* check if all TX queues have the work finished */
	for (queue = 0; queue < tx_cnt; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];

		if (tx_q->dirty_tx != tx_q->cur_tx)
			return -EBUSY;	/* still unfinished work */
	}

	/* Check and enter in LPI mode */
	if (!priv->tx_path_in_lpi_mode)
		dn200_set_eee_mode(priv, priv->hw,
				   priv->plat->en_tx_lpi_clockgating, false);
	return 0;
}

/**
 * dn200_disable_eee_mode - disable and exit from LPI mode
 * @priv: driver private structure
 * Description: this function is to exit and disable EEE in case of
 * LPI state is true. This is called by the xmit.
 */
void dn200_disable_eee_mode(struct dn200_priv *priv)
{
	if (!priv->eee_sw_timer_en) {
		dn200_lpi_entry_timer_config(priv, 0);
		return;
	}

	dn200_reset_eee_mode(priv, priv->hw);
	del_timer_sync(&priv->eee_ctrl_timer);
	priv->tx_path_in_lpi_mode = false;
}

/**
 * dn200_eee_ctrl_timer - EEE TX SW timer.
 * @t:  timer_list struct containing private info
 * Description:
 *  if there is no data transfer and if we are not in LPI state,
 *  then MAC Transmitter can be moved to LPI state.
 */
static void dn200_eee_ctrl_timer(struct timer_list *t)
{
	struct dn200_priv *priv = from_timer(priv, t, eee_ctrl_timer);

	if (dn200_enable_eee_mode(priv))
		mod_timer(&priv->eee_ctrl_timer,
			  DN200_LPI_T(priv->tx_lpi_timer));
}

/**
 * dn200_eee_init - init EEE
 * @priv: driver private structure
 * Description:
 *  if the GMAC supports the EEE (from the HW cap reg) and the phy device
 *  can also manage EEE, this function enable the LPI state and start related
 *  timer.
 */
bool dn200_eee_init(struct dn200_priv *priv)
{
	int eee_tw_timer = priv->eee_tw_timer;

	if (PRIV_IS_VF(priv))
		return false;

	/* Using PCS we cannot dial with the phy registers at this stage
	 * so we do not support extra feature like EEE.
	 */
	if (priv->hw->pcs == DN200_PCS_TBI || priv->hw->pcs == DN200_PCS_RTBI)
		return false;

	/* Check if MAC core supports the EEE feature. */
	if (!priv->dma_cap.eee)
		return false;

	mutex_lock(&priv->lock);

	/* Check if it needs to be deactivated */
	if (!priv->eee_active) {
		if (priv->eee_enabled) {
			netdev_dbg(priv->dev, "disable EEE\n");
			dn200_lpi_entry_timer_config(priv, 0);
			del_timer_sync(&priv->eee_ctrl_timer);
			dn200_set_eee_timer(priv, priv->hw, 0, eee_tw_timer);
			if (priv->plat_ex->has_xpcs)
				dn200_xpcs_config_eee(PRIV_PHY_INFO(priv),
						      priv->plat->mult_fact_100ns, false);
			dn200_reset_eee_mode(priv, priv->hw);
		}
		mutex_unlock(&priv->lock);
		return false;
	}

	if (priv->eee_active && !priv->eee_enabled) {
		timer_setup(&priv->eee_ctrl_timer, dn200_eee_ctrl_timer, 0);
		dn200_set_eee_timer(priv, priv->hw, DN200_DEFAULT_LIT_LS,
				    eee_tw_timer);
		if (priv->plat_ex->has_xpcs)
			dn200_xpcs_config_eee(PRIV_PHY_INFO(priv),
					      priv->plat->mult_fact_100ns,
					      true);
	}

	if ((priv->mii || priv->plat->has_gmac4) && priv->tx_lpi_timer <= DN200_ET_MAX) {
		del_timer_sync(&priv->eee_ctrl_timer);
		priv->tx_path_in_lpi_mode = false;
		dn200_lpi_entry_timer_config(priv, 1);
		dn200_set_eee_timer(priv, priv->hw, DN200_DEFAULT_LIT_LS,
				    0x64);
		dn200_set_eee_mode(priv, priv->hw,
				   priv->plat->en_tx_lpi_clockgating, true);
	} else {
		dn200_lpi_entry_timer_config(priv, 0);
		mod_timer(&priv->eee_ctrl_timer,
			  DN200_LPI_T(priv->tx_lpi_timer));
	}

	mutex_unlock(&priv->lock);
	netdev_dbg(priv->dev, "Energy-Efficient Ethernet initialized\n");
	return true;
}

/* dn200_get_tx_hwtstamp - get HW TX timestamps
 * @priv: driver private structure
 * @p : descriptor pointer
 * @skb : the socket buffer
 * Description :
 * This function will read timestamp from the descriptor & pass it to stack.
 * and also perform some sanity checks.
 */
static void dn200_get_tx_hwtstamp(struct dn200_priv *priv,
				  struct dma_desc *p, struct sk_buff *skb)
{
	struct skb_shared_hwtstamps shhwtstamp;
	bool found = false;
	u64 ns = 0;

	if (!priv->hwts_tx_en || PRIV_IS_VF(priv))
		return;

	/* exit if skb doesn't support hw tstamp */
	if (likely(!skb || !(skb_shinfo(skb)->tx_flags & SKBTX_IN_PROGRESS)))
		return;

	/* check tx tstamp status */
	if (dn200_get_tx_timestamp_status(priv, p)) {
		dn200_get_timestamp(priv, p, priv->adv_ts, &ns);
		found = true;
	} else if (!dn200_get_mac_tx_timestamp(priv, priv->hw, &ns)) {
		found = true;
	}

	if (found) {
		memset(&shhwtstamp, 0, sizeof(struct skb_shared_hwtstamps));
		shhwtstamp.hwtstamp = ns_to_ktime(ns);

		netdev_dbg(priv->dev, "get valid TX hw timestamp %llu\n", ns);
		/* pass tstamp to stack */
		skb_tstamp_tx(skb, &shhwtstamp);
	}
}

/* dn200_get_rx_hwtstamp - get HW RX timestamps
 * @priv: driver private structure
 * @p : descriptor pointer
 * @np : next descriptor pointer
 * @skb : the socket buffer
 * Description :
 * This function will read received packet's timestamp from the descriptor
 * and pass it to stack. It also perform some sanity checks.
 */
static void dn200_get_rx_hwtstamp(struct dn200_priv *priv, struct dma_desc *p,
				  struct dma_desc *np, struct sk_buff *skb)
{
	struct skb_shared_hwtstamps *shhwtstamp = NULL;
	struct dma_desc *desc = p;
	u64 ns = 0;

	if (!priv->hwts_rx_en || PRIV_IS_VF(priv))
		return;
	/* For GMAC4, the valid timestamp is from CTX next desc. */
	if (priv->plat->has_gmac4 || priv->plat->has_xgmac)
		desc = np;

	/* Check if timestamp is available */
	if (dn200_get_rx_timestamp_status(priv, p, np, priv->adv_ts)) {
		dn200_get_timestamp(priv, desc, priv->adv_ts, &ns);

		netdev_dbg(priv->dev, "get valid RX hw timestamp %llu\n", ns);
		shhwtstamp = skb_hwtstamps(skb);
		memset(shhwtstamp, 0, sizeof(struct skb_shared_hwtstamps));
		shhwtstamp->hwtstamp = ns_to_ktime(ns);
	} else {
		netdev_dbg(priv->dev, "cannot get RX hw timestamp\n");
	}
}

/**
 *  dn200_hwtstamp_set - control hardware timestamping.
 *  @dev: device pointer.
 *  @ifr: An IOCTL specific structure, that can contain a pointer to
 *  a proprietary structure used to pass information to the driver.
 *  Description:
 *  This function configures the MAC to enable/disable both outgoing(TX)
 *  and incoming(RX) packets time stamping based on user input.
 *  Return Value:
 *  0 on success and an appropriate -ve integer on failure.
 */
static int dn200_hwtstamp_set(struct net_device *dev, struct ifreq *ifr)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct hwtstamp_config config;
	u32 ptp_v2 = 0;
	u32 tstamp_all = 0;
	u32 ptp_over_ipv4_udp = 0;
	u32 ptp_over_ipv6_udp = 0;
	u32 ptp_over_ethernet = 0;
	u32 snap_type_sel = 0;
	u32 ts_master_en = 0;
	u32 ts_event_en = 0;

	if (PRIV_IS_VF(priv)) {
		netdev_err(priv->dev, "VF No support for HW time stamping\n");
		return -EOPNOTSUPP;
	}
	if (!(priv->dma_cap.time_stamp || priv->adv_ts)) {
		netdev_alert(priv->dev, "No support for HW time stamping\n");
		priv->hwts_tx_en = 0;
		priv->hwts_rx_en = 0;

		return -EOPNOTSUPP;
	}

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	netdev_dbg(priv->dev,
		   "%s config flags:0x%x, tx_type:0x%x, rx_filter:0x%x\n",
		   __func__, config.flags, config.tx_type, config.rx_filter);
	/* reserved for future extensions */
	if (config.flags)
		return -EINVAL;
	if (config.tx_type != HWTSTAMP_TX_OFF &&
	    config.tx_type != HWTSTAMP_TX_ON)
		return -ERANGE;
	if (priv->adv_ts) {
		switch (config.rx_filter) {
		case HWTSTAMP_FILTER_NONE:
			/* time stamp no incoming packet at all */
			config.rx_filter = HWTSTAMP_FILTER_NONE;
			break;
		case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
			/* PTP v1, UDP, any kind of event packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_EVENT;
			/* 'xmac' hardware can support Sync, Pdelay_Req and
			 * Pdelay_resp by setting bit14 and bits17/16 to 01
			 * This leaves Delay_Req timestamps out.
			 * Enable all events *and* general purpose message
			 * timestamping
			 */
			snap_type_sel = PTP_TCR_SNAPTYPSEL_1;
			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
			/* PTP v1, UDP, Sync packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_SYNC;
			/* take time stamp for SYNC messages only */
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
			/* PTP v1, UDP, Delay_req packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ;
			/* take time stamp for Delay_Req messages only */
			ts_master_en = PTP_TCR_TSMSTRENA;
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
			/* PTP v2, UDP, any kind of event packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			/* take time stamp for all event messages */
			snap_type_sel = PTP_TCR_SNAPTYPSEL_1;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
			/* PTP v2, UDP, Sync packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			/* take time stamp for SYNC messages only */
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
			/* PTP v2, UDP, Delay_req packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			/* take time stamp for Delay_Req messages only */
			ts_master_en = PTP_TCR_TSMSTRENA;
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			break;
		case HWTSTAMP_FILTER_PTP_V2_EVENT:
			/* PTP v2/802.AS1 any layer, any kind of event packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			snap_type_sel = PTP_TCR_SNAPTYPSEL_1;
			if (priv->chip_id < DWMAC_CORE_4_10)
				ts_event_en = PTP_TCR_TSEVNTENA;
			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			ptp_over_ethernet = PTP_TCR_TSIPENA;
			tstamp_all = PTP_TCR_TSENALL;
			break;
		case HWTSTAMP_FILTER_PTP_V2_SYNC:
			/* PTP v2/802.AS1, any layer, Sync packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_SYNC;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			/* take time stamp for SYNC messages only */
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			ptp_over_ethernet = PTP_TCR_TSIPENA;
			break;
		case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
			/* PTP v2/802.AS1, any layer, Delay_req packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V2_DELAY_REQ;
			ptp_v2 = PTP_TCR_TSVER2ENA;
			/* take time stamp for Delay_Req messages only */
			ts_master_en = PTP_TCR_TSMSTRENA;
			ts_event_en = PTP_TCR_TSEVNTENA;

			ptp_over_ipv4_udp = PTP_TCR_TSIPV4ENA;
			ptp_over_ipv6_udp = PTP_TCR_TSIPV6ENA;
			ptp_over_ethernet = PTP_TCR_TSIPENA;
			break;
		case HWTSTAMP_FILTER_NTP_ALL:
		case HWTSTAMP_FILTER_ALL:
			/* time stamp any incoming packet */
			config.rx_filter = HWTSTAMP_FILTER_ALL;
			tstamp_all = PTP_TCR_TSENALL;
			break;
		default:
			return -ERANGE;
		}
	} else {
		switch (config.rx_filter) {
		case HWTSTAMP_FILTER_NONE:
			config.rx_filter = HWTSTAMP_FILTER_NONE;
			break;
		default:
			/* PTP v1, UDP, any kind of event packet */
			config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_EVENT;
			break;
		}
	}

	priv->systime_flags = DN200_HWTS_ACTIVE;

	if (priv->hwts_tx_en || priv->hwts_rx_en) {
		priv->systime_flags |= tstamp_all | ptp_v2 |
		    ptp_over_ethernet | ptp_over_ipv6_udp |
		    ptp_over_ipv4_udp | ts_event_en |
		    ts_master_en | snap_type_sel;
	}

	dn200_config_hw_tstamping(priv, priv->ptpaddr, priv->systime_flags);

	memcpy(&priv->tstamp_config, &config, sizeof(config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config))
	    ? -EFAULT : 0;
}

/**
 *  dn200_hwtstamp_get - read hardware timestamping.
 *  @dev: device pointer.
 *  @ifr: An IOCTL specific structure, that can contain a pointer to
 *  a proprietary structure used to pass information to the driver.
 *  Description:
 *  This function obtain the current hardware timestamping settings
 *  as requested.
 */
static int dn200_hwtstamp_get(struct net_device *dev, struct ifreq *ifr)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct hwtstamp_config *config = &priv->tstamp_config;

	if (PRIV_IS_VF(priv)) {
		netdev_err(priv->dev, "VF No support for HW time stamping\n");
		return -EOPNOTSUPP;
	}
	if (!(priv->dma_cap.time_stamp || priv->dma_cap.atime_stamp))
		return -EOPNOTSUPP;

	return copy_to_user(ifr->ifr_data, config, sizeof(*config))
	    ? -EFAULT : 0;
}

/**
 * dn200_init_tstamp_counter - init hardware timestamping counter
 * @priv: driver private structure
 * @systime_flags: timestamping flags
 * Description:
 * Initialize hardware counter for packet timestamping.
 * This is valid as long as the interface is open and not suspended.
 * Will be rerun after resuming from suspend, case in which the timestamping
 * flags updated by dn200_hwtstamp_set() also need to be restored.
 */
int dn200_init_tstamp_counter(struct dn200_priv *priv, u32 systime_flags)
{
	bool xmac = priv->plat->has_gmac4 || priv->plat->has_xgmac;
	struct timespec64 now;
	u32 sec_inc = 0;
	u64 temp = 0;

	if (!(priv->dma_cap.time_stamp || priv->dma_cap.atime_stamp))
		return -EOPNOTSUPP;

	dn200_config_hw_tstamping(priv, priv->ptpaddr, systime_flags);
	priv->systime_flags = systime_flags;

	/* program Sub Second Increment reg */
	dn200_config_sub_second_increment(priv, priv->ptpaddr,
					  priv->plat->clk_ptp_rate,
					  xmac, &sec_inc);
	temp = div_u64(1000000000ULL, sec_inc);

	/* Store sub second increment for later use */
	priv->sub_second_inc = sec_inc;

	/* calculate default added value:
	 * formula is :
	 * addend = (2^32)/freq_div_ratio;
	 * where, freq_div_ratio = 1e9ns/sec_inc
	 */
	temp = (u64)(temp << 32);
	priv->default_addend = div_u64(temp, priv->plat->clk_ptp_rate);
	dn200_config_addend(priv, priv->ptpaddr, priv->default_addend);

	/* initialize system time */
	ktime_get_real_ts64(&now);

	/* lower 32 bits of tv_sec are safe until y2106 */
	dn200_init_systime(priv, priv->ptpaddr, (u32)now.tv_sec, now.tv_nsec);

	return 0;
}
EXPORT_SYMBOL_GPL(dn200_init_tstamp_counter);

/**
 * dn200_init_ptp - init PTP
 * @priv: driver private structure
 * Description: this is to verify if the HW supports the PTPv1 or PTPv2.
 * This is done by looking at the HW cap. register.
 * This function also registers the ptp driver.
 */
static int dn200_init_ptp(struct dn200_priv *priv)
{
	bool xmac = priv->plat->has_gmac4 || priv->plat->has_xgmac;
	int ret;

	if (PRIV_IS_VF(priv))
		return -EOPNOTSUPP;
	ret = dn200_init_tstamp_counter(priv, DN200_HWTS_ACTIVE);
	if (ret)
		return ret;

	priv->adv_ts = 0;
	/* Check if adv_ts can be enabled for dwmac 4.x / xgmac core */
	if (xmac && priv->dma_cap.atime_stamp)
		priv->adv_ts = 1;

	if (priv->adv_ts) {
		netdev_dbg(priv->dev,
				"IEEE 1588-2008 Advanced Timestamp supported\n");
	}

	return 0;
}

static void dn200_release_ptp(struct dn200_priv *priv)
{
	clk_disable_unprepare(priv->plat->clk_ptp_ref);
	dn200_ptp_unregister(priv);
}

/**
 *  dn200_mac_flow_ctrl - Configure flow control in all queues
 *  @priv: driver private structure
 *  @duplex: duplex passed to the next function
 *  Description: It is used for configuring the flow control in all queues
 */
static void dn200_mac_flow_ctrl(struct dn200_priv *priv, u32 duplex)
{
	u32 tx_cnt = priv->plat->tx_queues_to_use;

	priv->duplex = duplex;
	dn200_flow_ctrl(priv, priv->hw, priv->duplex, priv->flow_ctrl,
			priv->pause, tx_cnt);
}

static void dn200_xgmac_halfduplex_set(struct dn200_priv *priv, int duplex,
				       phy_interface_t interface)
{
	u32 old_ctrl, ctrl;

	if (priv->plat->has_xgmac) {
		if (unlikely(priv->plat->tx_queues_to_use == 1 &&
			     interface == PHY_INTERFACE_MODE_RGMII &&
			     (priv->speed == SPEED_10 ||
			      priv->speed == SPEED_100 ||
			      priv->speed == SPEED_1000))) {
			old_ctrl = readl(priv->ioaddr +
					 XGMAC_MAC_EXT_CONF); // offset 0x140
			ctrl = old_ctrl;
			if (!duplex)
				ctrl |= XGMAC_MACEXT_HD;
			else
				ctrl &= ~XGMAC_MACEXT_HD;
			if (ctrl != old_ctrl)
				writel(ctrl, priv->ioaddr + XGMAC_MAC_EXT_CONF);
		}
	}
}

static void tx_clean_by_loopback(struct dn200_priv *priv)
{
	u32 value = 0;
	u8 vf_carrier_state = 0;
	u8 vf_offset = 0;
	u8 reg_info = 0;
	u8 probe_bitmap = 0, wb_bitmap = 0;
	int i = 0;
	unsigned long in_time_start = jiffies;

	if (test_bit(DN200_DOWN, &priv->state))
		return;

	value = readl(priv->ioaddr + XGMAC_MAC_DEBUG);
	if (!value)
		return;

	if (!priv->plat_ex->sriov_cfg)
		goto loopback;

	DN200_ITR_SYNC_SET(priv->hw, pf_carrier, 0, 1);
	irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
	while (true) {
		probe_bitmap = 0;
		wb_bitmap = 0;
		// num_vf_probe = priv->plat_ex->pf.registered_vfs;
		for (vf_offset = 0; vf_offset < priv->plat_ex->pf.registered_vfs; vf_offset++) {
			DN200_HEARTBEAT_GET(priv->hw, registered_vf_state, vf_offset, &reg_info);
			if ((reg_info & DN200_VF_REG_STATE_OPENED))
				probe_bitmap |= (1 << vf_offset);

			DN200_ITR_SYNC_GET(priv->hw, vf_carrier, vf_offset,
								 &vf_carrier_state);
			if (vf_carrier_state) {
				wb_bitmap |= (1 << vf_offset);
				continue;
			}
			usleep_range(1000, 2000);
		}
		if ((wb_bitmap & probe_bitmap) == probe_bitmap) {
			for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++) {
				DN200_ITR_SYNC_SET(priv->hw, vf_carrier, i,
								 0);
			}
			DN200_ITR_SYNC_SET(priv->hw, pf_carrier, 0, 0);
			break;
		}

		if (time_after(jiffies, in_time_start + msecs_to_jiffies(500))) {
			netdev_dbg(priv->dev, "%s %d wb_bitmap %d probe_bitmap %d\n", __func__, __LINE__, wb_bitmap, probe_bitmap);
			netdev_warn(priv->dev, "%s,%d PF notify VF do link down timeout\n",
							__func__, __LINE__);
			for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++) {
				DN200_ITR_SYNC_SET(priv->hw, vf_carrier, i,
								 0);
			}
			DN200_ITR_SYNC_SET(priv->hw, pf_carrier, 0, 0);
			break;
		}
	}
loopback:
	dn200_set_mac_loopback(priv, priv->ioaddr, true);
	usleep_range(2000, 3000);
}

void dn200_normal_close_open(struct dn200_priv *priv)
{
	queue_work(priv->wq, &priv->service_task);
}

static void dn200_mac_link_down(struct dn200_priv *priv,
				unsigned int mode, phy_interface_t interface)
{
	set_bit(DN200_MAC_LINK_DOWN, &priv->state);
	queue_work(priv->wq, &priv->service_task);
	// dn200_wq_mac_link_down(priv);
}


static void dn200_wq_mac_link_down(struct dn200_priv *priv)
{
	if (priv->mii)
		/* set rgmii rx clock from soc */
		dn200_xgmac_rx_ext_clk_set(priv, false);
	tx_clean_by_loopback(priv);
	dn200_mac_set(priv, priv->ioaddr, false, priv->hw);
	priv->eee_active = false;
	priv->tx_lpi_enabled = false;
	priv->eee_enabled = dn200_eee_init(priv);
	dn200_set_eee_pls(priv, priv->hw, false);
	dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);
	memset(&priv->hw->set_state, 0, sizeof(struct dn200_set_state));
	clear_bit(DN200_MAC_LINK_DOWN, &priv->state);
}

static void dn200_set_itr_divisor(struct dn200_priv *priv, u32 speed)
{
	int i = 0;
	int itr_div = 0;

	switch (speed) {
	case SPEED_10000:
		itr_div = 512;
		break;
	default:
		itr_div = 64;
		break;
	}

	for (; i < MTL_MAX_RX_QUEUES; i++) {
		priv->rx_intr[i].itr_div = itr_div;
		priv->rx_intr[i].target_itr = 0x10;
		priv->rx_intr[i].current_itr = 0x10;
	}

	for (i = 0; i < MTL_MAX_TX_QUEUES; i++) {
		priv->tx_intr[i].itr_div = itr_div;
		priv->tx_intr[i].target_itr = 0x40;
		priv->tx_intr[i].current_itr = 0x40;
	}
}

static void dn200_rx_itr_update(struct dn200_itr_info *itr,
				struct dn200_priv *priv, u8 chan);
static void dn200_update_1G_speed_itr(struct dn200_itr_info *itr,
				struct dn200_priv *priv, u8 chan);
static const struct itr_update_ops dn200_itr_update_ops = {
	.dn200_rx_itr_update = dn200_rx_itr_update,
};

static const struct itr_update_ops dn200_itr_update_ops_1G = {
	.dn200_rx_itr_update = dn200_update_1G_speed_itr,
};

static inline void dn200_rx_itr_usec_update(struct dn200_priv *priv);
static void dn200_vf_mac_link_up(struct dn200_priv *priv,
				 struct phy_device *phy,
				 unsigned int mode, phy_interface_t interface,
				 int speed, int duplex,
				 bool tx_pause, bool rx_pause)
{
	priv->speed = speed;
	dn200_rx_itr_usec_update(priv);
	if (speed == SPEED_10000)
		priv->dn200_update_ops = &dn200_itr_update_ops;
	else
		priv->dn200_update_ops = &dn200_itr_update_ops_1G;
	dn200_set_itr_divisor(priv, speed);
}

static void dn200_xgmac_rx_ext_clk_set(struct dn200_priv *priv, bool external)
{
	u32 reg_val;

	/* disable clk_rx_180 and phy_clk_rx */
	reg_val =
	    readl(priv->ioaddr +
		  XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
	writel(reg_val & (~(BIT(4) | BIT(6))),
	       priv->ioaddr +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));

	/* set xgmac clk mux */
	reg_val =
	    readl(priv->ioaddr + XGE_XGMAC_CLK_MUX_CTRL(priv->plat_ex->funcid));
	if (external)
		reg_val &= ~(BIT(17));
	else
		reg_val |= BIT(17);
	writel(reg_val,
	       priv->ioaddr + XGE_XGMAC_CLK_MUX_CTRL(priv->plat_ex->funcid));

	/* enable clk_rx_180 and phy_clk_rx */
	reg_val =
	    readl(priv->ioaddr +
		  XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
	writel(reg_val | (BIT(4) | BIT(6)),
	       priv->ioaddr +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
}

static void dn200_xgmac_ge_tx_clk_set(struct dn200_priv *priv, u32 speed)
{
	u32 reg_val;

	/* disable clk_tx_div */
	reg_val =
	    readl(priv->ioaddr +
		  XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
	writel(reg_val & (~(BIT(3))),
	       priv->ioaddr +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));

	/* reset xge clk gen module */
	reg_val =
	    readl(priv->ioaddr +
			XGE_XGMAC_XPCS_SW_RST(priv->plat_ex->funcid));
	writel(reg_val & (~BIT(3)),
			priv->ioaddr +
			XGE_XGMAC_XPCS_SW_RST(priv->plat_ex->funcid));
	writel(reg_val | BIT(3),
			priv->ioaddr +
			XGE_XGMAC_XPCS_SW_RST(priv->plat_ex->funcid));

	/* set ge tx clk */
	reg_val =
	    readl(priv->ioaddr + XGE_XGMAC_CLK_TX_CTRL(priv->plat_ex->funcid));
	reg_val &= ~(BIT(1) | BIT(2) | BIT(3));

	switch (speed) {
	case SPEED_10:
		reg_val |= BIT(2);
		break;
	case SPEED_100:
		reg_val |= BIT(1);
		break;
	default:
		break;
	}
	writel(reg_val,
	       priv->ioaddr + XGE_XGMAC_CLK_TX_CTRL(priv->plat_ex->funcid));

	/* enable clk_tx_div */
	reg_val =
	    readl(priv->ioaddr +
		  XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
	writel(reg_val | BIT(3),
	       priv->ioaddr +
	       XGE_XGMAC_CLK_MUX_ENABLE_CTRL(priv->plat_ex->funcid));
}

static void dn200_xgmac_rgmii_speed_set(struct dn200_priv *priv,
					phy_interface_t interface, u32 speed)
{
	u32 ctrl;

	if (interface == PHY_INTERFACE_MODE_RGMII ||
	    interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    interface == PHY_INTERFACE_MODE_RGMII_RXID ||
	    interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		ctrl = readl(priv->ioaddr + MAC_CTRL_REG);
		ctrl &= ~priv->hw->link.speed_mask;

		switch (speed) {
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		case SPEED_100:
			ctrl |= priv->hw->link.speed100;
			break;
		case SPEED_10:
			ctrl |= priv->hw->link.speed10;
			break;
		default:
			return;
		}
		writel(ctrl, priv->ioaddr + MAC_CTRL_REG);
		dn200_xgmac_ge_tx_clk_set(priv, speed);
	}
}

static void dn200_mac_link_up(struct dn200_priv *priv,
			      struct phy_device *phy,
			      unsigned int mode, phy_interface_t interface,
			      int speed, int duplex,
			      bool tx_pause, bool rx_pause)
{
	u32 old_ctrl, ctrl;

	if (priv->mii)
		/* set rgmii rx clock from phy */
		dn200_xgmac_rx_ext_clk_set(priv, true);

	old_ctrl = readl(priv->ioaddr + MAC_CTRL_REG);
	ctrl = old_ctrl;

	dn200_set_itr_divisor(priv, speed);

	if (!duplex)
		ctrl &= ~priv->hw->link.duplex;
	else
		ctrl |= priv->hw->link.duplex;

	/* Flow Control operation */
	dn200_mac_flow_ctrl(priv, duplex);
	if (ctrl != old_ctrl)
		writel(ctrl, priv->ioaddr + MAC_CTRL_REG);

	/* Make sure that speed select has been completely written. */
	dma_wmb();
	dn200_xgmac_halfduplex_set(priv, duplex, interface);
	dn200_xgmac_rgmii_speed_set(priv, interface, speed);

	dn200_mac_set(priv, priv->ioaddr, true, priv->hw);
	if (phy && priv->dma_cap.eee) {
		priv->eee_active = PRIV_PHY_OPS(priv)->init_eee(PRIV_PHY_INFO(priv), 1) >= 0;
		priv->eee_enabled = dn200_eee_init(priv);
		priv->tx_lpi_enabled = priv->eee_enabled;
		dn200_set_eee_pls(priv, priv->hw, true);
	}

	dn200_mmc_err_clear(priv, priv->mmcaddr);

	if (speed == 10000) {
		priv->max_usecs = DN200_ITR_MAX_USECS;
		priv->min_usecs = DN200_ITR_MIN_USECS;
	} else {
		priv->max_usecs = dn200_riwt2usec(DN200_ITR_MAX_RWT, priv);
		priv->min_usecs = DN200_ITR_MIN_USECS_1G;
	}
	priv->speed = speed;
	dn200_rx_itr_usec_update(priv);
	if (speed == 10000)
		priv->dn200_update_ops = &dn200_itr_update_ops;
	else
		priv->dn200_update_ops = &dn200_itr_update_ops_1G;

	dn200_set_mac_loopback(priv, priv->ioaddr,
			       !!(priv->dev->features & NETIF_F_LOOPBACK));
}

static void dn200_mac_speed_set(struct dn200_priv *priv,
				phy_interface_t interface, int speed)
{
	u32 old_ctrl, ctrl;

	old_ctrl = readl(priv->ioaddr + MAC_CTRL_REG);
	ctrl = old_ctrl & ~priv->hw->link.speed_mask;
	if (interface == PHY_INTERFACE_MODE_XGMII) {
		switch (speed) {
		case SPEED_10000:
			ctrl |= priv->hw->link.xgmii.speed10000;
			break;
		case SPEED_2500:
			ctrl |= priv->hw->link.speed2500;
			break;
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		default:
			return;
		}
	} else {
		switch (speed) {
		case SPEED_2500:
			ctrl |= priv->hw->link.speed2500;
			break;
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		case SPEED_100:
			ctrl |= priv->hw->link.speed100;
			break;
		case SPEED_10:
			ctrl |= priv->hw->link.speed10;
			break;
		default:
			return;
		}
	}

	priv->speed = speed;
	dev_dbg(priv->device, "%s %d speed %d ctrl %#x\n", __func__, __LINE__,
		speed, ctrl);
	writel(ctrl, priv->ioaddr + MAC_CTRL_REG);
}

static const struct dn200_mac_ops dn200_phy_mac_ops = {
	.mac_link_down = dn200_mac_link_down,
	.mac_link_up = dn200_mac_link_up,
	.mac_speed_set = dn200_mac_speed_set,
};

static const struct dn200_mac_ops dn200_vf_phy_mac_ops = {
	.mac_link_up = dn200_vf_mac_link_up,
};

/**
 * dn200_check_pcs_mode - verify if RGMII/SGMII is supported
 * @priv: driver private structure
 * Description: this is to verify if the HW supports the PCS.
 * Physical Coding Sublayer (PCS) interface that can be used when the MAC is
 * configured for the TBI, RTBI, or SGMII PHY interface.
 */
static void dn200_check_pcs_mode(struct dn200_priv *priv)
{
	int interface = priv->plat->phy_interface;

	if (!priv->plat_ex->has_xpcs) {
		if (interface == PHY_INTERFACE_MODE_RGMII ||
		    interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    interface == PHY_INTERFACE_MODE_RGMII_RXID ||
		    interface == PHY_INTERFACE_MODE_RGMII_TXID) {
			netdev_dbg(priv->dev, "PCS RGMII support enabled\n");
			priv->hw->pcs = DN200_PCS_RGMII;
			if (!priv->plat->mac_port_sel_speed)
				priv->plat->mac_port_sel_speed = SPEED_1000;
		} else if (interface == PHY_INTERFACE_MODE_SGMII) {
			netdev_dbg(priv->dev, "PCS SGMII support enabled\n");
			priv->hw->pcs = DN200_PCS_SGMII;
		}
	}
}

/**
 * dn200_init_phy - PHY initialization
 * @dev: net device structure
 * Description: it initializes the driver's PHY state, and attaches the PHY
 * to the mac driver.
 *  Return value:
 *  0 on success
 */
static int dn200_init_phy(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	if (PRIV_PHY_INFO(priv) && PRIV_PHY_OPS(priv))
		return PRIV_PHY_OPS(priv)->init(PRIV_PHY_INFO(priv));
	return 0;
}

static void dn200_display_rx_rings(struct dn200_priv *priv)
{
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	unsigned int desc_size;
	void *head_rx;
	u32 queue;

	/* Display RX rings */
	for (queue = 0; queue < rx_cnt; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		pr_info("\tRX Queue %u rings\n", queue);

		head_rx = (void *)rx_q->dma_rx;
		desc_size = sizeof(struct dma_desc);
		/* Display RX ring */
		dn200_display_ring(priv, head_rx, priv->dma_rx_size, true,
				   rx_q->dma_rx_phy, desc_size, priv->hw);
	}
}

static void dn200_display_tx_rings(struct dn200_priv *priv)
{
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	unsigned int desc_size;
	void *head_tx;
	u32 queue;
	bool flags = true;
	/* Display TX rings */
	for (queue = 0; queue < tx_cnt; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];

		pr_info("\tTX Queue %d rings\n", queue);

		head_tx = (void *)tx_q->dma_tx;
		desc_size = sizeof(struct dma_desc);

		dn200_display_ring(priv, head_tx, priv->dma_tx_size, flags,
				   tx_q->dma_tx_phy, desc_size, priv->hw);
	}
}

static void dn200_display_rings(struct dn200_priv *priv)
{
	/* Display RX ring */
	dn200_display_rx_rings(priv);

	/* Display TX ring */
	dn200_display_tx_rings(priv);
}

/**
 * dn200_clear_rx_descriptors - clear RX descriptors
 * @priv: driver private structure
 * @queue: RX queue index
 * Description: this function is called to clear the RX descriptors
 * in case of both basic and extended descriptors are used.
 */
static void dn200_clear_rx_descriptors(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	int i;

	/* Clear the RX descriptors */
	for (i = 0; i < priv->dma_rx_size - 1; i++)
		dn200_init_rx_desc(priv, &rx_q->dma_rx[i],
				   priv->use_riwt, priv->mode,
				   (i == priv->dma_rx_size - 1),
				   priv->dma_buf_sz);
}

/**
 * dn200_clear_tx_descriptors - clear tx descriptors
 * @priv: driver private structure
 * @queue: TX queue index.
 * Description: this function is called to clear the TX descriptors
 * in case of both basic and extended descriptors are used.
 */
static void dn200_clear_tx_descriptors(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	int i;

	/* Clear the TX descriptors */
	for (i = 0; i < priv->dma_tx_size; i++) {
		int last = (i == (priv->dma_tx_size - 1));
		struct dma_desc *p;

		p = &tx_q->dma_tx[i];

		dn200_init_tx_desc(priv, p, priv->mode, last);
	}
}

/**
 * dn200_clear_descriptors - clear descriptors
 * @priv: driver private structure
 * Description: this function is called to clear the TX and RX descriptors
 * in case of both basic and extended descriptors are used.
 */
static void dn200_clear_descriptors(struct dn200_priv *priv)
{
	u32 rx_queue_cnt = priv->plat->rx_queues_to_use;
	u32 tx_queue_cnt = priv->plat->tx_queues_to_use;
	u32 queue;

	/* Clear the RX descriptors */
	for (queue = 0; queue < rx_queue_cnt; queue++)
		dn200_clear_rx_descriptors(priv, queue);

	/* Clear the TX descriptors */
	for (queue = 0; queue < tx_queue_cnt; queue++)
		dn200_clear_tx_descriptors(priv, queue);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
static int dn200_alloc_page(struct dn200_priv *priv,
			    struct dn200_rx_queue *rx_q,
			    struct dn200_rx_buffer *buf, char index, int offset,
			    int dma_rx_size)
{
	struct dn200_page_buf *pg_buf = NULL;

	pg_buf =
	    dn200_rx_pool_buf_alloc(rx_q->rx_pool, rx_q->queue_index, offset,
				    dma_rx_size);
	if (unlikely(!pg_buf))
		return -ENOMEM;

	buf->pg_buf = pg_buf;
	if (index == FIRST_PAGE) {
		buf->page = pg_buf->page;
		buf->desc_addr = pg_buf->desc_addr;
		buf->kernel_addr = pg_buf->kernel_addr;
		buf->page_offset = pg_buf->page_offset;
		buf->rx_times = 0;
	} else {
		buf->sec_page = pg_buf->page;
		buf->sec_addr = pg_buf->kernel_addr;
		buf->sec_page_offset = pg_buf->page_offset;
	}

	return 0;
}

#pragma GCC diagnostic pop
/**
 * dn200_init_rx_buffers - init the RX descriptor buffer.
 * @priv: driver private structure
 * @p: descriptor pointer
 * @i: descriptor index
 * @flags: gfp flag
 * @queue: RX queue index
 * Description: this function is called to allocate a receive buffer, perform
 * the DMA mapping and init the descriptor.
 */
static int dn200_init_rx_buffers(struct dn200_priv *priv, struct dma_desc *p,
				 int i, gfp_t flags, u32 queue)
{
	int ret;
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	struct dn200_rx_buffer *buf = &rx_q->buf_pool[i];

	if (!buf->page) {
		ret =
		    dn200_alloc_page(priv, rx_q, buf, FIRST_PAGE, i,
				     priv->dma_rx_size);
		if (ret)
			return ret;
		dn200_set_desc_addr(priv, p, buf->desc_addr + buf->page_offset,
				    priv->hw);
	}

	if (priv->sph && !buf->sec_page) {
		ret =
		    dn200_alloc_page(priv, rx_q, buf, SECON_PAGE, i,
				     priv->dma_rx_size);
		if (ret)
			return ret;
		dn200_set_desc_sec_addr(priv, p, buf->sec_addr, true, priv->hw);
	} else {
		buf->sec_page = NULL;
		dn200_set_desc_sec_addr(priv, p, buf->sec_addr, false,
					priv->hw);
	}

	if (priv->dma_buf_sz == BUF_SIZE_16KiB)
		dn200_init_desc3(priv, p);

	return 0;
}

static inline void dn200_free_dma32_tx_buffer(struct dn200_priv *priv,
					      struct dn200_tx_queue *tx_q,
					      int entry)
{
	if (tx_q->tx_dma32_bufs[entry].mem_type != DN200_DMA32)
		return;
	__free_pages(tx_q->tx_dma32_bufs[entry].page,
		     tx_q->tx_dma32_bufs[entry].order);

	dma_unmap_page(priv->device, tx_q->tx_dma32_bufs[entry].buf,
		       tx_q->tx_dma32_bufs[entry].len, DMA_TO_DEVICE);
	tx_q->tx_dma32_bufs[entry].mem_type = DN200_NORMAL;
	tx_q->tx_dma32_bufs[entry].page = NULL;
	tx_q->tx_dma32_bufs[entry].len = 0;
	tx_q->tx_dma32_bufs[entry].order = 0;
	if (tx_q->tx_dma32_bufs[entry].iatu_ref_ptr) {
		atomic_sub(1, tx_q->tx_dma32_bufs[entry].iatu_ref_ptr);
		tx_q->tx_dma32_bufs[entry].iatu_ref_ptr = NULL;
	}
}

/**
 * dn200_free_tx_buffer - free RX dma buffers
 * @priv: private structure
 * @queue: RX queue index
 * @i: buffer index.
 */
static void dn200_free_tx_buffer(struct dn200_priv *priv, u32 queue, int i)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];

	if (!tx_q->tx_skbuff_dma)
		return;

	if (tx_q->tx_skbuff_dma[i].buf &&
	    tx_q->tx_skbuff_dma[i].buf_type != DN200_TXBUF_T_XDP_TX) {
		if (tx_q->tx_skbuff_dma[i].map_as_page)
			dma_unmap_page(priv->device,
				       tx_q->tx_skbuff_dma[i].buf,
				       tx_q->tx_skbuff_dma[i].len,
				       DMA_TO_DEVICE);
		else
			dma_unmap_single(priv->device,
					 tx_q->tx_skbuff_dma[i].buf,
					 tx_q->tx_skbuff_dma[i].len,
					 DMA_TO_DEVICE);
	}
	dn200_free_dma32_tx_buffer(priv, tx_q, i);

	if (tx_q->tx_skbuff_dma[i].buf_type == DN200_TXBUF_T_XSK_TX)
		tx_q->xsk_frames_done++;

	if (tx_q->tx_skbuff[i] &&
	    tx_q->tx_skbuff_dma[i].buf_type == DN200_TXBUF_T_SKB) {
		dev_kfree_skb_any(tx_q->tx_skbuff[i]);
		tx_q->tx_skbuff[i] = NULL;
	}

	tx_q->tx_skbuff_dma[i].buf = 0;
	tx_q->tx_skbuff_dma[i].map_as_page = false;
	if (tx_q->tx_skbuff_dma[i].iatu_ref_ptr) {
		atomic_sub(1, tx_q->tx_skbuff_dma[i].iatu_ref_ptr);
		tx_q->tx_skbuff_dma[i].iatu_ref_ptr = NULL;
	}
}

static int dn200_alloc_rx_buffers(struct dn200_priv *priv, u32 queue,
				  gfp_t flags)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	int i;
	struct dma_desc *p;
	int ret;

	/* keep one buff as free otherwise we don't known the queue is empty or full */
	for (i = 0; i < priv->dma_rx_size - 1; i++) {
		p = rx_q->dma_rx + i;
		ret = dn200_init_rx_buffers(priv, p, i, flags, queue);
		if (ret)
			return ret;
		rx_q->buf_alloc_num++;
	}

	return 0;
}

/**
 * dma_free_rx_xskbufs - free RX dma buffers from XSK pool
 * @priv: private structure
 * @queue: RX queue index
 */
/**
 * __init_dma_rx_desc_rings - init the RX descriptor ring (per queue)
 * @priv: driver private structure
 * @queue: RX queue index
 * @flags: gfp flag.
 * Description: this function initializes the DMA RX descriptors
 * and allocates the socket buffers. It supports the chained and ring
 * modes.
 */
static int __init_dma_rx_desc_rings(struct dn200_priv *priv, u32 queue,
				    gfp_t flags)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	int ret;

	netif_dbg(priv, probe, priv->dev,
		  "(%s) dma_rx_phy=0x%08x\n", __func__, (u32)rx_q->dma_rx_phy);

	dn200_clear_rx_descriptors(priv, queue);
	ret = dn200_alloc_rx_buffers(priv, queue, flags);
	if (ret < 0)
		return -ENOMEM;
	rx_q->cur_rx = 0;
	rx_q->alloc_rx = rx_q->buf_alloc_num;
	rx_q->dirty_rx = rx_q->buf_alloc_num;

	/* Setup the chained descriptor addresses */
	if (priv->mode == DN200_CHAIN_MODE) {
		dn200_mode_init(priv, rx_q->dma_rx,
				rx_q->dma_rx_phy, priv->dma_rx_size);
	}

	return 0;
}

static int init_dma_rx_desc_rings(struct net_device *dev, gfp_t flags)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 queue;
	int ret;

	/* RX INITIALIZATION */
	netif_dbg(priv, probe, priv->dev,
		  "SKB addresses:\nskb\t\tskb data\tdma data\n");

	for (queue = 0; queue < rx_count; queue++) {
		ret = __init_dma_rx_desc_rings(priv, queue, flags);
		if (ret)
			goto err_init_rx_buffers;
	}

	return 0;

err_init_rx_buffers:
	while (queue >= 0) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		rx_q->buf_alloc_num = 0;
		if (queue == 0)
			break;

		queue--;
	}

	return ret;
}

/**
 * __init_dma_tx_desc_rings - init the TX descriptor ring (per queue)
 * @priv: driver private structure
 * @queue : TX queue index
 * Description: this function initializes the DMA TX descriptors
 * and allocates the socket buffers. It supports the chained and ring
 * modes.
 */
static int __init_dma_tx_desc_rings(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	int i;
	struct dma_desc *p;

	netif_dbg(priv, probe, priv->dev,
		  "(%s) dma_tx_phy=0x%08x\n", __func__, (u32)tx_q->dma_tx_phy);
	/* Setup the chained descriptor addresses */
	if (priv->mode == DN200_CHAIN_MODE) {
		if (!(tx_q->tbs & DN200_TBS_AVAIL))
			dn200_mode_init(priv, tx_q->dma_tx,
					tx_q->dma_tx_phy, priv->dma_tx_size);
	}
	for (i = 0; i < priv->dma_tx_size; i++) {
		p = tx_q->dma_tx + i;

		dn200_clear_desc(priv, p);

		tx_q->tx_skbuff_dma[i].buf = 0;
		tx_q->tx_skbuff_dma[i].map_as_page = false;
		tx_q->tx_skbuff_dma[i].len = 0;
		tx_q->tx_skbuff_dma[i].last_segment = false;
		tx_q->tx_skbuff_dma[i].iatu_ref_ptr = NULL;
		tx_q->tx_skbuff[i] = NULL;
	}

	tx_q->dirty_tx = 0;
	tx_q->cur_tx = 0;
	tx_q->mss = 0;
	tx_q->next_to_watch = -1;
	atomic_set(&tx_q->txtimer_running, 0);
	netdev_tx_reset_queue(netdev_get_tx_queue(priv->dev, queue));

	return 0;
}

static int init_dma_tx_desc_rings(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 tx_queue_cnt;
	u32 queue;

	tx_queue_cnt = priv->plat->tx_queues_to_use;

	for (queue = 0; queue < tx_queue_cnt; queue++)
		__init_dma_tx_desc_rings(priv, queue);

	return 0;
}

/**
 * init_dma_desc_rings - init the RX/TX descriptor rings
 * @dev: net device structure
 * @flags: gfp flag.
 * Description: this function initializes the DMA RX/TX descriptors
 * and allocates the socket buffers. It supports the chained and ring
 * modes.
 */
static int init_dma_desc_rings(struct net_device *dev, gfp_t flags)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret;

	ret = init_dma_rx_desc_rings(dev, flags);
	if (ret)
		return ret;

	ret = init_dma_tx_desc_rings(dev);

	dn200_clear_descriptors(priv);

	if (netif_msg_hw(priv))
		dn200_display_rings(priv);

	return ret;
}

/**
 * dma_free_tx_skbufs - free TX dma buffers
 * @priv: private structure
 * @queue: TX queue index
 */
static void dma_free_tx_skbufs(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	int i;

	tx_q->xsk_frames_done = 0;

	for (i = 0; i < priv->dma_tx_size; i++)
		dn200_free_tx_buffer(priv, queue, i);
}

/**
 * __free_dma_rx_desc_resources - free RX dma desc resources (per queue)
 * @priv: private structure
 * @queue: RX queue index
 */
static void __free_dma_rx_desc_resources(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

	if (rx_q->state_saved) {
		rx_q->state_saved = false;
		dev_kfree_skb(rx_q->state.skb);
		rx_q->state.skb = NULL;
	}
	rx_q->buf_alloc_num = 0;
	/* Free DMA regions of consistent memory previously allocated */
	dma_free_coherent(priv->device,
			  priv->dma_rx_size * sizeof(struct dma_desc),
			  rx_q->dma_rx, rx_q->origin_dma_rx_phy);

	kfree(rx_q->buf_pool);
	rx_q->buf_pool = NULL;
}

static void free_dma_rx_desc_resources(struct dn200_priv *priv)
{
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 queue;

	/* Free RX queue resources */
	for (queue = 0; queue < rx_count; queue++)
		__free_dma_rx_desc_resources(priv, queue);
}

/**
 * __free_dma_tx_desc_resources - free TX dma desc resources (per queue)
 * @priv: private structure
 * @queue: TX queue index
 */
static void __free_dma_tx_desc_resources(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	size_t size;
	void *addr;

	/* Release the DMA TX socket buffers */
	dma_free_tx_skbufs(priv, queue);
	size = sizeof(struct dma_desc);
	addr = tx_q->dma_tx;
	size *= priv->dma_tx_size;

	if (tx_q->origin_dma_tx_phy)
		dma_free_coherent(priv->device, size, addr,
				  tx_q->origin_dma_tx_phy);

	kfree(tx_q->tx_dma32_bufs);
	tx_q->tx_dma32_bufs = NULL;
	kfree(tx_q->tx_skbuff_dma);
	tx_q->tx_skbuff_dma = NULL;
	kfree(tx_q->tx_skbuff);
	tx_q->tx_skbuff = NULL;
}

static void free_dma_tx_desc_resources(struct dn200_priv *priv)
{
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue;

	/* Free TX queue resources */
	for (queue = 0; queue < tx_count; queue++)
		__free_dma_tx_desc_resources(priv, queue);
}

static inline void *dma32_alloc_coherent(struct device *device, size_t size,
					 dma_addr_t *dma_handle, gfp_t gfp)
{
	int ret;
	void *addr;

	ret = dma_set_mask_and_coherent(device, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(device, "Failed to set DMA 32 bit Mask\n");
		return NULL;
	}
	addr = dma_alloc_coherent(device, size, dma_handle, gfp);
	if (!addr)
		return NULL;

	ret = dma_set_mask_and_coherent(device, DMA_BIT_MASK(64));
	if (ret)
		dev_err(device, "Failed to set DMA 64 bit Mask\n");

	return addr;
}

/**
 * __alloc_dma_rx_desc_resources - alloc RX resources (per queue).
 * @priv: private structure
 * @queue: RX queue index
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int __alloc_dma_rx_desc_resources(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned int napi_id;

	rx_q->queue_index = queue;
	rx_q->priv_data = priv;
	rx_q->buf_pool =
		kcalloc(priv->dma_rx_size, sizeof(*rx_q->buf_pool), GFP_KERNEL);
	if (!rx_q->buf_pool)
		return -ENOMEM;

	rx_q->dma_rx = NULL;
	rx_q->dma_rx = dma_alloc_coherent(priv->device,
		priv->dma_rx_size * sizeof(struct dma_desc),
		&rx_q->origin_dma_rx_phy, GFP_KERNEL);
	if (!rx_q->dma_rx)
		return -ENOMEM;
	if (dn200_rx_iatu_find(rx_q->origin_dma_rx_phy, priv,
			       &rx_q->dma_rx_phy) < 0) {
		dev_dbg(priv->device,
			"%s %d alloc rx desc failed!!! queue %d dma_addr %#llx\n",
			__func__, __LINE__, queue, rx_q->origin_dma_rx_phy);
		if (rx_q->dma_rx)
			dma_free_coherent(priv->device,
				priv->dma_rx_size * sizeof(struct dma_desc),
				rx_q->dma_rx, rx_q->origin_dma_rx_phy);

		rx_q->dma_rx = dma32_alloc_coherent(priv->device,
			priv->dma_rx_size * sizeof(struct dma_desc),
			&rx_q->origin_dma_rx_phy, GFP_KERNEL);
		if (!rx_q->dma_rx)
			return -ENOMEM;
		if (dn200_rx_iatu_find(rx_q->origin_dma_rx_phy, priv,
				       &rx_q->dma_rx_phy) < 0) {
			dev_err(priv->device,
				"%s %d alloc dma32 rx desc failed!!! queue %d dma_addr %#llx\n",
				__func__, __LINE__, queue,
				rx_q->origin_dma_rx_phy);
			return -ENOMEM;
		}
	}

	if (queue < priv->plat->rx_queues_to_use) {
		if (priv->txrx_itr_combined)
			napi_id = ch->agg_napi.napi_id;
		else
			napi_id = ch->rx_napi.napi_id;
	}
	ch->rx_q = rx_q;
	return 0;
}

static int alloc_dma_rx_desc_resources(struct dn200_priv *priv)
{
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 queue;
	int ret;

	/* RX queues buffers and DMA */
	for (queue = 0; queue < rx_count; queue++) {
		ret = __alloc_dma_rx_desc_resources(priv, queue);
		if (ret)
			goto err_dma;
	}
	return 0;

err_dma:
	free_dma_rx_desc_resources(priv);

	return ret;
}

/**
 * __alloc_dma_tx_desc_resources - alloc TX resources (per queue).
 * @priv: private structure
 * @queue: TX queue index
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int __alloc_dma_tx_desc_resources(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	size_t size;
	void *addr = NULL;

	tx_q->queue_index = queue;
	tx_q->priv_data = priv;

	/*alloc tx bufs array to manage dma32 buffers */
	tx_q->tx_dma32_bufs = kcalloc(priv->dma_tx_size,
				      sizeof(*tx_q->tx_dma32_bufs), GFP_KERNEL);
	if (!tx_q->tx_dma32_bufs)
		return -ENOMEM;

	tx_q->tx_skbuff_dma = kcalloc(priv->dma_tx_size,
				      sizeof(*tx_q->tx_skbuff_dma), GFP_KERNEL);
	if (!tx_q->tx_skbuff_dma)
		return -ENOMEM;

	tx_q->tx_skbuff = kcalloc(priv->dma_tx_size,
				  sizeof(struct sk_buff *), GFP_KERNEL);
	if (!tx_q->tx_skbuff)
		return -ENOMEM;

	size = sizeof(struct dma_desc);

	size *= priv->dma_tx_size;

	addr = dma_alloc_coherent(priv->device, size,
				  &tx_q->origin_dma_tx_phy, GFP_KERNEL);
	if (!addr)
		return -ENOMEM;
	if (dn200_rx_iatu_find(tx_q->origin_dma_tx_phy, priv, &tx_q->dma_tx_phy)
	    < 0) {
		dev_dbg(priv->device,
			"%s alloc tx desc failed!!! queue %d dma_addr %#llx\n",
			__func__, queue, tx_q->origin_dma_tx_phy);
		if (addr)
			dma_free_coherent(priv->device, size, addr,
					  tx_q->origin_dma_tx_phy);
		addr =
		    dma32_alloc_coherent(priv->device, size,
					 &tx_q->origin_dma_tx_phy, GFP_KERNEL);
		if (!addr)
			return -ENOMEM;
		if (dn200_rx_iatu_find
		    (tx_q->origin_dma_tx_phy, priv, &tx_q->dma_tx_phy) < 0) {
			dev_err(priv->device,
				"%s alloc dma32 tx desc failed!!! queue %d dma_addr %#llx\n",
				__func__, queue, tx_q->origin_dma_tx_phy);
			return -ENOMEM;
		}
	}

	tx_q->dma_tx = addr;

	return 0;
}

static int alloc_dma_tx_desc_resources(struct dn200_priv *priv)
{
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue;
	int ret;

	/* TX queues buffers and DMA */
	for (queue = 0; queue < tx_count; queue++) {
		ret = __alloc_dma_tx_desc_resources(priv, queue);
		if (ret)
			goto err_dma;
	}
	return 0;

err_dma:
	free_dma_tx_desc_resources(priv);
	return ret;
}

/**
 * alloc_dma_desc_resources - alloc TX/RX resources.
 * @priv: private structure
 * Description: according to which descriptor can be used (extend or basic)
 * this function allocates the resources for TX and RX paths. In case of
 * reception, for example, it pre-allocated the RX socket buffer in order to
 * allow zero-copy mechanism.
 */
static int alloc_dma_desc_resources(struct dn200_priv *priv)
{
	int ret;

	ret = dn200_rx_pool_setup(priv);
	if (ret)
		goto rx_pool_err;
	/* RX Allocation */
	ret = alloc_dma_rx_desc_resources(priv);
	if (ret)
		goto rx_desc_err;
	ret = alloc_dma_tx_desc_resources(priv);
	if (ret)
		goto tx_desc_err;
	return 0;

tx_desc_err:
	free_dma_rx_desc_resources(priv);
rx_desc_err:
rx_pool_err:
	dn200_rx_pool_destory(priv);
	return ret;
}

/**
 * free_dma_desc_resources - free dma desc resources
 * @priv: private structure
 */
static void free_dma_desc_resources(struct dn200_priv *priv)
{
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 i;
	struct dn200_tx_queue *tx_q;
	struct dn200_rx_queue *rx_q;

	/* Release the DMA TX socket buffers */
	free_dma_tx_desc_resources(priv);

	/* Release the DMA RX socket buffers later
	 * to ensure all pending XDP_TX buffers are returned.
	 */
	free_dma_rx_desc_resources(priv);
	dn200_rx_pool_destory(priv);

	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		dn200_iatu_uninit(priv);
	for (i = 0; i < tx_count; i++) {
		tx_q = &priv->tx_queue[i];
		memset(tx_q, 0, sizeof(struct dn200_tx_queue));
	}
	for (i = 0; i < rx_count; i++) {
		rx_q = &priv->rx_queue[i];
		memset(rx_q, 0, sizeof(struct dn200_rx_queue));
	}
}

/**
 *  dn200_mac_enable_rx_queues - Enable MAC rx queues
 *  @priv: driver private structure
 *  Description: It is used for enabling the rx queues in the MAC
 */
static void dn200_mac_enable_rx_queues(struct dn200_priv *priv)
{
	u32 queue;
	u8 mode;
	u32 rx_queues_count = priv->plat->rx_queues_to_use;

	for (queue = 0; queue < rx_queues_count; queue++) {
		mode = priv->plat->rx_queues_cfg[queue].mode_to_use;
		dn200_rx_queue_enable(priv, priv->hw, mode, queue);
	}

	if (!PRIV_IS_VF(priv)) {
		/* broadcast & mutlicast put to last mtl queue and copy to all dma channels */
		queue = DN200_LAST_QUEUE(priv);
		dn200_rx_queue_enable(priv, priv->hw, MTL_QUEUE_DCB, queue);
	}
}

/**
 * dn200_start_rx_dma - start RX DMA channel
 * @priv: driver private structure
 * @chan: RX channel index
 * Description:
 * This starts a RX DMA channel
 */
static void dn200_start_rx_dma(struct dn200_priv *priv, u32 chan)
{
	netdev_dbg(priv->dev, "DMA RX processes started in channel %d\n", chan);
	dn200_start_rx(priv, priv->ioaddr, chan, priv->hw);
}

/**
 * dn200_start_tx_dma - start TX DMA channel
 * @priv: driver private structure
 * @chan: TX channel index
 * Description:
 * This starts a TX DMA channel
 */
static void dn200_start_tx_dma(struct dn200_priv *priv, u32 chan)
{
	netdev_dbg(priv->dev, "DMA TX processes started in channel %d\n", chan);
	dn200_start_tx(priv, priv->ioaddr, chan, priv->hw);
}

/**
 * dn200_stop_rx_dma - stop RX DMA channel
 * @priv: driver private structure
 * @chan: RX channel index
 * Description:
 * This stops a RX DMA channel
 */
static void dn200_stop_rx_dma(struct dn200_priv *priv, u32 chan)
{
	netdev_dbg(priv->dev, "DMA RX processes stopped in channel %d\n", chan);
	dn200_stop_rx(priv, priv->ioaddr, chan, priv->hw);
}

/**
 * dn200_stop_tx_dma - stop TX DMA channel
 * @priv: driver private structure
 * @chan: TX channel index
 * Description:
 * This stops a TX DMA channel
 */
static void dn200_stop_tx_dma(struct dn200_priv *priv, u32 chan)
{
	netdev_dbg(priv->dev, "DMA TX processes stopped in channel %d\n", chan);
	dn200_stop_tx(priv, priv->ioaddr, chan, priv->hw);
}

static void dn200_enable_all_dma_irq(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 dma_csr_ch = max(rx_channels_count, tx_channels_count);
	u32 chan;

	for (chan = 0; chan < dma_csr_ch; chan++) {
		struct dn200_channel *ch = &priv->channel[chan];
		unsigned long flags;

		spin_lock_irqsave(&ch->lock, flags);
		dn200_enable_dma_irq(priv, priv->ioaddr, chan, 1, 1, priv->hw);
		spin_unlock_irqrestore(&ch->lock, flags);
	}
}

/**
 * dn200_start_all_dma - start all RX and TX DMA channels
 * @priv: driver private structure
 * Description:
 * This starts all the RX and TX DMA channels
 */
void dn200_start_all_dma(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 chan = 0;

	for (chan = 0; chan < rx_channels_count; chan++)
		dn200_start_rx_dma(priv, chan);

	for (chan = 0; chan < tx_channels_count; chan++)
		dn200_start_tx_dma(priv, chan);
}

/**
 * dn200_stop_all_dma - stop all RX and TX DMA channels
 * @priv: driver private structure
 * Description:
 * This stops the RX and TX DMA channels
 */
void dn200_stop_all_dma(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 chan = 0;

	for (chan = 0; chan < rx_channels_count; chan++)
		dn200_stop_rx_dma(priv, chan);

	for (chan = 0; chan < tx_channels_count; chan++)
		dn200_stop_tx_dma(priv, chan);
}

/**
 * dn200_stop_vf_dma - stop all RX and TX DMA channels
 * @priv: driver private structure
 * Description:
 * This stops the RX and TX DMA channels
 */
static void dn200_stop_vf_dma(struct dn200_priv *priv, u8 vf_num)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 chan = 0, chan_start = 0;

	if (vf_num) {
		chan_start = priv->plat_ex->pf.vfs[vf_num - 1].rx_queue_start;
		rx_channels_count = priv->plat_ex->pf.vfs[vf_num - 1].rx_queues_num;
		tx_channels_count = priv->plat_ex->pf.vfs[vf_num - 1].tx_queues_num;
	} else {
		chan_start = 0;
	}
	for (chan = 0; chan < rx_channels_count; chan++)
		dn200_stop_rx_dma(priv, chan + chan_start);

	for (chan = 0; chan < tx_channels_count; chan++)
		dn200_stop_tx_dma(priv, chan + chan_start);
}

static void dn200_set_dma_operation_mode(struct dn200_priv *priv, u32 txmode,
					 u32 rxmode, u32 chan, u8 tc,
					 enum dn200_txrx_mode_dir set_dir);
/**
 *  dn200_dma_operation_mode - HW DMA operation mode
 *  @priv: driver private structure
 *  Description: it is used for configuring the DMA operation mode register in
 *  order to program the tx/rx DMA thresholds or Store-And-Forward mode.
 */
void dn200_dma_operation_mode(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 txmode = 0;
	u32 rxmode = 0;
	u32 chan = 0;

	if (priv->plat->force_sf_dma_mode || priv->plat->tx_coe) {
		/* In case of GMAC, SF mode can be enabled
		 * to perform the TX COE in HW. This depends on:
		 * 1) TX COE if actually supported
		 * 2) There is no bugged Jumbo frame support
		 *    that needs to not insert csum in the TDES.
		 */
		txmode = SF_DMA_MODE;
		/* clear rxmode store and forward (set to 0) and rxpbl=2,
		 * almost resolve the overflow issue
		 */
		rxmode = SF_DMA_MODE; /*SF_DMA_MODE; */
	} else {
		txmode = 64;
		rxmode = 0;	/*SF_DMA_MODE; */
	}

	/* configure all channels */
	for (chan = 0; chan < rx_channels_count; chan++) {
		dn200_set_dma_operation_mode(priv, txmode, rxmode, chan, chan,
					     DN200_SET_RX_MODE);

		dn200_set_dma_bfsize(priv, priv->ioaddr,
				     priv->dma_buf_sz, chan, priv->hw);
	}
	for (chan = 0; chan < tx_channels_count; chan++) {
		dn200_set_dma_operation_mode(priv, txmode, rxmode, chan, chan,
					     DN200_SET_TX_MODE);
	}
	/* pf set dma operation mode for all queues(vfs),
	 * when other vfs are tx/rx flow,
	 * one vf set self tx mode will cause tx timeout and pcie err
	 */
	if (PRIV_SRIOV_SUPPORT(priv)) {
		int vf_queue = priv->plat_ex->default_tx_queue_num + priv->plat_ex->max_vfs;

		for (chan = priv->plat_ex->default_tx_queue_num; chan < vf_queue;
			chan++) {
			dev_dbg(priv->device,
				"%s, %d, chan:%d, txmode:%d, rxmode:%d\n", __func__,
				__LINE__, chan, txmode, rxmode);
			/* To ensure all vfs run in same bandwidth, the method is:
			 * 1. map all vfs mtl queue to tc 0
			 * 2. all vfs use same weight confiugred in tc 0
			 */
			dn200_set_dma_operation_mode(priv, txmode, rxmode, chan, 0,
							DN200_SET_TX_MODE);
		}
	}
}


static inline void dn200_unmap_txbuff(struct dn200_priv *priv,
				      struct dn200_tx_queue *tx_q,
				      unsigned int entry)
{
	if (likely(tx_q->tx_skbuff_dma[entry].buf)) {
		if (tx_q->tx_skbuff_dma[entry].map_as_page)
			dma_unmap_page(priv->device,
				       tx_q->tx_skbuff_dma[entry].buf,
				       tx_q->tx_skbuff_dma[entry].len,
				       DMA_TO_DEVICE);
		else
			dma_unmap_single(priv->device,
					 tx_q->tx_skbuff_dma[entry].buf,
					 tx_q->tx_skbuff_dma[entry].len,
					 DMA_TO_DEVICE);
		tx_q->tx_skbuff_dma[entry].buf = 0;
		tx_q->tx_skbuff_dma[entry].len = 0;
		tx_q->tx_skbuff_dma[entry].map_as_page = false;
		if (tx_q->tx_skbuff_dma[entry].iatu_ref_ptr) {
			atomic_sub(1, tx_q->tx_skbuff_dma[entry].iatu_ref_ptr);
			tx_q->tx_skbuff_dma[entry].iatu_ref_ptr = NULL;
		}
	}
}

/**
 * dn200_sw_tx_clean - to manage the transmission completion
 * @priv: driver private structure
 * @budget: napi budget limiting this functions packet handling
 * @queue: TX queue index
 * Description: it reclaims the transmit resources after transmission completes.
 */
static int dn200_sw_tx_clean(struct dn200_priv *priv, int budget, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	unsigned int bytes_compl = 0, pkts_compl = 0;
	unsigned int entry, xmits = 0, count = 0;
	unsigned int cur_tx = 0;

	cur_tx = tx_q->cur_tx;
	priv->xstats.tx_clean++;
	tx_q->xsk_frames_done = 0;
	entry = tx_q->dirty_tx;

	/* Try to clean all TX complete frame in 1 shot */
	while ((entry != cur_tx) && count < priv->dma_tx_size
	       /*&& count <= budget*/) {
		struct sk_buff *skb;
		struct dma_desc *p;
		int status;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();
		if (tx_q->tx_skbuff_dma[entry].buf_type == DN200_TXBUF_T_SKB)
			skb = tx_q->tx_skbuff[entry];
		else
			skb = NULL;
		p = tx_q->dma_tx + entry;
		status = dn200_tx_status(priv, &priv->dev->stats,
					 &priv->xstats, p, priv->ioaddr);
		count++;
		/* Make sure descriptor fields are read after reading
		 * the own bit.
		 */
		dma_rmb();

		/* Just consider the last segment and ... */
		if (likely(!(status & tx_not_ls))) {
			/* ... verify the status error condition */
			if (unlikely(status & tx_err)) {
				priv->dev->stats.tx_errors++;
			} else {
				priv->dev->stats.tx_packets++;
				priv->xstats.tx_pkt_n++;
				priv->xstats.txq_stats[queue].tx_pkt_n++;
			}
			if (skb)
				dn200_get_tx_hwtstamp(priv, p, skb);
		}

		dn200_unmap_txbuff(priv, tx_q, entry);
		dn200_free_dma32_tx_buffer(priv, tx_q, entry);
		dn200_clean_desc3(priv, tx_q, p);

		tx_q->tx_skbuff_dma[entry].last_segment = false;
		tx_q->tx_skbuff_dma[entry].is_jumbo = false;

		if (tx_q->tx_skbuff_dma[entry].buf_type == DN200_TXBUF_T_SKB) {
			if (likely(skb)) {
				pkts_compl++;
				bytes_compl += skb->len;
				napi_consume_skb(skb, budget);
				tx_q->tx_skbuff[entry] = NULL;
			}
		}

		dn200_release_tx_desc(priv, p, priv->mode);

		entry = DN200_GET_ENTRY(entry, priv->dma_tx_size);
	}
	tx_q->dirty_tx = entry;
	netdev_tx_completed_queue(netdev_get_tx_queue(priv->dev, queue),
				  pkts_compl, bytes_compl);

	if (unlikely(netif_tx_queue_stopped(netdev_get_tx_queue(priv->dev, queue))) &&
	    dn200_tx_avail(priv, queue) > DN200_TX_THRESH(priv)) {
		netif_dbg(priv, tx_done, priv->dev,
			  "%s: restart transmit\n", __func__);
		netif_tx_wake_queue(netdev_get_tx_queue(priv->dev, queue));
	}

	return max(count, xmits);
}

/**
 * dn200_tx_clean - to manage the transmission completion
 * @priv: driver private structure
 * @budget: napi budget limiting this functions packet handling
 * @queue: TX queue index
 * Description: it reclaims the transmit resources after transmission completes.
 */
int dn200_tx_clean(struct dn200_priv *priv, int budget, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	unsigned int bytes_compl = 0, pkts_compl = 0;
	unsigned int entry, xmits = 0, count = 0;
	int next_to_watch = tx_q->next_to_watch;

	/*The read barrier is used to protect next_to_watch*/
	smp_rmb();
	priv->xstats.tx_clean++;
	tx_q->xsk_frames_done = 0;
	entry = tx_q->dirty_tx;

	/* Try to clean all TX complete frame in 1 shot */
	while ((entry != next_to_watch) && count < priv->dma_tx_size
	       /*&& count <= budget*/) {
		struct sk_buff *skb;
		struct dma_desc *p;
		int status;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();
		if (tx_q->tx_skbuff_dma[entry].buf_type == DN200_TXBUF_T_SKB)
			skb = tx_q->tx_skbuff[entry];
		else
			skb = NULL;
		p = tx_q->dma_tx + entry;
		status = dn200_tx_status(priv, &priv->dev->stats,
					 &priv->xstats, p, priv->ioaddr);

		/* Check if the descriptor is owned by the DMA */
		if (unlikely(status & tx_dma_own))
			break;

		count++;
		/* Make sure descriptor fields are read after reading
		 * the own bit.
		 */
		dma_rmb();

		/* Just consider the last segment and ... */
		if (likely(!(status & tx_not_ls))) {
			/* ... verify the status error condition */
			if (unlikely(status & tx_err)) {
				priv->dev->stats.tx_errors++;
			} else {
				priv->dev->stats.tx_packets++;
				priv->xstats.tx_pkt_n++;
				priv->xstats.txq_stats[queue].tx_pkt_n++;
			}
			if (skb)
				dn200_get_tx_hwtstamp(priv, p, skb);
		}

		dn200_unmap_txbuff(priv, tx_q, entry);
		dn200_free_dma32_tx_buffer(priv, tx_q, entry);
		dn200_clean_desc3(priv, tx_q, p);

		tx_q->tx_skbuff_dma[entry].last_segment = false;
		tx_q->tx_skbuff_dma[entry].is_jumbo = false;

		if (tx_q->tx_skbuff_dma[entry].buf_type == DN200_TXBUF_T_SKB) {
			if (likely(skb)) {
				pkts_compl++;
				bytes_compl += skb->len;
				napi_consume_skb(skb, budget);
				tx_q->tx_skbuff[entry] = NULL;
			}
		}

		dn200_release_tx_desc(priv, p, priv->mode);

		entry = DN200_GET_ENTRY(entry, priv->dma_tx_size);
	}
	tx_q->dirty_tx = entry;
	netdev_tx_completed_queue(netdev_get_tx_queue(priv->dev, queue),
				  pkts_compl, bytes_compl);

	if (unlikely(netif_tx_queue_stopped(netdev_get_tx_queue(priv->dev, queue))) &&
	    dn200_tx_avail(priv, queue) > DN200_TX_THRESH(priv)) {
		netif_dbg(priv, tx_done, priv->dev,
			  "%s: restart transmit\n", __func__);
		netif_tx_wake_queue(netdev_get_tx_queue(priv->dev, queue));
	}
	if (priv->eee_enabled && !priv->tx_path_in_lpi_mode &&
	    priv->eee_sw_timer_en) {
		if (dn200_enable_eee_mode(priv))
			mod_timer(&priv->eee_ctrl_timer,
				  DN200_LPI_T(priv->tx_lpi_timer));
	}

	/* Combine decisions from TX clean and XSK TX */
	if (tx_q->cur_tx != tx_q->dirty_tx) {
		/* We still have pending packets, let's call for a new scheduling */
		hrtimer_start(&tx_q->txtimer,
					DN200_COAL_TIMER(priv->tx_coal_timer[queue] ? : 1),
					HRTIMER_MODE_REL);
		tx_q->txtimer_need_sch = true;
	} else {
		atomic_set(&tx_q->txtimer_running, 0);
		/*write barrier to protect shared value*/
		smp_wmb();
	}

	return max(count, xmits);
}

/**
 * dn200_tx_iatu_ref_clean - to manage the transmission completion
 */
void dn200_tx_iatu_ref_clean(struct dn200_priv *priv,
			     struct dn200_tx_queue *tx_q)
{
	unsigned int entry, count = 0;
	struct dma_desc *p;
	int status;

	entry = tx_q->dirty_tx;

	/* Try to clean all TX complete frame in 1 shot */
	while ((entry != tx_q->cur_tx) && count < priv->dma_tx_size) {
		p = tx_q->dma_tx + entry;
		status = dn200_tx_status(priv, &priv->dev->stats,
					 &priv->xstats, p, priv->ioaddr);
		/* Check if the descriptor is owned by the DMA */
		if (unlikely(status & tx_dma_own))
			break;

		count++;

		/*Only clean iatu ref count, other resource wait soft irq */
		if (tx_q->tx_skbuff_dma[entry].iatu_ref_ptr) {
			atomic_sub(1, tx_q->tx_skbuff_dma[entry].iatu_ref_ptr);
			tx_q->tx_skbuff_dma[entry].iatu_ref_ptr = NULL;
		}
		if (tx_q->tx_dma32_bufs[entry].iatu_ref_ptr) {
			atomic_sub(1, tx_q->tx_dma32_bufs[entry].iatu_ref_ptr);
			tx_q->tx_dma32_bufs[entry].iatu_ref_ptr = NULL;
		}

		entry = DN200_GET_ENTRY(entry, priv->dma_tx_size);
	}
}

/**
 * dn200_tx_err - to manage the tx error
 * @priv: driver private structure
 * @chan: channel index
 * Description: it cleans the descriptors and restarts the transmission
 * in case of transmission errors.
 */
static void dn200_tx_err(struct dn200_priv *priv, u32 chan)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[chan];

	netif_tx_stop_queue(netdev_get_tx_queue(priv->dev, chan));

	dn200_stop_tx_dma(priv, chan);
	dma_free_tx_skbufs(priv, chan);
	dn200_clear_tx_descriptors(priv, chan);
	tx_q->dirty_tx = 0;
	tx_q->cur_tx = 0;
	tx_q->mss = 0;
	netdev_tx_reset_queue(netdev_get_tx_queue(priv->dev, chan));
	dn200_init_tx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
			   tx_q->dma_tx_phy, chan, priv->hw);
	dn200_start_tx_dma(priv, chan);

	priv->dev->stats.tx_errors++;
	netif_tx_wake_queue(netdev_get_tx_queue(priv->dev, chan));
}

/**
 *  dn200_set_dma_operation_mode - Set DMA operation mode by channel
 *  @priv: driver private structure
 *  @txmode: TX operating mode
 *  @rxmode: RX operating mode
 *  @chan: channel index
 *  Description: it is used for configuring of the DMA operation mode in
 *  runtime in order to program the tx/rx DMA thresholds or Store-And-Forward
 *  mode.
 */
static void dn200_set_dma_operation_mode(struct dn200_priv *priv, u32 txmode,
					 u32 rxmode, u32 chan, u8 tc,
					 enum dn200_txrx_mode_dir set_dir)
{
	u8 rxqmode = MTL_QUEUE_DCB;
	u8 txqmode = MTL_QUEUE_DCB;
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	int rxfifosz = priv->plat->rx_fifo_size;
	int txfifosz = priv->plat->tx_fifo_size;

	if (PRIV_IS_VF(priv))
		return;
	/* just support 7 tcs */
	if (tc > 0x7) {
		dev_info(priv->device, "%s, %d, change input tc:%d to %d\n",
			 __func__, __LINE__, tc, tc & 0x7);
		tc &= 0x7;
	}

	if (rxfifosz == 0)
		rxfifosz = priv->dma_cap.rx_fifo_size;
	if (txfifosz == 0)
		txfifosz = priv->dma_cap.tx_fifo_size;

	/* Adjust for real per queue fifo size */
	rxfifosz /= rx_channels_count;
	if (tx_channels_count > 1)
		txfifosz = (txfifosz - priv->tx_fifo_queue_0) / (tx_channels_count - 1);

	if (set_dir & DN200_SET_TX_MODE) {
		/* queue 0 used for pf jumbo frame, give it larger fifo size,
		 * vf don't support jumbo
		 */
		if (chan == 0) {
			if (tx_channels_count != 1)
				txfifosz = priv->tx_fifo_queue_0;
			dn200_dma_tx_mode(priv, priv->ioaddr, txmode, chan,
						 txfifosz, txqmode,
						 tc, priv->hw);
		} else {
			dev_dbg(priv->device,
				"%s, %d, chan:%d, txfifosz:%d, txmode:%d, txqmode:%d\n",
				__func__, __LINE__, chan, txfifosz, txmode,
				txqmode);
			if (chan >= priv->plat_ex->default_tx_queue_num) /*set vf fifo*/
				txfifosz = priv->vf_tx_fifo_size;
			dn200_dma_tx_mode(priv, priv->ioaddr, txmode, chan,
					  txfifosz, txqmode, tc, priv->hw);
		}
	}

	if (set_dir & DN200_SET_RX_MODE) {
		/* queue 15 used for untag packets and tag prio 0 & 1 packets,
		 * so give it larger fifo size
		 */
		dn200_dma_rx_mode(priv, priv->ioaddr, rxmode, chan, priv->mtl_queue_fifo_avg,
				  rxqmode, priv->hw);

		/* DDS fifo size(DSS use last queue to copy broadcast &
		 * mutlicast pkts to all channel)
		 */
		if (!PRIV_IS_PUREPF(priv)) {
			chan = DN200_LAST_QUEUE(priv);
			dn200_dma_rx_mode(priv, priv->ioaddr, rxmode, chan, priv->mtl_queue_fifo_more,
					rxqmode, priv->hw);
		}
	}
}

static bool dn200_safety_feat_interrupt(struct dn200_priv *priv)
{
	int ret;

	ret = dn200_safety_feat_irq_status(priv, priv->dev,
					   priv->ioaddr, priv->dma_cap.asp,
					   &priv->sstats);
	if (ret && (ret != -EINVAL)) {
		dn200_global_err(priv, DN200_SAFETY_FEAT_INT);
		return true;
	}

	return false;
}

static int dn200_napi_check(struct dn200_priv *priv, u32 chan, u32 dir)
{
	int status = 0;
	struct dn200_channel *ch = &priv->channel[chan];
	struct napi_struct *rx_napi;
	struct napi_struct *tx_napi;
	struct napi_struct *agg_napi;
	struct dn200_itr_info *rx_intr;
	/*unsigned long flags; */

	if (!priv->plat->multi_msi_en) {
		status = dn200_dma_interrupt_status(priv, priv->ioaddr,
						    &priv->xstats, chan, dir,
						    priv->hw);
	} else {
		if (dir == DMA_DIR_RX)
			status = handle_rx;
		else if (dir == DMA_DIR_TX)
			status = handle_tx;
		else if (dir == DMA_DIR_RXTX)
			status = handle_rx | handle_tx;
	}

	if ((status == (handle_rx | handle_tx)) &&
	    (chan < priv->plat->rx_queues_to_use &&
			 chan < priv->plat->tx_queues_to_use) &&
				 priv->txrx_itr_combined) {
		agg_napi = &ch->agg_napi;
		if (napi_schedule_prep(agg_napi))
			__napi_schedule(agg_napi);
	} else {
		rx_napi = &ch->rx_napi;
		tx_napi = &ch->tx_napi;
		if ((status & handle_rx) &&
		    chan < priv->plat->rx_queues_to_use) {
			rx_intr = &priv->rx_intr[ch->index];
			if (napi_schedule_prep(rx_napi)) {
				if (rx_intr->itr_setting & DN200_ITR_DYNAMIC_ITR &&
						rx_intr->current_itr > DN200_ITR_RWT_BOUND)
					dn200_rx_watchdog(priv, priv->ioaddr,
						 DN200_ITR_MAX_RWT, ch->index, priv->hw);
				__napi_schedule(rx_napi);
			}
		}
		if ((status & handle_tx) &&
		    chan < priv->plat->tx_queues_to_use) {
			if (napi_schedule_prep(tx_napi)) {
				dn200_disable_tx_dma_irq(priv->ioaddr,
					 ch->index, priv->hw);
				__napi_schedule(tx_napi);
			}
		}
	}

	return status;
}

/**
 * dn200_dma_interrupt - DMA ISR
 * @priv: driver private structure
 * Description: this is the DMA ISR. It is called by the main ISR.
 * It calls the dwmac dma routine and schedule poll method in case of some
 * work can be done.
 */
static void dn200_dma_interrupt(struct dn200_priv *priv)
{
	u32 tx_channel_count = priv->plat->tx_queues_to_use;
	u32 rx_channel_count = priv->plat->rx_queues_to_use;
	u32 channels_to_check =
	    tx_channel_count >
	    rx_channel_count ? tx_channel_count : rx_channel_count;
	u32 chan;
	int status[DN200_CH_MAX];

	/* Make sure we never check beyond our status buffer. */
	if (WARN_ON_ONCE(channels_to_check > ARRAY_SIZE(status)))
		channels_to_check = ARRAY_SIZE(status);

	for (chan = 0; chan < channels_to_check; chan++)
		status[chan] = dn200_napi_check(priv, chan, DMA_DIR_RXTX);

	for (chan = 0; chan < tx_channel_count; chan++) {
		if (unlikely(status[chan] == tx_hard_error))
			dn200_tx_err(priv, chan);
	}
}

/**
 * dn200_mmc_setup: setup the Mac Management Counters (MMC)
 * @priv: driver private structure
 * Description: this masks the MMC irq, in fact, the counters are managed in SW.
 */
static void dn200_mmc_setup(struct dn200_priv *priv)
{
	unsigned int mode = MMC_CNTRL_RESET_ON_READ | MMC_CNTRL_COUNTER_RESET |
	    MMC_CNTRL_PRESET | MMC_CNTRL_FULL_HALF_PRESET;

	dn200_mmc_intr_all_mask(priv, priv->mmcaddr);

	if (priv->dma_cap.rmon) {
		dn200_mmc_ctrl(priv, priv->mmcaddr, mode);
	} else {
		netdev_info(priv->dev,
			    "No MAC Management Counters available\n");
	}
}

/**
 * dn200_check_hw_features_support - get MAC capabilities from the HW cap. register.
 * @priv: driver private structure
 * Description:
 *  new GMAC chip generations have a new register to indicate the
 *  presence of the optional feature/functions.
 *  This can be also used to override the value passed through the
 *  platform and necessary for old MAC10/100 and GMAC chips.
 */
static bool dn200_check_hw_features_support(struct dn200_priv *priv)
{
	int ret = 0;

	ret = dn200_get_hw_feature(priv, priv->ioaddr, &priv->dma_cap);
	dn200_sriov_reconfig_hw_feature(priv, &priv->dma_cap);

	if (!ret)
		return true;

	return false;
}

/**
 * dn200_check_ether_addr - check if the MAC addr is valid
 * @priv: driver private structure
 * Description:
 * it is to verify if the MAC address is valid, in case of failures it
 * generates a random MAC address
 */
static void dn200_check_ether_addr(struct dn200_priv *priv)
{
	if (!is_valid_ether_addr(priv->dev->dev_addr)) {
		eth_hw_addr_random(priv->dev);
		dev_info(priv->device, "device MAC address %pM\n",
			 priv->dev->dev_addr);
	} else {
		eth_hw_addr_set(priv->dev, priv->dev->dev_addr);
	}
}

/**
 * dn200_init_dma_engine - DMA init.
 * @priv: driver private structure
 * Description:
 * It inits the DMA invoking the specific MAC/GMAC callback.
 * Some DMA parameters can be passed from the platform;
 * in case of these are not passed a default is kept for the MAC or GMAC.
 */
static int dn200_init_dma_engine(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 dma_csr_ch = max(rx_channels_count, tx_channels_count);
	struct dn200_rx_queue *rx_q;
	struct dn200_tx_queue *tx_q;
	u32 chan = 0;
	int atds = 0;
	int ret = 0;

	if (!priv->plat->dma_cfg || !priv->plat->dma_cfg->pbl) {
		dev_err(priv->device, "Invalid DMA configuration\n");
		return -EINVAL;
	}

	/* DMA Configuration */
	dn200_dma_init(priv, priv->ioaddr, priv->plat->dma_cfg, atds, priv->hw);

	if (priv->plat->axi)
		dn200_axi(priv, priv->ioaddr, priv->plat->axi, priv->hw);

	/* DMA CSR Channel configuration */
	for (chan = 0; chan < dma_csr_ch; chan++) {
		dn200_init_chan(priv, priv->ioaddr, priv->plat->dma_cfg, chan,
				priv->hw);
		dn200_disable_dma_irq(priv, priv->ioaddr, chan, 1, 1, priv->hw);
	}

	/* DMA RX Channel Configuration */
	for (chan = 0; chan < rx_channels_count; chan++) {
		rx_q = &priv->rx_queue[chan];

		dn200_init_rx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
				   rx_q->dma_rx_phy, chan, priv->hw);

		rx_q->rx_tail_addr = rx_q->dma_rx_phy +
		    (rx_q->buf_alloc_num * sizeof(struct dma_desc));
		dn200_set_rx_tail_ptr(priv, priv->ioaddr,
				      rx_q->rx_tail_addr, chan, priv->hw);
	}

	/* DMA TX Channel Configuration */
	for (chan = 0; chan < tx_channels_count; chan++) {
		tx_q = &priv->tx_queue[chan];

		dn200_init_tx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
				   tx_q->dma_tx_phy, chan, priv->hw);

		tx_q->tx_tail_addr = tx_q->dma_tx_phy;
		dn200_set_tx_tail_ptr(priv, priv->ioaddr,
				      tx_q->tx_tail_addr, chan, priv->hw);
	}

	return ret;
}

static inline void dn200_tx_timer_arm(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	u32 cpuid = 0;

	cpuid = find_last_bit(cpumask_bits(irq_get_affinity_mask(priv->tx_irq[tx_q->queue_index])),
						nr_cpumask_bits);
	queue_work_on(cpuid, priv->tx_wq, &tx_q->tx_task);
	tx_q->task_need_sch = true;
	atomic_set(&tx_q->txtimer_running, 1);
}

static inline void dn200_tx_timer_poll(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	u32 cpuid = 0;

	cpuid = find_last_bit(cpumask_bits(irq_get_affinity_mask(priv->tx_irq[tx_q->queue_index])),
						nr_cpumask_bits);
	queue_work_on(cpuid, priv->tx_wq, &tx_q->poll_tx_task);
}

static inline void dn200_rx_timer_poll(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	u32 cpuid = 0;

	cpuid = find_last_bit(cpumask_bits(irq_get_affinity_mask(priv->rx_irq[rx_q->queue_index])),
						nr_cpumask_bits);
	queue_work_on(cpuid, priv->tx_wq, &rx_q->poll_rx_task);
}

/**
 * dn200_tx_timer - mitigation sw timer for tx.
 * @t: data pointer
 * Description:
 * This is the timer handler to directly invoke the dn200_tx_clean.
 */
static enum hrtimer_restart dn200_tx_timer(struct hrtimer *t)
{
	struct dn200_tx_queue *tx_q =
	    container_of(t, struct dn200_tx_queue, txtimer);
	struct dn200_priv *priv = tx_q->priv_data;
	struct dn200_channel *ch;
	struct napi_struct *napi;

	tx_q->txtimer_need_sch = false;
	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return HRTIMER_NORESTART;

	ch = &priv->channel[tx_q->queue_index];
	if (priv->txrx_itr_combined)
		napi = tx_q->xsk_pool ? &ch->rxtx_napi : &ch->agg_napi;
	else
		napi = tx_q->xsk_pool ? &ch->rxtx_napi : &ch->tx_napi;

	if (likely(napi_schedule_prep(napi)))
		__napi_schedule(napi);

	return HRTIMER_NORESTART;
}

static void dn200_tx_task(struct work_struct *work)
{
	struct dn200_tx_queue *tx_q = container_of(work, struct dn200_tx_queue,
						   tx_task);
	struct dn200_priv *priv = tx_q->priv_data;

	if (unlikely(test_bit(DN200_DOWN, &priv->state) || tx_q->txtimer.function == NULL)) {
		netdev_info(priv->dev, "%s chan %d\n", __func__, tx_q->queue_index);
		return;
	}

	hrtimer_start(&tx_q->txtimer,
		      DN200_COAL_TIMER(priv->tx_coal_timer[tx_q->queue_index] ? : 1),
		      HRTIMER_MODE_REL);
	tx_q->task_need_sch = false;
	tx_q->txtimer_need_sch = true;
}

/**
 * dn200_tx_timer - mitigation sw timer for tx.
 * @t: data pointer
 * Description:
 * This is the timer handler to directly invoke the dn200_tx_clean.
 */
static enum hrtimer_restart dn200_poll_tx_timer(struct hrtimer *t)
{
	struct dn200_tx_queue *tx_q =
	    container_of(t, struct dn200_tx_queue, poll_txtimer);
	struct dn200_priv *priv = tx_q->priv_data;
	struct dn200_channel *ch;
	struct napi_struct *napi;

	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return HRTIMER_NORESTART;

	ch = &priv->channel[tx_q->queue_index];
	if (priv->txrx_itr_combined)
		napi = tx_q->xsk_pool ? &ch->rxtx_napi : &ch->agg_napi;
	else
		napi = tx_q->xsk_pool ? &ch->rxtx_napi : &ch->tx_napi;

	if (likely(napi_schedule_prep(napi)))
		__napi_schedule(napi);

	return HRTIMER_NORESTART;
}

static void dn200_poll_tx_task(struct work_struct *work)
{
	struct dn200_tx_queue *tx_q = container_of(work, struct dn200_tx_queue,
						   poll_tx_task);
	struct dn200_priv *priv = tx_q->priv_data;

	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return;
	hrtimer_start(&tx_q->txtimer,
		      DN200_POLL_TIMER(2), HRTIMER_MODE_REL);
}

/**
 * dn200_tx_timer - mitigation sw timer for tx.
 * @t: data pointer
 * Description:
 * This is the timer handler to directly invoke the dn200_tx_clean.
 */
static enum hrtimer_restart dn200_poll_rx_timer(struct hrtimer *t)
{
	struct dn200_rx_queue *rx_q =
	    container_of(t, struct dn200_rx_queue, poll_rxtimer);
	struct dn200_priv *priv = rx_q->priv_data;
	struct dn200_channel *ch;
	struct napi_struct *napi;

	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return HRTIMER_NORESTART;

	ch = &priv->channel[rx_q->queue_index];
	if (priv->txrx_itr_combined)
		napi = &ch->agg_napi;
	else
		napi = &ch->rx_napi;

	if (likely(napi_schedule_prep(napi)))
		__napi_schedule(napi);

	return HRTIMER_NORESTART;
}

static void dn200_poll_rx_task(struct work_struct *work)
{
	struct dn200_rx_queue *rx_q = container_of(work, struct dn200_rx_queue,
						   poll_rx_task);
	struct dn200_priv *priv = rx_q->priv_data;

	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return;
	hrtimer_start(&rx_q->poll_rxtimer,
		      DN200_POLL_TIMER(2), HRTIMER_MODE_REL);
}

/**
 * dn200_init_coalesce - init mitigation options.
 * @priv: driver private structure
 * Description:
 * This inits the coalesce parameters: i.e. timer rate,
 * timer handler and default threshold used for enabling the
 * interrupt on completion bit.
 */
static void dn200_init_coalesce(struct dn200_priv *priv)
{
	u32 tx_channel_count = priv->plat->tx_queues_to_use;
	u32 rx_channel_count = priv->plat->rx_queues_to_use;
	u32 chan;
	struct dn200_channel *ch;

	for (chan = 0; chan < tx_channel_count; chan++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[chan];

		ch = &priv->channel[chan];
		if (!priv->tx_coal_frames_set[chan] ||
		    (priv->tx_coal_frames_set[chan] >
		     ((u32)(priv->dma_tx_size >> 1))))
			priv->tx_coal_frames_set[chan] =
			    min((u32)(priv->dma_tx_size >> 1),
				(u32)DN200_TX_FRAMES);
		priv->tx_coal_timer[chan] = DN200_COAL_TX_TIMER;

		hrtimer_init(&tx_q->txtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		tx_q->txtimer.function = dn200_tx_timer;

		INIT_WORK(&tx_q->tx_task, dn200_tx_task);

		hrtimer_init(&tx_q->poll_txtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		tx_q->poll_txtimer.function = dn200_poll_tx_timer;

		INIT_WORK(&tx_q->poll_tx_task, dn200_poll_tx_task);
	}

	for (chan = 0; chan < rx_channel_count; chan++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[chan];

		if (!priv->rx_coal_frames[chan])
			priv->rx_coal_frames[chan] = DN200_RX_FRAMES;

		if ((priv->rx_coal_frames[chan] + dn200_rx_refill_size(priv)) >=
		    priv->dma_rx_size) {
			netdev_warn(priv->dev,
				    "change queue %d rx-frames to %d\n", chan,
				    DN200_RX_MIN_FRAMES);
			priv->rx_coal_frames[chan] = DN200_RX_MIN_FRAMES;
		}

		hrtimer_init(&rx_q->poll_rxtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		rx_q->poll_rxtimer.function = dn200_poll_rx_timer;

		INIT_WORK(&rx_q->poll_rx_task, dn200_poll_rx_task);
	}
}

static void dn200_set_rings_length(struct dn200_priv *priv)
{
	u32 rx_channels_count = priv->plat->rx_queues_to_use;
	u32 tx_channels_count = priv->plat->tx_queues_to_use;
	u32 chan;

	/* set TX ring length */
	for (chan = 0; chan < tx_channels_count; chan++)
		dn200_set_tx_ring_len(priv, priv->ioaddr,
				      (priv->dma_tx_size - 1), chan, priv->hw);

	/* set RX ring length */
	for (chan = 0; chan < rx_channels_count; chan++)
		dn200_set_rx_ring_len(priv, priv->ioaddr,
				      (priv->dma_rx_size - 1), chan, priv->hw);
}

/**
 *  dn200_set_tx_queue_weight - Set TX queue weight
 *  @priv: driver private structure
 *  Description: It is used for setting TX queues weight
 */
static void dn200_set_tx_queue_weight(struct dn200_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 weight;
	u32 queue;

	for (queue = 0; queue < tx_queues_count; queue++) {
		weight = priv->plat->tx_queues_cfg[queue].weight;
		dn200_set_mtl_tx_queue_weight(priv, priv->hw, weight, queue);
	}
}

/**
 *  dn200_set_rx_queue_weight - Set RX queue weight
 *  @priv: driver private structure
 *  Description: It is used for setting RX queues weight
 */
static void dn200_set_rx_queue_weight(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 weight;
	u32 queue;

	for (queue = 0; queue < rx_queues_count; queue++) {
		weight = priv->plat->rx_queues_cfg[queue].weight;
		dn200_set_mtl_rx_queue_weight(priv, priv->hw, weight, queue);
	}
}

/**
 *  dn200_configure_cbs - Configure CBS in TX queue
 *  @priv: driver private structure
 *  Description: It is used for configuring CBS in AVB TX queues
 */
static void dn200_configure_cbs(struct dn200_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 mode_to_use;
	u32 queue;

	/* queue 0 is reserved for legacy traffic */
	for (queue = 1; queue < tx_queues_count; queue++) {
		mode_to_use = priv->plat->tx_queues_cfg[queue].mode_to_use;
		if (mode_to_use == MTL_QUEUE_DCB)
			continue;

		dn200_config_cbs(priv, priv->hw,
				 priv->plat->tx_queues_cfg[queue].send_slope,
				 priv->plat->tx_queues_cfg[queue].idle_slope,
				 priv->plat->tx_queues_cfg[queue].high_credit,
				 priv->plat->tx_queues_cfg[queue].low_credit,
				 queue);
	}
}

/**
 *  dn200_rx_queue_dma_chan_map - Map RX queue to RX dma channel
 *  @priv: driver private structure
 *  Description: It is used for mapping RX queues to RX dma channels
 */
static void dn200_rx_queue_dma_chan_map(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue;
	u32 chan;

	for (queue = 0; queue < rx_queues_count; queue++) {
		chan = priv->plat->rx_queues_cfg[queue].chan;
		dn200_map_mtl_to_dma(priv, priv->hw, queue, chan);
	}
}

/**
 *  dn200_mac_config_rx_queues_prio - Configure RX Queue priority
 *  @priv: driver private structure
 *  Description: It is used for configuring the RX Queue Priority
 */
static void dn200_mac_config_rx_queues_prio(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue;
	u32 prio;

	for (queue = 0; queue < rx_queues_count; queue++) {
		if (!priv->plat->rx_queues_cfg[queue].use_prio)
			continue;

		prio = priv->plat->rx_queues_cfg[queue].prio;
		dn200_rx_queue_prio(priv, priv->hw, prio, queue);
	}
}

/**
 *  dn200_mac_config_tx_queues_prio - Configure TX Queue priority
 *  @priv: driver private structure
 *  Description: It is used for configuring the TX Queue Priority
 */
static void dn200_mac_config_tx_queues_prio(struct dn200_priv *priv)
{
	u32 tx_queues_count = priv->plat->tx_queues_to_use;
	u32 queue;
	u32 prio;

	for (queue = 0; queue < tx_queues_count; queue++) {
		if (!priv->plat->tx_queues_cfg[queue].use_prio)
			continue;

		prio = priv->plat->tx_queues_cfg[queue].prio;
		dn200_tx_queue_prio(priv, priv->hw, prio, queue);
	}
}

/**
 *  dn200_mac_config_rx_queues_routing - Configure RX Queue Routing
 *  @priv: driver private structure
 *  Description: It is used for configuring the RX queue routing
 */
static void dn200_mac_config_rx_queues_routing(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue;
	u8 packet;

	for (queue = 0; queue < rx_queues_count; queue++) {
		/* no specific packet type routing specified for the queue */
		if (priv->plat->rx_queues_cfg[queue].pkt_route == 0x0)
			continue;

		packet = priv->plat->rx_queues_cfg[queue].pkt_route;
		dn200_rx_queue_routing(priv, priv->hw, packet, queue);
	}
}

/**
 *  dn200_rx_queue_dma_chan_dynamic_map - Map RX queue to RX dma channel as dynamic
 *  @priv: driver private structure
 *  Description: It is used for mapping RX MTL queues to RX dma channels as dynamic
 */
static void dn200_rx_queue_dma_chan_dynamic_map(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 queue;

	bool dynamic = true;

	/* if dynamic is true: set all rx queues as dynamic
	 * if dynamic is false: set all rx queues as static mapping,
	 *        mtl queue N statically map to dma channel N
	 *        e.g. enable 4 mtl queues, just 0~3 dma channel or desc rings can receive pkts
	 *        1. untag pkts route to queue 0,
	 *        2. tag prio 0 & 1 route to queue 0,
	 *        3. tag prio 2 & 3 route to queue 1,
	 *        4. tag prio 0 & 1 route to queue 2,
	 *        5. tag prio 2 & 3 route to queue 3.
	 */
	for (queue = 0; queue < rx_queues_count; queue++)
		dn200_mtl_dynamic_chan_set(priv, priv->hw, queue, dynamic);

	/* set last queue as dynamic map */
	if (!PRIV_IS_VF(priv))
		dn200_mtl_dynamic_chan_set(priv, priv->hw,
					   DN200_LAST_QUEUE(priv), dynamic);
}

static void dn200_mac_config_rss(struct dn200_priv *priv)
{
	if (!priv->dma_cap.rssen || !priv->plat->rss_en) {
		priv->rss.enable = false;
		return;
	}

	if (priv->dev->features & NETIF_F_RXHASH)
		priv->rss.enable = true;
	else
		priv->rss.enable = false;

	if (priv->plat->rx_queues_to_use <= 1)
		priv->rss.enable = false;
	dn200_rss_configure(priv, priv->hw, &priv->rss,
			    priv->plat->rx_queues_to_use);
}

/**
 *  dn200_mtl_configuration - Configure MTL
 *  @priv: driver private structure
 *  Description: It is used for configurring MTL
 */
static void dn200_mtl_configuration(struct dn200_priv *priv)
{
	u32 rx_queues_count = priv->plat->rx_queues_to_use;
	u32 tx_queues_count = priv->plat->tx_queues_to_use;

	if (tx_queues_count > 1)
		dn200_set_tx_queue_weight(priv);

	if (rx_queues_count > 1)
		dn200_set_rx_queue_weight(priv);

	/* Configure MTL RX algorithms */
	if (rx_queues_count > 1)
		dn200_prog_mtl_rx_algorithms(priv, priv->hw,
					     priv->plat->rx_sched_algorithm);

	/* Configure MTL TX algorithms */
	if (tx_queues_count > 1)
		dn200_prog_mtl_tx_algorithms(priv, priv->hw,
					     priv->plat->tx_sched_algorithm);

	/* Configure CBS in AVB TX queues */
	if (tx_queues_count > 1)
		dn200_configure_cbs(priv);

	/* Map RX MTL to DMA channels */
	dn200_rx_queue_dma_chan_map(priv);

	/* Enable MAC RX Queues */
	dn200_mac_enable_rx_queues(priv);

	/* Set RX priorities */
	if (rx_queues_count > 1)
		dn200_mac_config_rx_queues_prio(priv);

	/* Set TX priorities */
	if (tx_queues_count > 1)
		dn200_mac_config_tx_queues_prio(priv);

	/* Set RX routing */
	if (rx_queues_count > 1)
		dn200_mac_config_rx_queues_routing(priv);

	/* Set MTL queues to DMA channels mapping as dynamic
	 * rss, l3/l4 filter, vf routing, etc. features should base on it
	 */
	dn200_rx_queue_dma_chan_dynamic_map(priv);

	/* Receive Side Scaling */
	dn200_mac_config_rss(priv);
}

static void dn200_safety_feat_configuration(struct dn200_priv *priv)
{
	if (priv->dma_cap.asp) {
		dn200_safety_feat_config(priv, priv->ioaddr, priv->dma_cap.asp,
					 priv->plat->safety_feat_cfg, priv->hw);
	}
}

u32 dn200_riwt2usec(u32 riwt, struct dn200_priv *priv)
{
	unsigned long clk = clk_get_rate(priv->plat->dn200_clk);

	if (!clk) {
		clk = priv->plat->clk_ref_rate;
		if (!clk)
			return 0;
	}
	/* use 512 system clock cycles as rwt units */
	return (riwt * 512) / (clk / 1000000);
}

/**
 * dn200_hw_setup - setup mac in a usable state.
 *  @dev : pointer to the device structure.
 *  @ptp_register: register PTP if set
 *  Description:
 *  this is the main function to setup the HW in a usable state because the
 *  dma engine is reset, the core registers are configured (e.g. AXI,
 *  Checksum features, timers). The DMA is ready to start receiving and
 *  transmitting.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */

static int dn200_hw_setup(struct net_device *dev, bool ptp_register)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 queue;
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	bool sph_en;
	u32 chan;
	int ret;

	/* DMA initialization and SW reset */
	ret = dn200_init_dma_engine(priv);
	if (ret < 0) {
		netdev_err(priv->dev,
			   "%s: DMA engine common part initialization failed\n",
			   __func__);
		return ret;
	}

	/* PS and related bits will be programmed according to the speed */
	if (priv->hw->pcs) {
		int speed = priv->plat->mac_port_sel_speed;

		if (speed == SPEED_10 || speed == SPEED_100 ||
		    speed == SPEED_1000) {
			priv->hw->ps = speed;
		} else {
			dev_warn(priv->device, "invalid port speed\n");
			priv->hw->ps = 0;
		}
	}

	/* Initialize the MAC Core */
	dn200_core_init(priv, priv->hw, dev);

	/* Initialize MTL */
	dn200_mtl_configuration(priv);

	/* Initialize Safety Features */
	dn200_safety_feat_configuration(priv);

	ret = dn200_rx_ipc(priv, priv->hw);
	if (!ret) {
		netdev_warn(priv->dev, "RX IPC Checksum Offload disabled\n");
		priv->hw->rx_csum = 0;
	}

	/* Disable the MAC Rx/Tx , mac should enabled after phy link up */
	dn200_mac_set(priv, priv->ioaddr, false, priv->hw);

	/* Set the HW DMA mode and the COE */
	dn200_dma_operation_mode(priv);

	if (PRIV_IS_VF(priv)) {
		priv->dma_cap.rmon = 0;
		priv->dma_cap.asp = 0;
	} else {
		dn200_mmc_setup(priv);
	}

	if (ptp_register) {
		ret = clk_prepare_enable(priv->plat->clk_ptp_ref);
		if (ret < 0)
			netdev_warn(priv->dev,
				    "failed to enable PTP reference clock: %pe\n",
				    ERR_PTR(ret));
	}

	if (!PRIV_IS_VF(priv)) {
		ret = dn200_init_ptp(priv);
		if (ret == -EOPNOTSUPP)
			netdev_warn(priv->dev, "PTP not supported by HW\n");
		else if (ret)
			netdev_warn(priv->dev, "PTP init failed\n");
		else if (ptp_register)
			dn200_ptp_register(priv);
	}
	priv->eee_tw_timer = DN200_DEFAULT_TWT_LS;

	/* Convert the timer from msec to usec */
	if (!priv->tx_lpi_timer)
		priv->tx_lpi_timer = DN200_DEFAULT_LPI_TIMER * 1000;

	for (queue = 0; queue < rx_cnt; queue++) {
		if (priv->use_riwt) {
			if (!priv->rx_riwt[queue])
				priv->rx_riwt[queue] = DEF_DMA_RIWT;
			priv->rx_rius[queue] =
			    dn200_riwt2usec(priv->rx_riwt[queue], priv);
			/*Dynamic itr is supported by default */
			if (!priv->rx_intr[queue].itr_setting)
				priv->rx_intr[queue].itr_setting =
				    (DN200_ITR_DYNAMIC_ITR | 1);
			priv->rx_intr[queue].target_itr = priv->min_usecs;
			dn200_rx_watchdog(priv, priv->ioaddr,
					  priv->rx_riwt[queue], queue,
					  priv->hw);
		}
		if (!priv->tx_intr[queue].itr_setting)
			priv->tx_intr[queue].itr_setting =
			    (DN200_ITR_DYNAMIC_ITR | 1);
		priv->tx_intr[queue].target_itr = DN200_TX_FRAMES;
	}

	if (priv->hw->pcs)
		dn200_pcs_ctrl_ane(priv, priv->ioaddr, 1, priv->hw->ps, 0);

	/* set TX and RX rings length */
	dn200_set_rings_length(priv);

	/* Enable TSO */
	if (priv->tso) {
		for (chan = 0; chan < tx_cnt; chan++) {
			struct dn200_tx_queue *tx_q = &priv->tx_queue[chan];

			/* TSO and TBS cannot co-exist */
			if (tx_q->tbs & DN200_TBS_AVAIL)
				continue;

			dn200_enable_tso(priv, priv->ioaddr, 1, chan, priv->hw);
		}
	}

	/* Enable Split Header */
	sph_en = (priv->hw->rx_csum > 0) && priv->sph;
	for (chan = 0; chan < rx_cnt; chan++)
		dn200_enable_sph(priv, priv->ioaddr, sph_en, chan, priv->hw);

	/* VLAN Tag Insertion */
	if (priv->dma_cap.vlins)
		dn200_enable_vlan(priv, priv->hw,
				  (priv->dev->features & NETIF_F_HW_VLAN_CTAG_TX) ?
				  DN200_VLAN_INSERT : DN200_VLAN_NONE);

	/* TBS */
	for (chan = 0; chan < tx_cnt; chan++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[chan];
		int enable = tx_q->tbs & DN200_TBS_AVAIL;

		dn200_enable_tbs(priv, priv->ioaddr, enable, chan, priv->hw);
	}
	/* DCB */
	if (!PRIV_IS_VF(priv))
		dn200_dcbnl_init(priv, true);

	/* Start the ball rolling... */
	if (!PRIV_IS_VF(priv))
		dn200_start_all_dma(priv);

	return 0;
}

static void dn200_hw_teardown(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	clk_disable_unprepare(priv->plat->clk_ptp_ref);
}

static void dn200_free_irq(struct net_device *dev,
			   enum request_irq_err irq_err, int irq_idx)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct device *pdev = priv->device;
	int j;

	switch (irq_err) {
	case REQ_IRQ_ERR_ALL:
		irq_idx = priv->plat->tx_queues_to_use;
		fallthrough;
	case REQ_IRQ_ERR_RXTX:
		if (priv->txrx_itr_combined) {
			for (j = irq_idx - 1; j >= 0; j--) {
				if (priv->tx_irq[j] > 0) {
					irq_set_affinity_hint(priv->tx_irq[j],
							      NULL);
					synchronize_irq(priv->tx_irq[j]);
					devm_free_irq(pdev, priv->tx_irq[j],
						 &priv->channel[j]);
				}
				if (priv->rx_irq[j] > 0) {
					irq_set_affinity_hint(priv->rx_irq[j],
							      NULL);
					synchronize_irq(priv->rx_irq[j]);
					devm_free_irq(pdev, priv->rx_irq[j],
						 &priv->channel[j]);
				}
			}
		}
		fallthrough;
	case REQ_IRQ_ERR_TX:
		if (!priv->txrx_itr_combined) {
			for (j = irq_idx - 1; j >= 0; j--) {
				netdev_dbg(dev, "j %d tx irq %d rx irq %d\n", j,
					   priv->tx_irq[j], priv->rx_irq[j]);
				if (priv->tx_irq[j] > 0) {
					irq_set_affinity_hint(priv->tx_irq[j],
							      NULL);
					synchronize_irq(priv->tx_irq[j]);
					devm_free_irq(pdev, priv->tx_irq[j],
						 &priv->tx_queue[j]);
				}
			}
			irq_idx = priv->plat->rx_queues_to_use;
		}
		fallthrough;
	case REQ_IRQ_ERR_RX:
		if (!priv->txrx_itr_combined) {
			for (j = irq_idx - 1; j >= 0; j--) {
				if (priv->rx_irq[j] > 0) {
					irq_set_affinity_hint(priv->rx_irq[j],
							      NULL);
					synchronize_irq(priv->rx_irq[j]);
					devm_free_irq(pdev, priv->rx_irq[j],
						 &priv->rx_queue[j]);
				}
			}
		}
		fallthrough;
	case REQ_IRQ_ERR_SFTY_UE:
		if (priv->sfty_ue_irq > 0 && priv->sfty_ue_irq != dev->irq) {
			synchronize_irq(priv->sfty_ue_irq);
			devm_free_irq(pdev, priv->sfty_ue_irq, dev);
		}
		fallthrough;
	case REQ_IRQ_ERR_SFTY_CE:
		if (priv->sfty_ce_irq > 0 && priv->sfty_ce_irq != dev->irq) {
			synchronize_irq(priv->sfty_ce_irq);
			devm_free_irq(pdev, priv->sfty_ce_irq, dev);
		}
		fallthrough;
	case REQ_IRQ_ERR_LPI:
		if (priv->lpi_irq > 0 && priv->lpi_irq != dev->irq) {
			synchronize_irq(priv->lpi_irq);
			devm_free_irq(pdev, priv->lpi_irq, dev);
		}
		fallthrough;
	case REQ_IRQ_ERR_MAC:
		if (dev->irq > 0) {
			synchronize_irq(dev->irq);
			devm_free_irq(pdev, dev->irq, dev);
		}
		fallthrough;
	case REQ_IRQ_ERR_NO:
		/* If MAC IRQ request error, no more IRQ to free */
		break;
	}

	kfree(priv->cpu_mask);
}

static void dn200_set_affinity_hint(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 max_q = 0, cpu_offset = 0;
	u16 pf_id = priv->plat_ex->pf_id;
	int i = 0;

	max_q = max_t(u32, priv->plat->rx_queues_to_use,
				priv->plat->tx_queues_to_use);

	if (max_q > 2) {
		cpu_offset = (priv->plat_ex->pf_id ? 8 : 0);
		for (i = 0; i < priv->plat->rx_queues_to_use; i++) {
			if (priv->rx_irq[i] == 0)
				continue;
			cpumask_clear(priv->cpu_mask);
			cpumask_set_cpu(cpumask_local_spread
					(i + cpu_offset, priv->numa_node), priv->cpu_mask);
			irq_set_affinity_hint(priv->rx_irq[i], priv->cpu_mask);
		}
		for (i = 0; i < priv->plat->tx_queues_to_use; i++) {
			if (priv->tx_irq[i] == 0)
				continue;
			cpumask_clear(priv->cpu_mask);
			cpumask_set_cpu(cpumask_local_spread
					(i + cpu_offset, priv->numa_node), priv->cpu_mask);
			irq_set_affinity_hint(priv->tx_irq[i], priv->cpu_mask);
		}
	} else {
		if (pf_id < 2) {
			cpu_offset = (priv->plat_ex->pf_id ? 8 : 0);
		} else {
			max_q = max_t(u32, priv->plat->rx_queues_to_use,
				priv->plat->tx_queues_to_use);
			cpu_offset = 16 + max_q * (pf_id - 2) * 2;
		}
		for (i = 0; i < priv->plat->rx_queues_to_use; i++) {
			if (priv->rx_irq[i] == 0)
				continue;
			cpumask_clear(priv->cpu_mask);
			cpumask_set_cpu(cpumask_local_spread
					(i + cpu_offset, priv->numa_node), priv->cpu_mask);
			irq_set_affinity_hint(priv->rx_irq[i], priv->cpu_mask);
		}
		for (i = 0; i < priv->plat->tx_queues_to_use; i++) {
			if (priv->tx_irq[i] == 0)
				continue;
			cpumask_clear(priv->cpu_mask);
			cpumask_set_cpu(cpumask_local_spread
					(i + cpu_offset + priv->plat->rx_queues_to_use, priv->numa_node), priv->cpu_mask);
			irq_set_affinity_hint(priv->tx_irq[i], priv->cpu_mask);
		}
	}
}

static int dn200_request_irq_multi_msi(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct device *pdev = priv->device;
	enum request_irq_err irq_err;
	u32 cpuid = 0;
	int irq_idx = 0;
	char *int_name;
	int ret;
	int i;

	priv->cpu_mask = kmalloc(sizeof(cpumask_t), GFP_KERNEL);
	if (!priv->cpu_mask)
		return -ENOMEM;

	/* For pmt interrupt */
	if (dev->irq > 0) {
		int_name = priv->int_name_mac;
		sprintf(int_name, "%s:%s", dev->name, "mac");
		ret = devm_request_irq(pdev, dev->irq, dn200_mac_interrupt,
				  0, int_name, dev);
		if (unlikely(ret < 0)) {
			netdev_err(priv->dev,
				   "%s: alloc mac MSI %d (error: %d)\n",
				   __func__, dev->irq, ret);
			irq_err = REQ_IRQ_ERR_NO;
			goto irq_error;
		}
	}

	/* Request the LPI IRQ in case of another line
	 * is used for LPI
	 */
	if (priv->lpi_irq > 0 && priv->lpi_irq != dev->irq) {
		int_name = priv->int_name_lpi;
		sprintf(int_name, "%s:%s", dev->name, "lpi");
		ret = devm_request_irq(pdev, priv->lpi_irq,
				  dn200_mac_interrupt, 0, int_name, dev);
		if (unlikely(ret < 0)) {
			netdev_err(priv->dev,
				   "%s: alloc lpi MSI %d (error: %d)\n",
				   __func__, priv->lpi_irq, ret);
			irq_err = REQ_IRQ_ERR_MAC;
			goto irq_error;
		}
	}

	/* Request the Safety Feature Correctible Error line in
	 * case of another line is used
	 */
	if (priv->sfty_ce_irq > 0 && priv->sfty_ce_irq != dev->irq) {
		int_name = priv->int_name_sfty_ce;
		sprintf(int_name, "%s:%s", dev->name, "safety-ce");
		ret = devm_request_irq(pdev, priv->sfty_ce_irq,
				  dn200_safety_interrupt, 0, int_name, dev);
		if (unlikely(ret < 0)) {
			netdev_err(priv->dev,
				   "%s: alloc sfty ce MSI %d (error: %d)\n",
				   __func__, priv->sfty_ce_irq, ret);
			irq_err = REQ_IRQ_ERR_LPI;
			goto irq_error;
		}
	}

	/* Request the Safety Feature Uncorrectible Error line in
	 * case of another line is used
	 */
	if (priv->sfty_ue_irq > 0 && priv->sfty_ue_irq != dev->irq) {
		int_name = priv->int_name_sfty_ue;
		sprintf(int_name, "%s:%s", dev->name, "safety-ue");
		ret = devm_request_irq(pdev, priv->sfty_ue_irq,
				  dn200_safety_interrupt, 0, int_name, dev);
		if (unlikely(ret < 0)) {
			netdev_err(priv->dev,
				   "%s: alloc sfty ue MSI %d (error: %d)\n",
				   __func__, priv->sfty_ue_irq, ret);
			irq_err = REQ_IRQ_ERR_SFTY_CE;
			goto irq_error;
		}
	}
	if (priv->txrx_itr_combined) {
		/* Request Rx MSI irq */
		for (i = 0; i < priv->plat->rx_queues_to_use &&
		     i < priv->plat->tx_queues_to_use; i++) {
			cpumask_clear(priv->cpu_mask);
			cpuid =
			    cpumask_local_spread(i + priv->plat_ex->pf_id * 8,
						 priv->numa_node);
			cpumask_set_cpu(cpuid, priv->cpu_mask);
			if (priv->rx_irq[i]) {
				int_name = priv->int_name_rx_irq[i];
				sprintf(int_name, "%s:%s-%d", dev->name, "rx",
					i);
				ret =
				    devm_request_irq(pdev, priv->rx_irq[i],
						dn200_msi_intr_rxtx, 0,
						int_name, &priv->channel[i]);
				if (unlikely(ret < 0)) {
					netdev_err(priv->dev,
						   "%s: alloc rx-%d  MSI %d (error: %d)\n",
						   __func__, i, priv->rx_irq[i],
						   ret);
					irq_err = REQ_IRQ_ERR_RXTX;
					irq_idx = i;
					goto irq_error;
				}
				irq_set_affinity_hint(priv->rx_irq[i],
						      priv->cpu_mask);
			}
			if (priv->tx_irq[i]) {
				int_name = priv->int_name_tx_irq[i];
				sprintf(int_name, "%s:%s-%d", dev->name, "tx",
					i);
				ret =
				    devm_request_irq(pdev, priv->tx_irq[i],
						dn200_msi_intr_rxtx, 0,
						int_name, &priv->channel[i]);
				if (unlikely(ret < 0)) {
					netdev_err(priv->dev,
						   "%s: alloc tx-%d  MSI %d (error: %d)\n",
						   __func__, i, priv->tx_irq[i],
						   ret);
					devm_free_irq(pdev, priv->rx_irq[i],
						 &priv->rx_queue[i]);
					irq_err = REQ_IRQ_ERR_RXTX;
					irq_idx = i;
					goto irq_error;
				}
				irq_set_affinity_hint(priv->tx_irq[i],
						      priv->cpu_mask);
			}
		}
	} else {
		/* Request Rx MSI irq */
		for (i = 0; i < priv->plat->rx_queues_to_use; i++) {
			if (priv->rx_irq[i] == 0)
				continue;
			int_name = priv->int_name_rx_irq[i];
			sprintf(int_name, "%s:%s-%d", dev->name, "rx", i);
			ret = devm_request_irq(pdev, priv->rx_irq[i],
					  dn200_msi_intr_rx,
					  0, int_name, &priv->rx_queue[i]);
			if (unlikely(ret < 0)) {
				netdev_err(priv->dev,
					   "%s: alloc rx-%d  MSI %d (error: %d)\n",
					   __func__, i, priv->rx_irq[i], ret);
				irq_err = REQ_IRQ_ERR_RX;
				irq_idx = i;
				goto irq_error;
			}
		}

		/* Request Tx MSI irq */
		for (i = 0; i < priv->plat->tx_queues_to_use; i++) {
			if (priv->tx_irq[i] == 0)
				continue;
			int_name = priv->int_name_tx_irq[i];
			sprintf(int_name, "%s:%s-%d", dev->name, "tx", i);
			ret = devm_request_irq(pdev, priv->tx_irq[i],
					  dn200_msi_intr_tx,
					  0, int_name, &priv->tx_queue[i]);
			if (unlikely(ret < 0)) {
				netdev_err(priv->dev,
					   "%s: alloc tx-%d  MSI %d (error: %d)\n",
					   __func__, i, priv->tx_irq[i], ret);
				irq_err = REQ_IRQ_ERR_TX;
				irq_idx = i;
				goto irq_error;
			}
		}
		dn200_set_affinity_hint(dev);
	}

	ret = 0;
	return ret;
irq_error:
	dn200_free_irq(dev, irq_err, irq_idx);
	return ret;
}

static int dn200_request_irq_single(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct device *pdev = priv->device;
	enum request_irq_err irq_err;
	int ret;

	ret = devm_request_irq(pdev, dev->irq, dn200_interrupt,
			  IRQF_SHARED, dev->name, dev);
	if (unlikely(ret < 0)) {
		netdev_err(priv->dev,
			   "%s: ERROR: allocating the IRQ %d (error: %d)\n",
			   __func__, dev->irq, ret);
		irq_err = REQ_IRQ_ERR_MAC;
		goto irq_error;
	}

	/* Request the IRQ lines */
	if (priv->lpi_irq > 0 && priv->lpi_irq != dev->irq) {
		ret = devm_request_irq(pdev, priv->lpi_irq, dn200_interrupt,
				  IRQF_SHARED, dev->name, dev);
		if (unlikely(ret < 0)) {
			netdev_err(priv->dev,
				   "%s: ERROR: allocating the LPI IRQ %d (%d)\n",
				   __func__, priv->lpi_irq, ret);
			irq_err = REQ_IRQ_ERR_LPI;
			goto irq_error;
		}
	}

	return 0;

irq_error:
	dn200_free_irq(dev, irq_err, 0);
	return ret;
}

static int dn200_request_irq(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret = 0;

	/* Request the IRQ lines */
	if (priv->plat->multi_msi_en)
		ret = dn200_request_irq_multi_msi(dev);
	else
		ret = dn200_request_irq_single(dev);

	return ret;
}

/*Check dma TX/RX status, if not idle/stop status, we should reset hw.*/
static int dn200_check_dma_status(struct dn200_priv *priv)
{
	u16 chan = 0;
	int ret = 0;

	for (chan = 0; chan < priv->plat->tx_queues_to_use; chan++) {
		ret |=
		    dn200_check_chan_status(priv, priv->ioaddr, chan, priv->hw,
					    true);
	}
	for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
		ret |=
		    dn200_check_chan_status(priv, priv->ioaddr, chan, priv->hw,
					    false);
	}
	if (ret) {
		netdev_warn(priv->dev,
			    "dma status is in error status before open\n");
		return -1;
	}
	return 0;
}

static void dn200_set_vf_heart_vf_state(struct dn200_priv *priv, u8 val,
					bool enable)
{
	u8 registered_vf_state = 0;

	DN200_HEARTBEAT_GET(priv->hw, registered_vf_state,
			    priv->plat_ex->vf_offset, &registered_vf_state);
	if (enable)
		registered_vf_state |= val;
	else
		registered_vf_state &= ~val;
	DN200_HEARTBEAT_SET(priv->hw, registered_vf_state,
			    priv->plat_ex->vf_offset, registered_vf_state);
}

static void dn200_vf_init_heartbeat(struct dn200_priv *priv)
{
	u8 last_beat, beat;

	if (!PRIV_IS_VF(priv))
		return;
	DN200_HEARTBEAT_SET(priv->hw, registered_vf_state,
			    priv->plat_ex->vf_offset,
			    DN200_VF_REG_STATE_OPENED);
	DN200_HEARTBEAT_GET(priv->hw, last_heartbeat, priv->plat_ex->vf_offset,
			    &last_beat);
	beat = !last_beat;
	DN200_HEARTBEAT_SET(priv->hw, heartbeat, priv->plat_ex->vf_offset,
			    beat);
}

static void dn200_set_am_and_pm(struct dn200_priv *priv)
{
	u32 value = 0;

	value &= ~(XGMAC_FILTER_PR | XGMAC_FILTER_HMC | XGMAC_FILTER_PM);
	value |= XGMAC_FILTER_PR;
	writel(value, priv->ioaddr + XGMAC_PACKET_FILTER);
}

static int _dn200_set_umac_addr(struct dn200_priv *priv, unsigned char *addr, u8 reg_n)
{
	int ret = 0;
	u8 wakeup_wq = false;

	ret = dn200_set_umac_addr(priv, priv->hw, addr, reg_n, &wakeup_wq);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: set umac fail.\n", __func__);
		return ret;
	}
	priv->pf_rxp_set |= RXP_SET_UMAC;
	if (wakeup_wq && PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
		queue_work(priv->wq, &priv->rxp_task);
	else if (PRIV_SRIOV_SUPPORT(priv))
		set_bit(DN200_RXP_NEED_CHECK, &priv->state);

	if (PRIV_IS_VF(priv)) {
		netdev_dbg(priv->dev, "%s: notify pf set umac.\n", __func__);
		DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, RXP_TASK, 1);
		irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
	}
	return ret;
}

static void _dn200_config_vlan_rx_fltr(struct dn200_priv *priv, struct mac_device_info *hw, bool enable)
{
	if (PRIV_IS_PUREPF(priv)) {
		dn200_config_vlan_rx_fltr(priv, priv->hw, enable);
	} else if (PRIV_SRIOV_SUPPORT(priv)) {
		priv->pf_rxp_set |= RXP_SET_VLAN_FIL;
		priv->vlan_fil_enable = enable;
		if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
			queue_work(priv->wq, &priv->rxp_task);
		else if (PRIV_SRIOV_SUPPORT(priv))
			set_bit(DN200_RXP_NEED_CHECK, &priv->state);
	}
}

static int dn200_set_features(struct net_device *netdev,
			      netdev_features_t features);
static void dn200_set_rx_mode(struct net_device *dev);
static void dn200_vlan_reconfig(struct dn200_priv *priv);
static void dn200_fdirs_reconfig(struct dn200_priv *priv, bool enable);

static void dn200_eth_reconfig(struct dn200_priv *priv)
{
	struct net_device *dev = priv->dev;
	int ret = 0;
	/*if sriov support && sriov enable */
	if (PRIV_SRIOV_SUPPORT(priv)) {
		/*enable DDS and mcbc duplicate */
		dn200_set_am_and_pm(priv);
		dn200_rx_dds_config(priv, priv->hw, true);
	}

	ret = _dn200_set_umac_addr(priv, (unsigned char *)priv->dev->dev_addr, 0);
	if (ret < 0)
		netdev_err(priv->dev, "%s: set umac fail.\n", __func__);
	/*init vlan filter */
	if (!HW_IS_VF(priv->hw))
		dn200_init_hw_vlan_rx_fltr(priv, priv->hw);
	dn200_set_features(dev, dev->features);
	dn200_set_rx_mode(dev);
	dn200_set_mac_loopback(priv, priv->ioaddr,
			       !!(dev->features & NETIF_F_LOOPBACK));
	dn200_config_hw_tstamping(priv, priv->ptpaddr, priv->systime_flags);
	if (!HW_IS_VF(priv->hw)) {
		dn200_rxp_config(priv, priv->hw, priv->tc_entries,
				 priv->tc_entries_max);
	}
	if (!HW_IS_VF(priv->hw))
		dn200_vlan_reconfig(priv);
	if (!HW_IS_VF(priv->hw)) {
		dn200_fdirs_reconfig(priv, true);
		dn200_rss_configure(priv, priv->hw, &priv->rss,
				    priv->plat->rx_queues_to_use);
	}
	if (priv->vxlan_port)
		dn200_vxlan_set(dev);

	if (!(PRIV_IS_PUREPF(priv) || PRIV_IS_VF(priv))) {
		_dn200_config_vlan_rx_fltr(priv, priv->hw,
					  !(priv->dev->flags & IFF_PROMISC));
	}
}

static inline void dn200_rx_itr_usec_update(struct dn200_priv *priv)
{
	/* for 10Gbit/s bandwidth(or 1250Byte/us),
	 * packet total size = (mtu * dma_rx_size) can hold
	 * rx_usec = ((mtu * priv->dma_rx_size)/1250),
	 * so rx watchdog should take effect within 1/2 * rx_usec
	 */
	const int mtu = priv->dev->mtu;

	priv->rx_itr_usec = (((mtu * priv->dma_rx_size) / 125) * 1000 / priv->speed) >> 1;
	if (priv->rx_itr_usec > priv->max_usecs)
		priv->rx_itr_usec = priv->max_usecs;
	priv->rx_itr_usec_min = priv->min_usecs;
	dev_dbg(priv->device, "%s, %d, priv->rx_itr_usec:%u, priv->rx_itr_usec_min:%u\n",
			__func__, __LINE__, priv->rx_itr_usec, priv->rx_itr_usec_min);
}

static int dn200_reset(struct dn200_priv *priv)
{
	int ret = 0;

	if (!priv->mii) {
		/*add phy clock's stable judge*/
		ret = dn200_phy_clock_stable_judge(PRIV_PHY_INFO(priv));
		if (ret) {
			netdev_warn(priv->dev,
				 "%s %d phy clock maybe not stable,give more time\n",
					 __func__, __LINE__);
			usleep_range(1000000, 2000000);
		}
	}

	ret = dn200_dma_reset(priv, priv->ioaddr, priv->hw);
	if (ret) {
		dev_err(priv->device, "dma reset err\n");
		return ret;
	}

	ret = dn200_rxf_and_acl_mem_reset(priv, priv->hw);
	if (ret) {
		dev_err(priv->device, "rxf reset err\n");
		return ret;
	}

	return 0;
}

static int dn200_open_continue(struct dn200_priv *priv)
{
	u8 vf_link_notify = 0;
	int ret = 0;

	if (PRIV_SRIOV_SUPPORT(priv)) {
		set_bit(DN200_DEV_INIT, &priv->state);

		/* reset rxp */
		ret = dn200_reset_rxp(priv, priv->hw);

		/* init broadcast rxp */
		if (ret == 0)
			ret = dn200_rxp_broadcast(priv, priv->hw);
	}

	/* if rxp init failed, clear down & opening state, but keep dev init state */
	if (ret) {
		netdev_info(priv->dev, "%s, %d, rxp init failure.\n", __func__, __LINE__);
		goto err_rxp;
	}

	/* can't configure rxp mac, vlan, filter before dev init complete */
	clear_bit(DN200_DEV_INIT, &priv->state);

	if (PRIV_IS_VF(priv))
		clear_bit(DN200_VF_IN_STOP, &priv->state);
	/* ethernet reconfig for two conditions:
	 * 1. dev down and open, to restore user configurations
	 * 2. protocol stack configurations set in the process of dev open
	 */
	dn200_eth_reconfig(priv);

	netif_tx_start_all_queues(priv->dev);
	/* if occur err, don't start phy and keep carrier off */
	if (PRIV_PHY_OPS_CHECK(priv) && PRIV_PHY_OPS(priv)->start)
		PRIV_PHY_OPS(priv)->start(PRIV_PHY_INFO(priv));

	if (PRIV_IS_VF(priv))
		dn200_vf_init_heartbeat(priv);
	timer_setup(&priv->keepalive_timer, dn200_heartbeat, 0);
	mod_timer(&priv->keepalive_timer, msecs_to_jiffies(1000));

	if (PRIV_IS_VF(priv)) {
		DN200_VF_LINK_GET(priv, priv->plat_ex->vf_offset,
				  &vf_link_notify);
		if (!vf_link_notify)
			DN200_VF_LINK_SET(priv, priv->plat_ex->vf_offset, 1);
	} else {
		DN200_SET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, 1);
	}
	/* keep following code at the end !!! */
err_rxp:
	clear_bit(DN200_DCB_DOWN, &priv->state);
	clear_bit(DN200_DOWN, &priv->state);

	/* must notify vf to reset after reconfig, otherwise vf can't work */
	if (ret == 0 && priv->plat_ex->sriov_cfg)
		_dn200_vf_flow_open(priv);

	return ret;
}

/**
 *  dn200_open - open entry point of the driver
 *  @dev : pointer to the device structure.
 *  Description:
 *  This function is the open entry point of the driver.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */

static int dn200_open(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int bfsize = 0;
	u32 chan;
	u8 states = 0;
	int ret = 0, dma_chan_err = 0;

	if (test_bit(DN200_SYS_SUSPENDED, &priv->state))
		return 0;

	if (test_and_set_bit(DN200_UP, &priv->state))
		return 0;
	/* forbid to set rxp in dev init state;
	 * clear dev init state when rxp complete to reset & init
	 */
	set_bit(DN200_DEV_INIT, &priv->state);
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		clear_bit(DN200_UP, &priv->state);
		return -EBUSY;
	}

	if (test_bit(DN200_DEV_ERR_CLOSE, &priv->state)) {
		netdev_err(dev, "%s: %s\n", __func__, DN200_FW_ERR_MSG);
		clear_bit(DN200_UP, &priv->state);
		return -EBUSY;
	}
	if (!PRIV_IS_VF(priv)) {
		unsigned long time_start = jiffies;

		while (!test_bit(DN200_PROBE_FINISHED, &priv->state)) {
			usleep_range(1000, 2000);
			if (time_after(jiffies, time_start + msecs_to_jiffies(4000))) {
				netdev_err(dev, "%s: probing took 4s, but not finish, may have errs\n", __func__);
				clear_bit(DN200_UP, &priv->state);
				return -EBUSY;
			}
		}
	}
	if (PRIV_IS_VF(priv)) {
		DN200_GET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, &states);
		if (!states) {
			netdev_err(dev,
				   "Unable to start - perhaps the PF Driver isn't up yet.\n");
			clear_bit(DN200_UP, &priv->state);
			return -EAGAIN;
		}
	}
	netif_carrier_off(dev);
	if (PRIV_IS_VF(priv))
		dn200_stop_all_dma(priv);
	if (dn200_iatu_init(priv) < 0)
		goto iatu_init_error;

	/* limit the rx buffer size to 1536 or 3K */
	bfsize = dn200_get_bfsize();
	priv->dma_buf_sz = bfsize;

	if (!priv->dma_tx_size)
		priv->dma_tx_size = DMA_DEFAULT_TX_SIZE;
	if (!priv->dma_rx_size) {
		if (PRIV_IS_VF(priv))
			priv->dma_rx_size = DMA_DEFAULT_VF_RX_SIZE;
		else
			priv->dma_rx_size = DMA_DEFAULT_RX_SIZE;
	}

	/* Earlier check for TBS */
	for (chan = 0; chan < priv->plat->tx_queues_to_use; chan++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[chan];
		int tbs_en = priv->plat->tx_queues_cfg[chan].tbs_en;

		/* Setup per-TXQ tbs flag before TX descriptor alloc */
		tx_q->tbs |= tbs_en ? DN200_TBS_AVAIL : 0;
	}
	ret = alloc_dma_desc_resources(priv);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: DMA descriptors allocation failed\n",
			   __func__);
		goto dma_desc_error;
	}
	ret = init_dma_desc_rings(dev, GFP_KERNEL);
	if (ret < 0) {
		netdev_err(priv->dev,
			   "%s: DMA descriptors initialization failed\n",
			   __func__);
		goto init_error;
	}
	dn200_add_rx_iatu2tx(priv);
	dn200_mac_set(priv, priv->ioaddr, false, priv->hw);
	if (PRIV_IS_VF(priv)) {
		/* check all dma channel status, if any channel existed error,
		 * will call global error reset in the end
		 */
		dma_chan_err = dn200_check_dma_status(priv);
	} else {
		if (priv->mii) {
			/* set rgmii rx clock from soc */
			dn200_xgmac_rx_ext_clk_set(priv, false);
			/* workaround: set phy loopback for reg timeout */
			mdiobus_write(priv->mii, priv->plat->phy_addr, 0, 0x4140);
		}
		/* hw reset when pure pf or sriov pf up */
		ret = dn200_reset(priv);
		if (ret) {
			dev_err(priv->device, "Failed to reset the dma\n");
			goto init_error;
		}
		if (PRIV_SRIOV_SUPPORT(priv) && !PRIV_IS_VF(priv))
			dn200_sriov_mail_init(priv);
	}
	if (priv->hw->pcs != DN200_PCS_TBI && priv->hw->pcs != DN200_PCS_RTBI) {
		ret = dn200_init_phy(dev);
		if (ret) {
			netdev_err(priv->dev,
				   "%s: Cannot attach to PHY (error: %d)\n",
				   __func__, ret);
			goto init_error;
		}
	}
	ret = dn200_hw_setup(dev, true);
	if (ret < 0) {
		netdev_err(priv->dev, "%s: Hw setup failed\n", __func__);
		goto init_error;
	}
	/* Configure real RX and TX queues */
	netif_set_real_num_rx_queues(dev, priv->plat->rx_queues_to_use);
	netif_set_real_num_tx_queues(dev, priv->plat->tx_queues_to_use);

	dn200_init_coalesce(priv);
	ret = dn200_request_irq(dev);
	if (ret)
		goto irq_error;

	netdev_update_features(dev);
	if (PRIV_IS_PUREPF(priv))
		dn200_napi_add(priv->dev);
	dn200_enable_all_queues(priv);
	dn200_enable_all_dma_irq(priv);
	/*modify vf tx timeout to 15s to avoid vm pending*/

	if (dma_chan_err) {
		dn200_global_err(priv, DN200_DMA_CHAN_ERR);
	} else {
		if (priv->mii) {
			/* cancel phy loopback */
			mdiobus_write(priv->mii, priv->plat->phy_addr, 0, 0x9140);
		}
	}

	/* base on new desc ring size to update rx interrupt usec */
	dn200_rx_itr_usec_update(priv);

	/* reconfig pf & vf settings */
	if (!dma_chan_err)
		dn200_open_continue(priv);

	if (PRIV_IS_VF(priv))
		dn200_start_all_dma(priv);
	return 0;

irq_error:
	for (chan = 0; chan < priv->plat->tx_queues_to_use; chan++) {
		if (priv->tx_queue[chan].tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].tx_task);
		if (priv->tx_queue[chan].txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].txtimer);
		memset(&priv->tx_queue[chan].txtimer, 0,
		       sizeof(struct hrtimer));
		if (priv->tx_queue[chan].poll_tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].poll_tx_task);
		if (priv->tx_queue[chan].poll_txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].poll_txtimer);
		memset(&priv->tx_queue[chan].poll_txtimer, 0,
		       sizeof(struct hrtimer));
	}
	for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
		if (priv->rx_queue[chan].poll_rx_task.func)
			cancel_work_sync(&priv->rx_queue[chan].poll_rx_task);
		if (priv->rx_queue[chan].poll_rxtimer.function)
			hrtimer_cancel(&priv->rx_queue[chan].poll_rxtimer);
		memset(&priv->rx_queue[chan].poll_rxtimer, 0,
		       sizeof(struct hrtimer));
	}
	dn200_hw_teardown(dev);
init_error:
	free_dma_desc_resources(priv);

dma_desc_error:
iatu_init_error:
	clear_bit(DN200_UP, &priv->state);
	return ret;
}

static void dn200_vf_clear_heartbeat(struct dn200_priv *priv)
{
	if (!PRIV_IS_VF(priv))
		return;
	dn200_set_vf_heart_vf_state(priv, DN200_VF_REG_STATE_OPENED, false);
	DN200_HEARTBEAT_SET(priv->hw, last_heartbeat, priv->plat_ex->vf_offset,
				0);
	DN200_HEARTBEAT_SET(priv->hw, heartbeat, priv->plat_ex->vf_offset, 0);
}

static void dn200_release_remain(struct dn200_priv *priv, bool pcie_ava)
{
	u8 vf_link_notify = 0;
	/* cancel the reconfig task used by hw locked failure when open */
	if (priv->reconfig_task.func)
		cancel_work_sync(&priv->reconfig_task);
	del_timer_sync(&priv->keepalive_timer);
	if (priv->eee_enabled) {
		priv->tx_path_in_lpi_mode = false;
		del_timer_sync(&priv->eee_ctrl_timer);
	}
	/* DCB */
	if (!PRIV_IS_VF(priv) && pcie_ava)
		dn200_dcbnl_init(priv, false);
	dn200_release_ptp(priv);

	if (PRIV_PHY_OPS_CHECK(priv) && PRIV_PHY_OPS(priv)->stop)
		PRIV_PHY_OPS(priv)->stop(PRIV_PHY_INFO(priv));
	set_bit(DN200_DCB_DOWN, &priv->state);
	if (PRIV_IS_VF(priv) && pcie_ava) {
		DN200_VF_LINK_GET(priv, priv->plat_ex->vf_offset,
				&vf_link_notify);
		if (vf_link_notify)
			DN200_VF_LINK_SET(priv, priv->plat_ex->vf_offset, 0);
	}
	memset(&priv->hw->set_state, 0, sizeof(struct dn200_set_state));
}

/**
 *  dn200_release - close entry point of the driver
 *  @dev : device pointer.
 *  Description:
 *  This is the stop entry point of the driver.
 */
static int dn200_release(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 chan = 0;
	u8 vf_link_notify = 0;
	int rx_state = 0;
	bool pcie_ava = false;

	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state)) {
		pcie_ava = dn200_hwif_id_check(priv->ioaddr);
		if (!pcie_ava)
			set_bit(DN200_PCIE_UNAVAILD, &priv->state);
	}

	if (test_bit(DN200_DOWN, &priv->state) && priv->vf_sw_close_flag && PRIV_IS_VF(priv)) {
		dn200_release_remain(priv, pcie_ava);
		priv->vf_sw_close_flag = false;
		return 0;
	}
	if (test_and_set_bit(DN200_DOWN, &priv->state))
		return 0;
	if (pcie_ava) {
		if (!PRIV_IS_VF(priv))
			dn200_flow_ctrl(priv, priv->hw, false, FLOW_OFF,
			 0, priv->plat->tx_queues_to_use);
		if (PRIV_PHY_OPS_CHECK(priv) && PRIV_PHY_OPS(priv)->phy_timer_del)
			PRIV_PHY_OPS(priv)->phy_timer_del(PRIV_PHY_INFO(priv));
		/* stop all vfs flow when pf down or release */
		rx_state = dn200_mac_rx_get(priv, priv->ioaddr);
		if (rx_state)
			dn200_mac_rx_set(priv, priv->ioaddr, false);
		if (!test_bit(DN200_DEV_ERR_CLOSE, &priv->state))
			dn200_vf_flow_close(priv);
	}
	/* cancel the reconfig task used by hw locked failure when open */
	if (priv->reconfig_task.func)
		cancel_work_sync(&priv->reconfig_task);
	if (pcie_ava) {
		if (!PRIV_IS_VF(priv))
			DN200_SET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, 0);
	}
	del_timer_sync(&priv->keepalive_timer);
	if (pcie_ava) {
		/* disable mac rx engine before clean tx queues and del rxp */
		if (PRIV_SRIOV_SUPPORT(priv))
			dn200_clean_all_tx_queues(priv, priv->plat_ex->tx_queues_total);
		else
			dn200_clean_all_tx_queues(priv, priv->plat->tx_queues_to_use);
		/* release rxp resource firstly to prevents
		 * broadcast packets from being sent to VF
		 */
		if (PRIV_IS_VF(priv)) {
			dn200_vf_clear_heartbeat(priv);
			dn200_vf_del_rxp(priv, priv->hw);
			if (PRIV_IS_VF(priv)) {
				netdev_dbg(priv->dev, "%s: notify pf set umac.\n", __func__);
				DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, RXP_TASK, 1);
				irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
			}
		}
	}

	dn200_disable_all_queues(priv);
	for (chan = 0; chan < priv->plat->tx_queues_to_use; chan++) {
		if (priv->tx_queue[chan].tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].tx_task);
		if (priv->tx_queue[chan].txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].txtimer);
		memset(&priv->tx_queue[chan].txtimer, 0,
		       sizeof(struct hrtimer));
		if (priv->tx_queue[chan].poll_tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].poll_tx_task);
		if (priv->tx_queue[chan].poll_txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].poll_txtimer);
		memset(&priv->tx_queue[chan].poll_txtimer, 0,
		       sizeof(struct hrtimer));
	}
	for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
		if (priv->rx_queue[chan].poll_rx_task.func)
			cancel_work_sync(&priv->rx_queue[chan].poll_rx_task);
		if (priv->rx_queue[chan].poll_rxtimer.function)
			hrtimer_cancel(&priv->rx_queue[chan].poll_rxtimer);
		memset(&priv->rx_queue[chan].poll_rxtimer, 0,
		       sizeof(struct hrtimer));
	}
	if (!PRIV_IS_VF(priv)) {
		if (!PRIV_IS_PUREPF(priv))
			udelay(100);
		else
			usleep_range(1000, 2000);
	}
	/* Free the IRQ lines */
	dn200_free_irq(dev, REQ_IRQ_ERR_ALL, 0);
	if (PRIV_IS_PUREPF(priv))
		dn200_napi_del(priv->dev);
	if (priv->eee_enabled) {
		priv->tx_path_in_lpi_mode = false;
		del_timer_sync(&priv->eee_ctrl_timer);
	}

	if (pcie_ava) {
		/* Stop TX/RX DMA and clear the descriptors */
		dn200_stop_all_dma(priv);

		/* enable mac rx engine state to deal with vf down but pf up */
		if (PRIV_IS_VF(priv) && rx_state)
			dn200_mac_rx_set(priv, priv->ioaddr, true);
	}
	/* DCB */
	if (!PRIV_IS_VF(priv) && pcie_ava)
		dn200_dcbnl_init(priv, false);
	/* Release and free the Rx/Tx resources */
	free_dma_desc_resources(priv);
	if (pcie_ava) {
		dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);
		/* Disable the MAC Rx/Tx */
		dn200_mac_set(priv, priv->ioaddr, false, priv->hw);
	}

	dn200_release_ptp(priv);

	if (PRIV_PHY_OPS_CHECK(priv) && PRIV_PHY_OPS(priv)->stop)
		PRIV_PHY_OPS(priv)->stop(PRIV_PHY_INFO(priv));
	if (pcie_ava) {
		if (priv->mii)
			/* set rgmii rx clock from soc */
			dn200_xgmac_rx_ext_clk_set(priv, false);
	}
	set_bit(DN200_DCB_DOWN, &priv->state);
	if (PRIV_IS_VF(priv) && pcie_ava) {
		DN200_VF_LINK_GET(priv, priv->plat_ex->vf_offset,
				  &vf_link_notify);
		if (vf_link_notify)
			DN200_VF_LINK_SET(priv, priv->plat_ex->vf_offset, 0);
	}
	memset(&priv->hw->set_state, 0, sizeof(struct dn200_set_state));

	clear_bit(DN200_UP, &priv->state);
	return 0;
}

static bool dn200_vlan_insert(struct dn200_priv *priv, struct sk_buff *skb,
			      struct dn200_tx_queue *tx_q)
{
	u16 tag = 0x0, inner_tag = 0x0;
	u32 inner_type = 0x0;
	struct dma_desc *p;

	if (!(priv->dev->features & NETIF_F_HW_VLAN_CTAG_TX))
		return false;
	if (!priv->dma_cap.vlins)
		return false;
	if (!skb_vlan_tag_present(skb))
		return false;
	if (skb->vlan_proto == htons(ETH_P_8021AD))
		return false;

	tag = skb_vlan_tag_get(skb);

	p = &tx_q->dma_tx[tx_q->cur_tx];

	if (dn200_set_desc_vlan_tag(priv, p, tag, inner_tag, inner_type))
		return false;
	priv->swc.mmc_tx_vlan_insert++;
	dn200_set_tx_owner(priv, p);
	tx_q->cur_tx = DN200_GET_ENTRY(tx_q->cur_tx, priv->dma_tx_size);
	return true;
}

/**
 *  dn200_tso_allocator - close entry point of the driver
 *  @priv: driver private structure
 *  @des: buffer start address
 *  @total_len: total length to fill in descriptors
 *  @last_segment: condition for the last descriptor
 *  @queue: TX queue index
 *  Description:
 *  This function fills descriptor and request new descriptors according to
 *  buffer length to fill
 */
static void dn200_tso_allocator(struct dn200_priv *priv, dma_addr_t des,
				int total_len, bool last_segment, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	struct dma_desc *desc;
	u32 buff_size;
	int tmp_len;

	tmp_len = total_len;

	while (tmp_len > 0) {
		dma_addr_t curr_addr;

		tx_q->cur_tx = DN200_GET_ENTRY(tx_q->cur_tx, priv->dma_tx_size);
		WARN_ON(tx_q->tx_skbuff[tx_q->cur_tx]);

		desc = &tx_q->dma_tx[tx_q->cur_tx];

		curr_addr = des + (total_len - tmp_len);
		dn200_set_desc_addr(priv, desc, curr_addr, priv->hw);

		buff_size =
		    tmp_len >= TSO_MAX_BUFF_SIZE ? TSO_MAX_BUFF_SIZE : tmp_len;

		dn200_prepare_tso_tx_desc(priv, desc, 0, buff_size,
						 0, 1,
						 (last_segment) &&
						 (tmp_len <= TSO_MAX_BUFF_SIZE), 0,
						 0);
		tmp_len -= TSO_MAX_BUFF_SIZE;
	}
}

static void dn200_flush_tx_descriptors(struct dn200_priv *priv, int queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	int desc_size;

	desc_size = sizeof(struct dma_desc);

	/* The own bit must be the latest setting done when prepare the
	 * descriptor and then barrier is needed to make sure that
	 * all is coherent before granting the DMA engine.
	 */
	wmb();

	tx_q->tx_tail_addr = tx_q->dma_tx_phy + (tx_q->cur_tx * desc_size);
	dn200_set_tx_tail_ptr(priv, priv->ioaddr, tx_q->tx_tail_addr, queue,
			      priv->hw);
}

static bool dn200_is_vxlan(struct sk_buff *skb)
{
	if (!skb->encapsulation)
		return false;

	switch (skb->protocol) {
	case htons(ETH_P_IP):
		if (ip_hdr(skb)->protocol != IPPROTO_UDP)
			return false;
		break;

	case htons(ETH_P_IPV6):
		if (ipv6_hdr(skb)->nexthdr != IPPROTO_UDP)
			return false;
		break;

	default:
		return false;
	}
	if (skb->inner_protocol_type != ENCAP_TYPE_ETHER ||
	    skb->inner_protocol != htons(ETH_P_TEB) ||
	    (skb_inner_mac_header(skb) - skb_transport_header(skb) !=
	     sizeof(struct udphdr) + sizeof(struct vxlanhdr)))
		return false;
	return true;
}

static int dn200_dma32_buf_get(struct dn200_priv *priv,
			       struct dn200_tx_queue *tx_q, int buf_len,
			       int entry)
{
	int order = 0, pgs = 0;
	struct dn200_tx_dma32_buff *tx_buf = &tx_q->tx_dma32_bufs[entry];

	if (tx_buf->mem_type == DN200_NORMAL) {
		pgs = DIV_ROUND_UP(buf_len, PAGE_SIZE);
		order = ilog2(roundup_pow_of_two(pgs));

		tx_buf->page = __dev_alloc_pages(GFP_ATOMIC | __GFP_NOWARN | GFP_DMA32, order);
		if (!tx_buf->page) {
			dev_err(priv->device, "no page memory: %s, %d.\n",
				__func__, __LINE__);
			return -1;
		}
		tx_buf->order = order;
	} else {
		dev_err(priv->device,
			"%s, %d, tx buf is dma32 already, should release it before use, entry:%d, queue:%d, buf:%pa.\n",
			__func__, __LINE__, entry, tx_q->queue_index,
			&tx_buf->buf);
		return -1;
	}

	return 0;
}

/**
 *  dn200_dma32_allocator - alloc a dma32 tx buffer from input queue's struct
 *  @priv: driver private structure
 *  @data: data buffer start address
 *  @total_len: total length of data
 *  @queue: TX queue index
 *  @entry: Tx buffer index
 *  Description:
 *  This function allocate dma32 buffer and map to dma address,
 *  if success return the mapped dma address, if failure return 0
 */
static dma_addr_t dn200_dma32_allocator(struct dn200_priv *priv, void *data,
					int total_len, u32 queue, int entry,
					u64 *base_addr)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	struct page *page = NULL;
	dma_addr_t dma_addr;
	int ret = 0;
	u8 *dest = NULL;

	ret = dn200_dma32_buf_get(priv, tx_q, total_len, entry);
	if (ret < 0)
		return 0;

	page = tx_q->tx_dma32_bufs[entry].page;
	if (!page) {
		dev_err(priv->device,
			"page is null or dma32 buf get failure: %s, %d, entry:%d.\n",
			__func__, __LINE__, entry);
		return 0;
	}

	dest = page_address(page);
	memcpy(dest, (u8 *)data, total_len);

	dma_addr = dma_map_page_attrs(priv->device, page, 0, total_len, DMA_TO_DEVICE,
			     (DMA_ATTR_SKIP_CPU_SYNC |
				 DMA_ATTR_WEAK_ORDERING));
	if (dma_mapping_error(priv->device, dma_addr)) {
		dev_err(priv->device, "dma map error: %s, %d.\n", __func__,
			__LINE__);
		__free_pages(page, tx_q->tx_dma32_bufs[entry].order);
		return 0;
	}

	/* should find the iatu base address for address lower or higher than 32 bit
	 * when iommu enabled, the dma address can higher than 32 bit,
	 * 1. when lower than 32 bit, just use dma32 iatu base address
	 * 2. when higher that 32 bit, use a new iatu entry's base address
	 */
	*base_addr = dma_addr;
	if (!dma_can_direct_use(priv, dma_addr)) {
		if (dn200_tx_iatu_find
		    (dma_addr, tx_q, &tx_q->tx_dma32_bufs[entry].iatu_ref_ptr,
		     base_addr) < 0) {
			dev_err(priv->device,
				"%s, %d, dma32 addr iatu find failed, dma_addr:%#llx, page phys:%#llx\n",
				__func__, __LINE__, dma_addr,
				page_to_phys(page));
			__free_pages(page, tx_q->tx_dma32_bufs[entry].order);
			dma_unmap_page(priv->device, dma_addr, total_len,
				       DMA_TO_DEVICE);
			return 0;
		}
	}
	priv->swc.tx_mem_copy++;
	priv->tx_mem_copy++;
	tx_q->tx_dma32_bufs[entry].mem_type = DN200_DMA32;
	tx_q->tx_dma32_bufs[entry].buf = dma_addr;
	tx_q->tx_dma32_bufs[entry].len = total_len;

	return dma_addr;
}

/**
 *  dn200_tso_xmit - Tx entry point of the driver for oversized frames (TSO)
 *  @skb : the socket buffer
 *  @dev : device pointer
 *  Description: this is the transmit function that is called on TSO frames
 *  (support available on GMAC4 and newer chips).
 *  Diagram below show the ring programming in case of TSO frames:
 *
 *  First Descriptor
 *   --------
 *   | DES0 |---> buffer1 = L2/L3/L4 header
 *   | DES1 |---> TCP Payload (can continue on next descr...)
 *   | DES2 |---> buffer 1 and 2 len
 *   | DES3 |---> must set TSE, TCP hdr len-> [22:19]. TCP payload len [17:0]
 *   --------
 *	|
 *     ...
 *	|
 *   --------
 *   | DES0 | --| Split TCP Payload on Buffers 1 and 2
 *   | DES1 | --|
 *   | DES2 | --> buffer 1 and 2 len
 *   | DES3 |
 *   --------
 *
 * mss is fixed when enable tso, so w/o programming the TDES3 ctx field.
 */
static netdev_tx_t dn200_tso_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int tunnel_flag = 0;
	struct dma_desc *desc, *first, *mss_desc = NULL;
	struct dn200_priv *priv = netdev_priv(dev);
	int nfrags = skb_shinfo(skb)->nr_frags;
	u32 queue = skb_get_queue_mapping(skb);
	unsigned int first_entry, entry, tx_packets;
	int tmp_pay_len = 0, first_tx;
	struct dn200_tx_queue *tx_q;
	bool has_vlan, set_ic;
	u8 proto_hdr_len, hdr;
	u32 pay_len, mss;
	dma_addr_t des = 0, pre_des = 0;
	int i;
	dma_addr_t dma32_addr;
	u64 base_addr;

	tx_q = &priv->tx_queue[queue];
	first_tx = tx_q->cur_tx;

	/* Compute header lengths */
	if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_L4) {
		proto_hdr_len =
		    skb_transport_offset(skb) + sizeof(struct udphdr);
		hdr = sizeof(struct udphdr);
	} else if (skb_shinfo(skb)->gso_type &
		   (SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM)) {
		/* get inner TCP segmentation header length */
		hdr = inner_tcp_hdrlen(skb);
		proto_hdr_len = skb_inner_transport_offset(skb) + hdr;
		tunnel_flag = TSO_DESC_IS_TUNNEL;
	} else {
		proto_hdr_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
		hdr = tcp_hdrlen(skb);
	}

	/* Desc availability based on threshold should be enough safe */
	if (unlikely(dn200_tx_avail(priv, queue) <
		     (((skb->len - proto_hdr_len) / TSO_MAX_BUFF_SIZE + 1)))) {
		if (!netif_tx_queue_stopped(netdev_get_tx_queue(dev, queue))) {
			netif_tx_stop_queue(netdev_get_tx_queue(priv->dev,
								queue));
			/* This is a hard error, log it. */
			netdev_err(priv->dev,
				   "%s: Tx Ring full when queue awake\n",
				   __func__);
		}
		return NETDEV_TX_BUSY;
	}

	pay_len = skb_headlen(skb) - proto_hdr_len;	/* no frags */

	mss = skb_shinfo(skb)->gso_size;

	/* set new MSS value if needed */
	if (mss != tx_q->mss) {
		mss_desc = &tx_q->dma_tx[tx_q->cur_tx];

		dn200_set_mss(priv, mss_desc, mss);
		tx_q->mss = mss;
		tx_q->cur_tx = DN200_GET_ENTRY(tx_q->cur_tx, priv->dma_tx_size);
		WARN_ON(tx_q->tx_skbuff[tx_q->cur_tx]);
	}

	if (netif_msg_tx_queued(priv)) {
		dev_info(priv->device,
			 "%s: hdrlen %d, hdr_len %d, pay_len %d, mss %d\n",
			 __func__, hdr, proto_hdr_len, pay_len, mss);
		dev_info(priv->device, "\tskb->len %d, skb->data_len %d\n",
			 skb->len, skb->data_len);
	}

	/* Check if VLAN can be inserted by HW */
	has_vlan = dn200_vlan_insert(priv, skb, tx_q);

	first_entry = tx_q->cur_tx;
	WARN_ON(tx_q->tx_skbuff[first_entry]);

	desc = &tx_q->dma_tx[first_entry];
	first = desc;

	if (has_vlan)
		dn200_set_desc_vlan(priv, first, DN200_VLAN_INSERT);

	/* first descriptor: fill Headers on Buf1 */
	des = dma_map_single(priv->device, skb->data, skb_headlen(skb),
			     DMA_TO_DEVICE);
	if (dma_mapping_error(priv->device, des))
		goto dma_map_err;
	tx_q->tx_skbuff_dma[first_entry].buf = des;
	tx_q->tx_skbuff_dma[first_entry].len = skb_headlen(skb);
	tx_q->tx_skbuff_dma[first_entry].map_as_page = false;
	tx_q->tx_skbuff_dma[first_entry].buf_type = DN200_TXBUF_T_SKB;

	dma32_addr = 0;
	if (!dma_can_direct_use(priv, des)) {
		if (dn200_tx_iatu_find
		    (des, tx_q, &tx_q->tx_skbuff_dma[first_entry].iatu_ref_ptr,
		     &base_addr) < 0) {
			dev_dbg(priv->device, "%s %d des %llx\n", __func__,
				__LINE__, des >> 32);
			dma32_addr =
			    dn200_dma32_allocator(priv, skb->data,
						  skb_headlen(skb), queue,
						  first_entry, &base_addr);
			if (dma32_addr) {
				des = dma32_addr;
			} else {
				/* use 64bits as DMA mask, hw limit is 40 or 32 bits,
				 * so the prev dma map output 48bit dma addr can't be used,
				 * just go out
				 */
				goto dma_map_err;
			}
		}

		if (!dma32_addr)
			des = (des & LIMIT_MASK) | base_addr;
		else
			des = (des & MAX_LIMIT_MASK) | base_addr;
	}

	dn200_set_desc_addr(priv, first, des, priv->hw);
	tmp_pay_len = pay_len;
	des += proto_hdr_len;
	pay_len = 0;

	dn200_tso_allocator(priv, des, tmp_pay_len, (nfrags == 0), queue);

	/* Prepare fragments */
	for (i = 0; i < nfrags; i++) {
		unsigned int cur_tx = tx_q->cur_tx;
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		atomic_t *iatu_ref_ptr = NULL;

		des = skb_frag_dma_map(priv->device, frag, 0,
				       skb_frag_size(frag), DMA_TO_DEVICE);
		if (dma_mapping_error(priv->device, des))
			goto dma_map_err;

		pre_des = des;
		dma32_addr = 0;
		if (!dma_can_direct_use(priv, des)) {
			if (dn200_tx_iatu_find
			    (des, tx_q, &iatu_ref_ptr, &base_addr) < 0) {
				void *data =
				    page_address(skb_frag_page(frag)) +
				    skb_frag_off(frag);
				dev_dbg(priv->device, "%s %d des %llx\n",
					__func__, __LINE__, des);
				cur_tx =
				    DN200_GET_ENTRY(cur_tx, priv->dma_tx_size);
				dma32_addr =
				    dn200_dma32_allocator(priv, data,
							  skb_frag_size(frag),
							  queue, cur_tx,
							  &base_addr);
				if (dma32_addr) {
					des = dma32_addr;
				} else {
					/* use 64bits as DMA mask, hw limit is 40 or 32 bits,
					 * so the prev dma map output 48bit dma addr can't be used,
					 * just go out
					 */
					goto dma_map_err;
				}
			}
			if (!dma32_addr)
				des = (des & LIMIT_MASK) | base_addr;
			else
				des = (des & MAX_LIMIT_MASK) | base_addr;
		}
		dn200_tso_allocator(priv, des, skb_frag_size(frag),
				    (i == nfrags - 1), queue);

		tx_q->tx_skbuff_dma[tx_q->cur_tx].buf = pre_des;
		tx_q->tx_skbuff_dma[tx_q->cur_tx].len = skb_frag_size(frag);
		tx_q->tx_skbuff_dma[tx_q->cur_tx].map_as_page = true;
		tx_q->tx_skbuff_dma[tx_q->cur_tx].buf_type = DN200_TXBUF_T_SKB;
		tx_q->tx_skbuff_dma[tx_q->cur_tx].iatu_ref_ptr = iatu_ref_ptr;

		if (dma32_addr) {
			/* move dma32 tx buffer from previous cur_tx to current cur_tx
			 * that is updated in tso allocator
			 */
			if (cur_tx != tx_q->cur_tx) {
				tx_q->tx_dma32_bufs[tx_q->cur_tx] =
				    tx_q->tx_dma32_bufs[cur_tx];
				tx_q->tx_dma32_bufs[cur_tx].mem_type =
				    DN200_NORMAL;
				tx_q->tx_dma32_bufs[cur_tx].iatu_ref_ptr = NULL;
			}
		}
	}

	tx_q->tx_skbuff_dma[tx_q->cur_tx].last_segment = true;

	/* Only the last descriptor gets to point to the skb. */
	tx_q->tx_skbuff[tx_q->cur_tx] = skb;
	tx_q->tx_skbuff_dma[tx_q->cur_tx].buf_type = DN200_TXBUF_T_SKB;

	/* Manage tx mitigation */
	desc = &tx_q->dma_tx[tx_q->cur_tx];
	tx_packets = dn200_ring_entries_calc(priv->dma_tx_size, first_tx, tx_q->cur_tx);
	tx_q->tx_count_frames += tx_packets;
	priv->tx_intr[queue].packet += tx_packets;

	if ((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) && priv->hwts_tx_en)
		set_ic = true;
	else if (!priv->tx_coal_frames[queue])
		set_ic = false;
	else if (tx_packets > priv->tx_coal_frames[queue])
		set_ic = true;
	else if ((tx_q->tx_count_frames %
		  priv->tx_coal_frames[queue]) < tx_packets)
		set_ic = true;
	else
		set_ic = false;

	if (set_ic) {
		tx_q->tx_count_frames = 0;
		dn200_set_tx_ic(priv, desc);
		priv->xstats.tx_set_ic_bit++;
	}

	/* We've used all descriptors we need for this skb, however,
	 * advance cur_tx so that it references a fresh descriptor.
	 * ndo_start_xmit will fill this descriptor the next time it's
	 * called and dn200_tx_clean may clean up to this descriptor.
	 */
	tx_q->cur_tx = DN200_GET_ENTRY(tx_q->cur_tx, priv->dma_tx_size);

	if (unlikely(dn200_tx_avail(priv, queue) <= (MAX_SKB_FRAGS + 1))) {
		netif_dbg(priv, hw, priv->dev, "%s: stop transmitted packets\n",
			  __func__);
		netif_tx_stop_queue(netdev_get_tx_queue(priv->dev, queue));
	}

	dev->stats.tx_bytes += skb->len;
	priv->tx_intr[queue].bytes += skb->len;
	priv->xstats.tx_tso_frames++;
	priv->xstats.tx_tso_nfrags += nfrags;
	netdev_tx_sent_queue(netdev_get_tx_queue(dev, queue), skb->len);

	if (priv->sarc_type)
		dn200_set_desc_sarc(priv, first, priv->sarc_type);

	skb_tx_timestamp(skb);

	if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
		     priv->hwts_tx_en)) {
		/* declare that device is doing timestamping */
		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
		dn200_enable_tx_timestamp(priv, first);
	}
	/* Complete the first descriptor before granting the DMA */
	dn200_prepare_tso_tx_desc(priv, first, TSO_DESC_IS_FIRST | tunnel_flag,
				  proto_hdr_len,
				  pay_len,
				  1,
				  tx_q->tx_skbuff_dma[first_entry].last_segment,
				  hdr / 4, (skb->len - proto_hdr_len));

	/* If context desc is used to change MSS */
	if (mss_desc) {
		/* Make sure that first descriptor has been completely
		 * written, including its own bit. This is because MSS is
		 * actually before first descriptor, so we need to make
		 * sure that MSS's own bit is the last thing written.
		 */
		dma_wmb();
		dn200_set_tx_owner(priv, mss_desc);
	}

	if (netif_msg_pktdata(priv)) {
		dev_info(priv->device,
			 "%s: curr=%d dirty=%d f=%d, e=%d, f_p=%p, nfrags %d\n",
			 __func__, tx_q->cur_tx, tx_q->dirty_tx, first_entry,
			 tx_q->cur_tx, first, nfrags);
		dev_info(priv->device, ">>> frame to be transmitted: ");
		print_pkt(skb->data, skb_headlen(skb));
	}

	dn200_flush_tx_descriptors(priv, queue);
	if (atomic_read(&tx_q->txtimer_running) == 0)
		dn200_tx_timer_arm(priv, queue);
	/* Make sure that data has been completely written. */
	tx_q->next_to_watch = tx_q->cur_tx;
	return NETDEV_TX_OK;
dma_map_err:
	entry = tx_q->cur_tx;
	for (;;) {
		tx_q->tx_skbuff[entry] = NULL;
		desc = tx_q->dma_tx + entry;
		dn200_unmap_txbuff(priv, tx_q, entry);
		dn200_free_dma32_tx_buffer(priv, tx_q, entry);
		dn200_release_tx_desc(priv, desc, priv->mode);
		if (entry == first_entry)
			break;

		entry = DN200_GET_PREVENTRY(entry, priv->dma_tx_size);
	}
	if (has_vlan) {
		entry = DN200_GET_PREVENTRY(entry, priv->dma_tx_size);
		desc = tx_q->dma_tx + entry;
		dn200_release_tx_desc(priv, desc, priv->mode);
	}
	tx_q->cur_tx = entry;
	dev_err(priv->device, "Tx dma map failed\n");
	dev_kfree_skb(skb);
	priv->dev->stats.tx_dropped++;
	return NETDEV_TX_OK;
}

/**
 *  dn200_xmit - Tx entry point of the driver
 *  @skb : the socket buffer
 *  @dev : device pointer
 *  Description : this is the tx entry point of the driver.
 *  It programs the chain or the ring and supports oversized frames
 *  and SG feature.
 */
static netdev_tx_t dn200_xmit(struct sk_buff *skb, struct net_device *dev)
{
	unsigned int first_entry, tx_packets, enh_desc;
	struct dn200_priv *priv = netdev_priv(dev);
	unsigned int nopaged_len = skb_headlen(skb);
	int i, csum_insertion = 0, is_jumbo = 0;
	u32 queue = skb_get_queue_mapping(skb);
	int nfrags = skb_shinfo(skb)->nr_frags;
	int gso = skb_shinfo(skb)->gso_type;
	struct dma_desc *desc, *first;
	struct dn200_tx_queue *tx_q;
	bool has_vlan, set_ic, iatu_lack = false;
	int entry, first_tx;
	dma_addr_t des;
	dma_addr_t dma32_addr;
	u64 base_addr;

	if (test_bit(DN200_DOWN, &priv->state))
		return NETDEV_TX_BUSY;
	tx_q = &priv->tx_queue[queue];
	first_tx = tx_q->cur_tx;
	if (priv->tx_path_in_lpi_mode && priv->eee_sw_timer_en)
		dn200_disable_eee_mode(priv);
	/* Manage oversized TCP frames for GMAC4 device */
	if (skb_is_gso(skb) && priv->tso) {
		if (gso & (SKB_GSO_TCPV4 | SKB_GSO_TCPV6))
			return dn200_tso_xmit(skb, dev);
	}

	if (unlikely(dn200_tx_avail(priv, queue) < nfrags + 1)) {
		if (!netif_tx_queue_stopped(netdev_get_tx_queue(dev, queue))) {
			netif_tx_stop_queue(netdev_get_tx_queue(priv->dev,
								queue));
			/* This is a hard error, log it. */
			netdev_err(priv->dev,
				   "%s: Tx Ring full when queue awake\n",
				   __func__);
		}
		return NETDEV_TX_BUSY;
	}
	if (netif_msg_tx_queued(priv)) {
		dev_info(priv->device,
			 "%s, %d: Tx Ring avial:%d, need:%d\n",
			 __func__, __LINE__, dn200_tx_avail(priv, queue),
			 nfrags);
		dev_info(priv->device, "\tskb->len %d, skb->data_len %d\n",
			 skb->len, skb->data_len);
	}
	/* Check if VLAN can be inserted by HW */
	has_vlan = dn200_vlan_insert(priv, skb, tx_q);

	entry = tx_q->cur_tx;
	first_entry = entry;
	WARN_ON(tx_q->tx_skbuff[first_entry]);

	csum_insertion = (skb->ip_summed == CHECKSUM_PARTIAL);
	desc = tx_q->dma_tx + entry;

	first = desc;

	if (has_vlan)
		dn200_set_desc_vlan(priv, first, DN200_VLAN_INSERT);
	if (dn200_is_vxlan(skb))
		dn200_set_vxlan(priv, first);

	enh_desc = priv->plat->enh_desc;
	/* To program the descriptors according to the size of the frame */
	if (enh_desc)
		is_jumbo = dn200_is_jumbo_frm(priv, skb->len, enh_desc);

	if (unlikely(is_jumbo)) {
		entry = dn200_jumbo_frm(priv, tx_q, skb, csum_insertion);
		if (unlikely(entry < 0) && (entry != -EINVAL))
			goto dma_map_err;
	}

	for (i = 0; i < nfrags; i++) {
		const skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		int len = skb_frag_size(frag);
		bool last_segment = (i == (nfrags - 1));

		entry = DN200_GET_ENTRY(entry, priv->dma_tx_size);
		WARN_ON(tx_q->tx_skbuff[entry]);
		desc = tx_q->dma_tx + entry;

		des = skb_frag_dma_map(priv->device, frag, 0, len,
				       DMA_TO_DEVICE);
		if (dma_mapping_error(priv->device, des))
			goto dma_map_err;	/* should reuse desc w/o issues */
		tx_q->tx_skbuff_dma[entry].buf = des;
		tx_q->tx_skbuff_dma[entry].map_as_page = true;
		tx_q->tx_skbuff_dma[entry].len = len;
		tx_q->tx_skbuff_dma[entry].last_segment = last_segment;
		tx_q->tx_skbuff_dma[entry].buf_type = DN200_TXBUF_T_SKB;

		dma32_addr = 0;
		if (!dma_can_direct_use(priv, des)) {
			if (dn200_tx_iatu_find
			    (des, tx_q,
			     &tx_q->tx_skbuff_dma[entry].iatu_ref_ptr,
			     &base_addr) < 0) {
				void *data =
				    page_address(skb_frag_page(frag)) +
				    skb_frag_off(frag);
				iatu_lack = true;
				dma32_addr =
				    dn200_dma32_allocator(priv, data, len,
							  queue, entry,
							  &base_addr);
				if (dma32_addr) {
					des = dma32_addr;
				} else {
					/* use 64bits as DMA mask, hw limit is 40 or 32 bits,
					 * so the prev dma map output 48bit dma addr can't be used,
					 * just go out
					 */
					goto dma_map_err;
				}
			}

			if (!dma32_addr)
				des = (des & LIMIT_MASK) | base_addr;
			else
				des = (des & MAX_LIMIT_MASK) | base_addr;
		}
		dn200_set_desc_addr(priv, desc, des, priv->hw);
		/* Prepare the descriptor and set the own bit too */
		dn200_prepare_tx_desc(priv, desc, 0, len, csum_insertion,
					priv->mode, 1, last_segment, skb->len);
	}

	/* Only the last descriptor gets to point to the skb. */
	tx_q->tx_skbuff[entry] = skb;
	tx_q->tx_skbuff_dma[entry].buf_type = DN200_TXBUF_T_SKB;

	/* According to the coalesce parameter the IC bit for the latest
	 * segment is reset and the timer re-started to clean the tx status.
	 * This approach takes care about the fragments: desc is the first
	 * element in case of no SG.
	 */
	tx_packets = dn200_ring_entries_calc(priv->dma_tx_size, first_tx, entry);
	tx_q->tx_count_frames += tx_packets;
	priv->tx_intr[queue].packet += tx_packets;

	if ((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) && priv->hwts_tx_en)
		set_ic = true;
	else if (!priv->tx_coal_frames[queue])
		set_ic = false;
	else if (tx_packets > priv->tx_coal_frames[queue])
		set_ic = true;
	else if ((tx_q->tx_count_frames %
		  priv->tx_coal_frames[queue]) < tx_packets)
		set_ic = true;
	else
		set_ic = false;

	if (set_ic || iatu_lack) {
		desc = &tx_q->dma_tx[entry];

		tx_q->tx_count_frames = 0;
		dn200_set_tx_ic(priv, desc);
		priv->xstats.tx_set_ic_bit++;
	}

	/* We've used all descriptors we need for this skb, however,
	 * advance cur_tx so that it references a fresh descriptor.
	 * ndo_start_xmit will fill this descriptor the next time it's
	 * called and dn200_tx_clean may clean up to this descriptor.
	 */
	entry = DN200_GET_ENTRY(entry, priv->dma_tx_size);
	tx_q->cur_tx = entry;

	if (netif_msg_pktdata(priv)) {
		netdev_info(priv->dev,
			    "%s: curr=%d dirty=%d f=%d, e=%d, first=%p, nfrags=%d",
			    __func__, tx_q->cur_tx, tx_q->dirty_tx, first_entry,
			    entry, first, nfrags);

		netdev_info(priv->dev, ">>> frame to be transmitted: ");
		print_pkt(skb->data, skb->len);
	}
	if (unlikely(dn200_tx_avail(priv, queue) <= (MAX_SKB_FRAGS + 1))) {
		netif_dbg(priv, hw, priv->dev, "%s: stop transmitted packets\n",
			  __func__);
		netif_tx_stop_queue(netdev_get_tx_queue(priv->dev, queue));
	}

	dev->stats.tx_bytes += skb->len;
	priv->tx_intr[queue].bytes += skb->len;
	netdev_tx_sent_queue(netdev_get_tx_queue(dev, queue), skb->len);
	if (priv->sarc_type)
		dn200_set_desc_sarc(priv, first, priv->sarc_type);

	skb_tx_timestamp(skb);

	/* Ready to fill the first descriptor and set the OWN bit w/o any
	 * problems because all the descriptors are actually ready to be
	 * passed to the DMA engine.
	 */
	if (likely(!is_jumbo)) {
		bool last_segment = (nfrags == 0);

		des = dma_map_single(priv->device, skb->data,
				     nopaged_len, DMA_TO_DEVICE);
		if (dma_mapping_error(priv->device, des))
			goto dma_map_err;

		tx_q->tx_skbuff_dma[first_entry].buf = des;
		tx_q->tx_skbuff_dma[first_entry].buf_type = DN200_TXBUF_T_SKB;
		tx_q->tx_skbuff_dma[first_entry].map_as_page = false;
		tx_q->tx_skbuff_dma[first_entry].len = nopaged_len;
		tx_q->tx_skbuff_dma[first_entry].last_segment = last_segment;

		dma32_addr = 0;
		if (!dma_can_direct_use(priv, des)) {
			if (dn200_tx_iatu_find
			    (des, tx_q,
			     &tx_q->tx_skbuff_dma[first_entry].iatu_ref_ptr,
			     &base_addr) < 0) {
				iatu_lack = true;
				dma32_addr =
				    dn200_dma32_allocator(priv, skb->data,
							  nopaged_len, queue,
							  first_entry,
							  &base_addr);
				if (dma32_addr) {
					des = dma32_addr;
				} else {
					/* use 64bits as DMA mask, hw limit is 40 or 32 bits,
					 * so the prev dma map output 48bit dma addr can't be used,
					 * just go out
					 */
					dev_err(priv->device,
						"%s %d dma32_allocator failed\n",
						__func__, __LINE__);
					goto dma_map_err;
				}
			}

			/* 1. for normal addr: the iatu limit can be changed
			 * 2. for dam32 addr: the iatu limit can not be changed, force to use 32bit
			 * you can change LIMIT_MASK(26 or 28 bit) to verify
			 * different address range (e.g. 64MB, 256MB) for one iATU to cover
			 */
			if (!dma32_addr)
				des = (des & LIMIT_MASK) | base_addr;
			else
				des = (des & MAX_LIMIT_MASK) | base_addr;
		}
		dn200_set_desc_addr(priv, first, des, priv->hw);

		if (unlikely((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) &&
			     priv->hwts_tx_en)) {
			/* declare that device is doing timestamping */
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
			dn200_enable_tx_timestamp(priv, first);
		}

		/* Prepare the first descriptor setting the OWN bit too */
		dn200_prepare_tx_desc(priv, first, 1, nopaged_len,
				      csum_insertion, priv->mode, 0,
				      last_segment, skb->len);
	}

	dn200_set_tx_owner(priv, first);

	dn200_enable_dma_transmission(priv, priv->ioaddr);
	if (netif_xmit_stopped(netdev_get_tx_queue(priv->dev, queue)) || !netdev_xmit_more()) {
		dn200_flush_tx_descriptors(priv, queue);
		if (atomic_read(&tx_q->txtimer_running) == 0)
			dn200_tx_timer_arm(priv, queue);
	}
	/* Make sure that data has been completely written. */
	tx_q->next_to_watch = tx_q->cur_tx;
	return NETDEV_TX_OK;
dma_map_err:
	for (;;) {
		tx_q->tx_skbuff[entry] = NULL;
		desc = tx_q->dma_tx + entry;
		dn200_unmap_txbuff(priv, tx_q, entry);
		dn200_free_dma32_tx_buffer(priv, tx_q, entry);
		dn200_release_tx_desc(priv, desc, priv->mode);
		if (entry == first_entry)
			break;

		entry = DN200_GET_PREVENTRY(entry, priv->dma_tx_size);
	}

	if (has_vlan) {
		entry = DN200_GET_PREVENTRY(entry, priv->dma_tx_size);
		desc = tx_q->dma_tx + entry;
		dn200_release_tx_desc(priv, desc, priv->mode);
	}
	tx_q->cur_tx = entry;
	netdev_err(priv->dev, "Tx DMA map failed\n");
	dev_kfree_skb(skb);
	priv->dev->stats.tx_dropped++;
	return NETDEV_TX_OK;
}

static void dn200_rx_vlan(struct dn200_priv *priv, struct sk_buff *skb,
			  struct dma_desc *p)
{
	u16 vlanid;

	if ((priv->dev->features & NETIF_F_HW_VLAN_CTAG_RX) || PRIV_IS_VF(priv)) {
		vlanid = dn200_get_ovt(priv, p);
		if (!vlanid)
			return;

		priv->swc.mmc_rx_vlan_strip++;
		__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vlanid);
	}
}

/* Base on HW curr ptr(pidx) and tail ptr to debug the ring state:
 * Ring empty: curr ptr == tail
 * Ring full: avail_desc < MIN_RX_FREE_DES
 */

/**
 * dn200_rx_refill - refill used skb preallocated buffers
 * @priv: driver private structure
 * @queue: RX queue index
 * Description : this is to reallocate the skb for the reception process
 * that is based on zero-copy.
 *
 * Returns false if all allocations were successful, true if any fail. Returning
 * true signals to the caller that we didn't replace cleaned_count buffers and
 * there is more work to do.
 */
static inline bool dn200_rx_refill(struct dn200_priv *priv, u32 queue,
				   u32 q_depth, int cleaned_count)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	int dirty = dn200_rx_dirty(priv, queue);
	unsigned int entry = rx_q->dirty_rx;
	struct dn200_rx_buffer *buf;
	struct dma_desc *p;

	if (unlikely(dirty == 0 && cleaned_count > 0)) {
		dev_err(priv->device,
			"%s, %d, rx ring %d is abnormal, dirty is 0, but cleaned %d, cur rx:%d, dirty rx:%d, ring depth:%d\n",
			__func__, __LINE__, queue, cleaned_count, rx_q->cur_rx,
			rx_q->dirty_rx, priv->dma_rx_size);
	}

	/* do nothing if no valid cleaned_count */
	if (!cleaned_count)
		return false;
	dirty = cleaned_count;

	while (dirty > 0) {
		buf = &rx_q->buf_pool[entry];
		p = rx_q->dma_rx + entry;

		if (dn200_alloc_page
		    (priv, rx_q, buf, FIRST_PAGE, entry, q_depth)) {
			netdev_err(priv->dev,
				   "alloc page failure for rx buf\n");
			break;
		}

		/* sync the buffer for use by the device */
		dma_sync_single_range_for_device(priv->device, buf->kernel_addr,
						 buf->page_offset,
						 priv->dma_buf_sz,
						 DMA_FROM_DEVICE);
		dn200_set_desc_addr(priv, p, buf->desc_addr + buf->page_offset,
				    priv->hw);

		dn200_set_desc_sec_addr(priv, p, 0, false, priv->hw);

		dn200_refill_desc3(priv, rx_q, p);

		dma_wmb();
		dn200_set_rx_owner(priv, p, 0);
		buf->rx_times = 0;

		entry = DN200_GET_ENTRY(entry, q_depth);
		dirty--;
	}
	/*use old tail addr to debug rx ring state */

	rx_q->dirty_rx = entry;

	if (unlikely(rx_q->dirty_rx == rx_q->cur_rx))
		dev_err(priv->device,
			"%s, %d, must not happen (dirty rx == curr rx), dirty:%d, cur rx:%d\n",
			__func__, __LINE__, rx_q->dirty_rx, rx_q->cur_rx);

	rx_q->rx_tail_addr = rx_q->dma_rx_phy +
	    (rx_q->dirty_rx * sizeof(struct dma_desc));
	dn200_set_rx_tail_ptr(priv, priv->ioaddr, rx_q->rx_tail_addr, queue,
			      priv->hw);

	return !!dirty;
}

static unsigned int dn200_rx_buf1_len(struct dn200_priv *priv,
				      struct dma_desc *p,
				      int status, unsigned int len)
{
	unsigned int plen = 0;
	int coe = priv->hw->rx_csum;

	/* First descriptor, not last descriptor and not split header */
	if (status & rx_not_ls)
		return priv->dma_buf_sz;

	plen = dn200_get_rx_frame_len(priv, p, coe);

	plen = plen - len;

	/* First descriptor and last descriptor and not split header */
	return plen;
}

#define DN200_DESC_UNUSED(priv, rx_q)                                 \
	((((rx_q)->cur_rx > (rx_q)->dirty_rx) ? 0 : (priv)->dma_rx_size) + \
	 (rx_q)->cur_rx - (rx_q)->dirty_rx - 1)


static inline void
dn200_get_ntuple_filter_num(struct dn200_priv *priv, struct dma_desc *desc,
			    u8 *status, u32 *ntuple_drop)
{
	u8 filter_no = 0;

	if (!(priv->dev->features & NETIF_F_NTUPLE))
		return;

	if (desc->des2 & (XGMAC_RDES2_L4FM | XGMAC_RDES2_L3FM)) {
		filter_no =
		    (desc->des2 & XGMAC_RDES2_MADRM) >> XGMAC_RDES2_MADRM_SHIFT;
		if ((1 << filter_no) & priv->fdir_map) {
			*status |= discard_frame;
			priv->swc.mmc_rx_fd_drop++;
			(*ntuple_drop)++;
		}
	}
}

/**
 * dn200_rx - manage the receive process
 * @priv: driver private structure
 * @limit: napi bugget
 * @queue: RX queue index.
 * Description :  this the function called by the napi poll method.
 * It gets all the frames inside the ring.
 */
static int dn200_rx(struct dn200_priv *priv, int limit, u32 queue, u32 q_depth)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned int count = 0, error = 0, len = 0, ntuple_drop = 0;
	int status = 0, coe = priv->hw->rx_csum;
	u8 filter_status = 0;
	unsigned int next_entry = rx_q->cur_rx;
	enum dma_data_direction dma_dir;
	struct sk_buff *skb = NULL;
	struct page *page = NULL;
	unsigned char *hard_start;
	int cleaned_count = DN200_DESC_UNUSED(priv, rx_q);
	bool is_failure = false;
	void *buf_addr = NULL;

	dma_dir = DMA_FROM_DEVICE;

	if (netif_msg_rx_status(priv)) {
		void *rx_head;
		u16 desc_size;

		netdev_info(priv->dev, "%s: descriptor ring:\n", __func__);
		rx_head = (void *)rx_q->dma_rx;
		desc_size = sizeof(struct dma_desc);

		dn200_display_ring(priv, rx_head, q_depth, true,
				   rx_q->dma_rx_phy, desc_size, priv->hw);
	}
	if (cleaned_count >= dn200_rx_refill_size(priv)) {
		is_failure =
			dn200_rx_refill(priv, queue, q_depth, cleaned_count);

		if (unlikely(is_failure)) {
			dev_err(priv->device,
				"%s %d rx refill failure %d\n",
				__func__, __LINE__, is_failure);
			goto err_refill_fail;
		}
		cleaned_count = 0;
	}

	while (count < limit) {
		unsigned int buf1_len = 0, buf2_len = 0;
		enum pkt_hash_types hash_type;
		struct dn200_rx_buffer *buf;
		struct dma_desc *np, *p;
		int entry;
		u32 hash;

		if (unlikely(!count && rx_q->state_saved)) {
			skb = rx_q->state.skb;
			error = rx_q->state.error;
			len = rx_q->state.len;
		} else {
			rx_q->state_saved = false;
			skb = NULL;
			error = 0;
			len = 0;
		}

		if (count >= limit)
			break;

		/* return some buffers to hardware
		 * fix issue: in the condition of 64 depth of desc ring,
		 * napi budget is more than available desc number(63)
		 * will have opportunity recv packet twice from
		 * one rx desc(almost always happen when rx burst)
		 */
		if (cleaned_count >= dn200_rx_refill_size(priv)) {
			is_failure =
			    dn200_rx_refill(priv, queue, q_depth, cleaned_count);

			if (unlikely(is_failure)) {
				dev_err(priv->device,
					"%s %d rx refill failure %d\n",
					__func__, __LINE__, is_failure);
				goto err_refill_fail;
			}
			cleaned_count = 0;
		}

read_again:
		buf1_len = 0;
		buf2_len = 0;
		entry = next_entry;
		buf = &rx_q->buf_pool[entry];
		p = rx_q->dma_rx + entry;
		/* read the status of the incoming frame */
		status = dn200_rx_status(priv, &priv->dev->stats,
					 &priv->xstats, p, priv->rec_all);
		/* check if managed by the DMA otherwise go ahead */
		if (unlikely(status & dma_own))
			break;

		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * dma_own bit is set.
		 */
		dma_rmb();
		rx_q->cur_rx = DN200_GET_ENTRY(rx_q->cur_rx, q_depth);
		next_entry = rx_q->cur_rx;
		np = rx_q->dma_rx + next_entry;
		prefetch(np);

		dn200_get_ntuple_filter_num(priv, p, &filter_status,
					    &ntuple_drop);
		buf1_len = dn200_rx_buf1_len(priv, p, status, len);
		if (buf1_len > priv->dma_buf_sz) {
			dev_err(priv->device,
					"%s, %d, invalid buf len %d.\n",
					__func__, __LINE__, buf1_len);
			status |= buf_len_err;
		}
		if (unlikely
		    (status & (discard_frame | buf_len_err) ||
				 filter_status == discard_frame)) {
			error = 1;
			if (!priv->hwts_rx_en)
				priv->dev->stats.rx_errors++;
		}

		if (unlikely(error && (status & rx_not_ls))) {
			cleaned_count++;
			dn200_rx_pool_buf_free(rx_q->rx_pool, rx_q->queue_index,
					       buf->pg_buf);
			goto read_again;
		}
		if (unlikely(error)) {
			if (skb) {
				dev_kfree_skb(skb);
				skb = NULL;
			}
			count++;
			cleaned_count++;
			dn200_rx_pool_buf_free(rx_q->rx_pool, rx_q->queue_index,
					       buf->pg_buf);
			if (status & buf_len_err) {
				netdev_err(priv->dev, "buf len err %d\n", buf1_len);
				set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
				set_bit(DN200_PCIE_UNAVAILD, &priv->state);
				dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
				break;
			}
			continue;
		}

		/* Buffer is good. Go on. */
		/* page_offset can be the first or sencond part of the page */
		buf_addr = page_address(buf->page) + buf->page_offset;
		hard_start = buf_addr - dn200_rx_offset(priv);
		net_prefetch(buf_addr);
		len += buf1_len;
		if (!skb) {
			if (unlikely(buf->pg_buf->low_res)) {
				if (priv->txrx_itr_combined)
					skb =
					    napi_alloc_skb(&ch->agg_napi,
							   buf1_len);
				else
					skb =
					    napi_alloc_skb(&ch->rx_napi,
							   buf1_len);
				if (unlikely(!skb)) {
					dev_err(priv->device,
						"%s, %d, build skb fail.\n",
						__func__, __LINE__);
					goto drain_data;
				}
				skb_copy_to_linear_data(skb,
							(unsigned char
							 *)(hard_start +
							    dn200_rx_offset
							    (priv)), buf1_len);
				priv->swc.rx_mem_copy++;
			} else {
				skb = build_skb(hard_start, DN200_RX_BUF_SIZE);
				if (unlikely(!skb)) {
					dev_err(priv->device,
						"%s, %d, build skb fail.\n",
						__func__, __LINE__);
					goto drain_data;
				}
				skb_reserve(skb, dn200_rx_offset(priv));
			}
			skb_put(skb, buf1_len);
			dn200_rx_pool_buf_free(rx_q->rx_pool, rx_q->queue_index,
					       buf->pg_buf);
			cleaned_count++;
		} else if (buf1_len) {
			if (unlikely(buf->pg_buf->low_res)) {
				page =
				    dev_alloc_pages(dn200_rx_pg_order_get
						    (priv));
				if (unlikely(!page)) {
					dev_err(priv->device,
						"%s, %d, build frag fail.\n",
						__func__, __LINE__);
					dev_kfree_skb(skb);
					goto drain_data;
				}
				priv->swc.rx_mem_copy++;
				memcpy(page_to_virt(page), buf_addr, buf1_len);
				skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
						page, 0, buf1_len, PAGE_SIZE);
			} else {
				dma_sync_single_range_for_cpu(priv->device,
							      buf->kernel_addr,
							      buf->page_offset,
							      buf1_len,
							      dma_dir);
				skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags,
						buf->page, buf->page_offset,
						buf1_len, priv->dma_buf_sz);
			}
			dn200_rx_pool_buf_free(rx_q->rx_pool, rx_q->queue_index,
					       buf->pg_buf);
			cleaned_count++;
		}

drain_data:
		if (unlikely(status & rx_not_ls))
			goto read_again;
		if (!skb)
			continue;
		/* Got entire packet into SKB. Finish it. */
		dn200_get_rx_hwtstamp(priv, p, np, skb);
		dn200_rx_vlan(priv, skb, p);
		skb->protocol = eth_type_trans(skb, priv->dev);
		if (unlikely(!coe))
			skb_checksum_none_assert(skb);
		else
			skb->ip_summed = CHECKSUM_UNNECESSARY;
		if (!dn200_get_rx_hash(priv, p, &hash, &hash_type))
			skb_set_hash(skb, hash, hash_type);
		skb_record_rx_queue(skb, queue);
		if (priv->txrx_itr_combined)
			napi_gro_receive(&ch->agg_napi, skb);
		else
			napi_gro_receive(&ch->rx_napi, skb);
		skb = NULL;
		priv->dev->stats.rx_packets++;
		priv->dev->stats.rx_bytes += len;
		count++;
		priv->rx_intr[queue].bytes += len;
	}

	if (status & rx_not_ls || skb) {
		rx_q->state_saved = true;
		rx_q->state.skb = skb;
		rx_q->state.error = error;
		rx_q->state.len = len;
	}
err_refill_fail:
	priv->rx_intr[queue].packet += count;
	priv->xstats.rx_pkt_n += (count - ntuple_drop);
	priv->xstats.rxq_stats[queue].rx_pkt_n += (count - ntuple_drop);
	/* if refill status is failure,
	 * return budget limit(e.g. 64) to put current napi to schedule list
	 */
	return is_failure ? limit : (int)count;
}

/**
 *  dn200_update_ring_itr - update the dynamic ITR value based on packet size
 *  @q_vector: pointer to q_vector
 *
 *  Stores a new ITR value based on strictly on packet size.  This
 *  algorithm is less sophisticated than that used in dn200_update_itr,
 *  due to the difficulty of synchronizing statistics across multiple
 *  receive rings.  The divisors and thresholds used by this function
 *  were determined based on theoretical maximum wire speed and testing
 *  data, in order to minimize response time while increasing bulk
 *  throughput.
 *  This functionality is controlled by ethtool's coalescing settings.
 *  NOTE:  This function is called only when operating in a multiqueue
 *         receive environment.
 **/
static void dn200_update_1G_speed_itr(struct dn200_itr_info *itr,
				struct dn200_priv *priv, u8 chan)
{
	u64 avg_wire_size = 0, packets = 0, bytes = 0, itr_usec = 0;
	unsigned long next_update = jiffies;
	u32 max_usec = priv->rx_itr_usec;
	u32 min_usec = priv->rx_itr_usec_min;

	/* These will do nothing if dynamic updates are not enabled */
	if (!(itr->itr_setting & DN200_ITR_DYNAMIC_ITR))
		return;

	if (time_after(next_update, itr->next_update))
		goto clear_counts;

	packets = itr->packet;
	bytes = itr->bytes;
	if (!packets || !bytes) {
		itr_usec = itr->target_itr;
		goto clear_counts;
	}

	avg_wire_size = bytes / packets;

	/* if avg_wire_size isn't set no work was done */
	if (!avg_wire_size)
		goto clear_counts;

	/* Add 24 bytes to size to account for CRC, preamble, and gap */
	avg_wire_size += 24;

	/* Give a little boost to mid-size frames */
	if (avg_wire_size >= 512 && avg_wire_size < 1200) {
		itr_usec = avg_wire_size / 30;
	} else if (avg_wire_size > 128 && avg_wire_size < 512) {
		itr_usec = avg_wire_size / 10;
	} else {
		itr_usec = avg_wire_size / 5;
		/* workaround: fix bonding iperf tx slow issue caused by
		 * rx small packet with high interrupt frequency(low rx-usecs)
		 * reason:
		 * 1. tx tso pkts lead to many rx acks for one tso pkt,
		 *    so at best receive multi ack with one interrupt,
		 *    but just deal with one ack with one hw interrupt in high interrupt frequency
		 * 2. tx & rx are two irqs, if aggregate together,
		 *    will prcocess more ack in once softirq
		 */
		if (test_bit(DN200_IS_BONDING, &priv->state))
			itr_usec = max_usec;
	}

	if (itr_usec > max_usec)
		itr_usec = max_usec;

clear_counts:
	/* write back value */
	itr->target_itr = (itr_usec & DN200_ITR_MASK);
	if (unlikely(itr->target_itr < min_usec)) {
		netdev_dbg(priv->dev, "invalid target itr usec:%u, rx itr usec:%u\n",
			itr->target_itr, priv->rx_itr_usec);
		itr->target_itr = min_usec;
	}
	/* next update should occur within next jiffy */
	itr->next_update = next_update + msecs_to_jiffies(1);

	itr->bytes = 0;
	itr->packet = 0;
}

static void dn200_rx_itr_update(struct dn200_itr_info *itr,
				struct dn200_priv *priv, u8 chan)
{
	u64 avg_wire_size = 0, packets = 0, bytes = 0, itr_usec;
	unsigned long next_update = jiffies;
	u32 max_usec = priv->rx_itr_usec;

	if (priv->plat_ex->phy_info->speed == SPEED_1000)
		max_usec = 0x20;

	itr_usec = priv->min_usecs | DN200_ITR_ADAPTIVE_LATENCY;

	/* These will do nothing if dynamic updates are not enabled */
	if (!(itr->itr_setting & DN200_ITR_DYNAMIC_ITR))
		return;

	packets = itr->packet;
	bytes = itr->bytes;
	if (!packets || !bytes)
		return;

	if (time_after(next_update, itr->next_update))
		goto clear_counts;

	if (itr->itr_countdown) {
		itr_usec = itr->target_itr;
		goto clear_counts;
	}

	if (packets && packets == 1) {
		itr_usec = itr->target_itr;
		itr_usec &= DN200_ITR_MASK;
		/* If packet count is 1 likely looking
		 * at a slight overrun of the delay we want. Try halving
		 * our delay to see if that will cut the number of packets
		 * in half per interrupt.
		 */
		itr_usec >>= 2;
		itr_usec &= DN200_ITR_MASK;
		if (itr_usec < priv->min_usecs)
			itr_usec = priv->min_usecs;
		goto clear_counts;
	} else if (packets >= 2 && bytes < 9000) {
		itr_usec = DN200_ITR_ADAPTIVE_LATENCY;
		goto adjust_by_size;
	} else if (packets >= 2 && packets < 32) {
		itr_usec = (itr->target_itr << 1);
		if ((itr_usec & DN200_ITR_MASK) > max_usec)
			itr_usec = max_usec;
	} else if (packets >= 32 && packets < 56) {
		itr_usec = (itr->target_itr + DN200_ITR_MIN_INC);
		if ((itr_usec & DN200_ITR_MASK) > max_usec)
			itr_usec = max_usec;

		goto clear_counts;
	} else if (packets <= 256) {
		itr_usec = itr->target_itr;
		itr_usec &= DN200_ITR_MASK;

		/* Between 56 and 112 is our "goldilocks" zone where we are
		 * working out "just right". Just report that our current
		 * ITR is good for us.
		 */
		if (packets <= 112)
			goto clear_counts;

		/* If packet count is 128 or greater we are likely looking
		 * at a slight overrun of the delay we want. Try halving
		 * our delay to see if that will cut the number of packets
		 * in half per interrupt.
		 */
		itr_usec -= priv->min_usecs;
		if (itr_usec < priv->min_usecs)
			itr_usec = priv->min_usecs;
		goto clear_counts;
	}

adjust_by_size:
	/* If packet counts are 256 or greater we can assume we have a gross
	 * overestimation of what the rate should be. Instead of trying to fine
	 * tune it just use the formula below to try and dial in an exact value
	 * give the current packet size of the frame.
	 */
	avg_wire_size = bytes / packets;
	if (avg_wire_size <= 60) {
		/* Start at 250k ints/sec */
		avg_wire_size = 4096;
	} else if (avg_wire_size <= 380) {
		/* 250K ints/sec to 60K ints/sec */
		avg_wire_size *= 40;
		avg_wire_size += 1696;
	} else if (avg_wire_size <= 1084) {
		/* 60K ints/sec to 36K ints/sec */
		avg_wire_size *= 15;
		avg_wire_size += 11452;
	} else if (avg_wire_size <= 1980) {
		/* 36K ints/sec to 30K ints/sec */
		avg_wire_size *= 5;
		avg_wire_size += 22420;
	} else {
		/* plateau at a limit of 30K ints/sec */
		avg_wire_size = 32256;
	}

	/* If we are in low latency mode halve our delay which doubles the
	 * rate to somewhere between 100K to 16K ints/sec
	 */
	if (itr_usec & DN200_ITR_ADAPTIVE_LATENCY)
		avg_wire_size >>= 1;

	/* Resultant value is 256 times larger than it needs to be. This
	 * gives us room to adjust the value as needed to either increase
	 * or decrease the value based on link speeds of 10G, 2.5G, 1G, etc.
	 *
	 * Use addition as we have already recorded the new latency flag
	 * for the ITR value.
	 */
	itr_usec += DIV_ROUND_UP(avg_wire_size, itr->itr_div) * 2;
	if ((itr_usec & DN200_ITR_MASK) > max_usec) {
		itr_usec &= DN200_ITR_ADAPTIVE_LATENCY;
		itr_usec += max_usec;
	}

clear_counts:
	/* write back value */
	itr->target_itr = (itr_usec & DN200_ITR_MASK);
	if (unlikely(itr->target_itr < priv->min_usecs)) {
		netdev_err(priv->dev, "invalid target itr usec:%u, rx itr usec:%u\n",
			itr->target_itr, priv->rx_itr_usec);
		itr->target_itr = priv->min_usecs;
	}

	/* next update should occur within next jiffy */
	itr->next_update = next_update + msecs_to_jiffies(1);
	itr->bytes = 0;
	itr->packet = 0;
}

static void dn200_tx_itr_update(struct dn200_priv *priv,
				struct dn200_itr_info *itr, u8 chan)
{
	u64 avg_wire_size = 0, packets = 0, bytes = 0, tx_frames = 0;
	u32 max_tx_frame = 0;
	unsigned long next_update = jiffies;

	/* These will do nothing if dynamic updates are not enabled */
	if (!(itr->itr_setting & DN200_ITR_DYNAMIC_ITR))
		return;

	if (time_after(next_update, itr->next_update)) {
		tx_frames = 1;
		goto clear_counts;
	}

	if (itr->itr_countdown) {
		tx_frames = itr->target_itr;
		goto clear_counts;
	}

	packets = itr->packet;
	bytes = itr->bytes;

	if (!packets || !bytes) {
		tx_frames = itr->target_itr;
		goto clear_counts;
	}

	avg_wire_size = bytes / packets;
	if (!avg_wire_size) {
		tx_frames = itr->target_itr;
		goto clear_counts;
	}
	if (avg_wire_size > 4096)
		max_tx_frame = 8;
	else if (avg_wire_size > 2048)
		max_tx_frame = 16;
	else if (avg_wire_size > 1024)
		max_tx_frame = 32;
	else if (avg_wire_size < 256)
		max_tx_frame = 128;
	else
		max_tx_frame = 64;

	if (priv->plat_ex->phy_info->speed == SPEED_1000)
		max_tx_frame = max_t(u32, max_tx_frame >> 2, 1);

	max_tx_frame = min((u32)(priv->dma_tx_size >> 1), (u32)max_tx_frame);
	tx_frames = max_tx_frame;

clear_counts:
	/* write back value */
	if (tx_frames)
		itr->target_itr = (tx_frames & DN200_ITR_MASK);
	/* next update should occur within next jiffy */
	itr->next_update = next_update + msecs_to_jiffies(1);

	itr->bytes = 0;
	itr->packet = 0;
	priv->tx_mem_copy = 0;
}

static int dn200_napi_poll_rx(struct napi_struct *napi, int budget)
{
	struct dn200_channel *ch =
	    container_of(napi, struct dn200_channel, rx_napi);
	struct dn200_priv *priv = ch->priv_data;
	u32 chan = ch->index;
	int work_done;
	struct dn200_itr_info *rx_intr;

	rx_intr = &priv->rx_intr[chan];

	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
		 test_bit(DN200_PCIE_UNAVAILD, &priv->state))) {
		napi_complete(napi);
		return 0;
	}
	priv->xstats.napi_poll++;
	work_done = dn200_rx(priv, budget, chan, priv->dma_rx_size);
	if (work_done < budget && napi_complete_done(napi, work_done)) {
		priv->dn200_update_ops->dn200_rx_itr_update(rx_intr, priv, chan);
		if (rx_intr->current_itr != rx_intr->target_itr) {
			/* Rx ITR needs to be increased, second priority */
			rx_intr->current_itr = rx_intr->target_itr;
			rx_intr->itr_countdown = ITR_COUNTDOWN_COUNT;
		} else {
			rx_intr->current_itr = rx_intr->target_itr;
			/* No ITR update, lowest priority */
			if (rx_intr->itr_countdown)
				rx_intr->itr_countdown--;
		}
		dn200_rx_watchdog(priv, priv->ioaddr,
				dn200_usec2riwt(rx_intr->target_itr, priv), chan,
				priv->hw);
	}
	/* In the condition of tx and rx share same irq(when use single msi),
	 * will lead to interrupt lost when
	 * process hardware interrupt by same cpu in dn200_interrupt,
	 * the phenomenon is hw rx queue curr == tail index
	 * (means hw consume all descs and sw don't refill)
	 * however hw interrupt(tx & rx) status is kept in register,
	 * so we call dn200_dma_interrupt again to process all queues' tx & rx interrupt
	 */
	if (!priv->plat->multi_msi_en)
		dn200_dma_interrupt(priv);
	return work_done;
}

static void tx_frame_count(struct dn200_priv *priv, u32 target_itr)
{
	if (target_itr < 17)
		priv->xstats.tx_frames_16_below++;
	else if (target_itr < 33)
		priv->xstats.tx_frames_17_to_32++;
	else if (target_itr < 65)
		priv->xstats.tx_frames_33_to_64++;
	else if (target_itr < 129)
		priv->xstats.tx_frames_65_to_128++;
	else
		priv->xstats.tx_frames_129_to_256++;
}

static int dn200_napi_poll_tx(struct napi_struct *napi, int budget)
{
	struct dn200_channel *ch =
	    container_of(napi, struct dn200_channel, tx_napi);
	struct dn200_priv *priv = ch->priv_data;
	struct dn200_itr_info *tx_intr;
	u32 chan = ch->index;
	int work_done;

	priv->xstats.napi_poll++;
	tx_intr = &priv->tx_intr[chan];

	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
		 test_bit(DN200_PCIE_UNAVAILD, &priv->state))) {
		napi_complete(napi);
		return 0;
	}

	work_done = dn200_tx_clean(priv, budget, chan);
	work_done = min(work_done, budget);

	if (work_done < budget && napi_complete_done(napi, work_done)) {
		if (tx_intr->packet) {
			dn200_tx_itr_update(priv, tx_intr, chan);
			tx_frame_count(priv, tx_intr->target_itr);
			if (tx_intr->target_itr < tx_intr->current_itr) {
				/* tx ITR needs to be reduced, this is highest priority */
				tx_intr->current_itr = tx_intr->target_itr;
				tx_intr->itr_countdown = ITR_COUNTDOWN_COUNT;
				priv->tx_coal_frames[chan] =
				    tx_intr->target_itr;
			} else if (tx_intr->current_itr != tx_intr->target_itr) {
				/* tx ITR needs to be increased, second priority */
				tx_intr->current_itr = tx_intr->target_itr;
				tx_intr->itr_countdown = ITR_COUNTDOWN_COUNT;
				priv->tx_coal_frames[chan] =
				    tx_intr->target_itr;
			} else {
				tx_intr->current_itr = tx_intr->target_itr;
				/* No ITR update, lowest priority */
				if (tx_intr->itr_countdown)
					tx_intr->itr_countdown--;
			}
			if (!(tx_intr->itr_setting & DN200_ITR_DYNAMIC_ITR))
				priv->tx_coal_frames[chan] =
				    priv->tx_coal_frames_set[chan] ? : 1;
		}
		dn200_enable_tx_dma_irq(priv->ioaddr, ch->index, priv->hw);
	}

	/* In the condition of tx and rx share same irq(when use single msi),
	 * will lead to interrupt lost when process hardware interrupt by
	 * same cpu in dn200_interrupt,
	 * the phenomenon is hw rx queue curr == tail index
	 * (means hw consume all descs and sw don't refill)
	 * however hw interrupt(tx & rx) status is kept in register,
	 * so we call dn200_dma_interrupt again to
	 * process all queues' tx & rx interrupt
	 */
	if (!priv->plat->multi_msi_en)
		dn200_dma_interrupt(priv);

	return work_done;
}

static int dn200_napi_poll_agg(struct napi_struct *napi, int budget)
{
	struct dn200_channel *ch =
	    container_of(napi, struct dn200_channel, agg_napi);
	struct dn200_priv *priv = ch->priv_data;
	u32 chan = ch->index;
	struct dn200_rx_queue *rx_q = &priv->rx_queue[chan];
	int work_done;
	int rx_rcv;
	bool complete_cleaned = true;
	struct dn200_itr_info *rx_intr;
	struct dn200_itr_info *tx_intr;

	rx_intr = &priv->rx_intr[chan];
	tx_intr = &priv->tx_intr[chan];
	priv->xstats.napi_poll++;

	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))) {
		napi_complete(napi);
		return 0;
	}

	rx_rcv = dn200_rx(priv, budget, chan, priv->dma_rx_size);
	if (rx_rcv >= budget)
		complete_cleaned = false;

	work_done = dn200_tx_clean(priv, budget, chan);
	if (work_done >= budget)
		complete_cleaned = false;

	if (!complete_cleaned)
		return budget;

	if (tx_intr->packet) {
		dn200_tx_itr_update(priv, tx_intr, chan);
		tx_frame_count(priv, tx_intr->target_itr);
		if (tx_intr->current_itr != tx_intr->target_itr) {
			tx_intr->current_itr = tx_intr->target_itr;
			tx_intr->itr_countdown = ITR_COUNTDOWN_COUNT;
		} else {
			tx_intr->current_itr = tx_intr->target_itr;
			/* No ITR update, lowest priority */
			if (tx_intr->itr_countdown)
				tx_intr->itr_countdown--;
		}
	}

	if (rx_intr->packet) {
		priv->dn200_update_ops->dn200_rx_itr_update(rx_intr, priv, chan);
		if (rx_intr->current_itr != rx_intr->target_itr) {
			/* Rx ITR needs to be increased, second priority */
			rx_intr->current_itr = rx_intr->target_itr;
			rx_intr->itr_countdown = ITR_COUNTDOWN_COUNT;
		} else {
			rx_intr->current_itr = rx_intr->target_itr;
			/* No ITR update, lowest priority */
			if (rx_intr->itr_countdown)
				rx_intr->itr_countdown--;
		}
	}

	if (!((rx_q->dma_rx + rx_q->cur_rx)->des3 & XGMAC_RDES3_OWN))
		return budget;

	if (napi_complete_done(napi, rx_rcv)) {
		/* enable rx interrupt through set riwt */
		dn200_rx_watchdog(priv, priv->ioaddr,
					dn200_usec2riwt(rx_intr->target_itr, priv), chan,
					priv->hw);
		priv->tx_coal_frames[chan] = tx_intr->current_itr;
		dn200_enable_tx_dma_irq(priv->ioaddr, ch->index, priv->hw);
	}

	return rx_rcv;
}

/**
 *  dn200_tx_timeout
 *  @dev : Pointer to net device structure
 *  @txqueue: the index of the hanging transmit queue
 *  Description: this function is called when a packet transmission fails to
 *   complete within a reasonable time. The driver will mark the error in the
 *   netdev structure and arrange for the device to be reset to a sane state
 *   in order to transmit a new packet.
 */
static void dn200_tx_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue, hw_chan;
	u32 cache_lvl;
	u32 ch_tail, ch_curr, tx_avail;
	u32 dbg_status, dbg_status0, dbg_status1;

	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(dev, "%s :%s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return;
	}

	netdev_info(dev, "=== Tx data path debug info ===\n");

	for (queue = 0; queue < tx_count; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
		u32 tobe_consume = 0;

		hw_chan = queue + DN200_RXQ_START_GET(priv->hw);

		/* get sw & hw ring status, include sw available descs number,
		 * hw to be consume descs number
		 */
		ch_tail = readl(priv->ioaddr +
				XGMAC_DMA_CH_TxDESC_TAIL_LPTR(hw_chan));
		ch_curr = readl(priv->ioaddr +
				XGMAC_DMA_CH_TxDESC_CURR_LPTR(hw_chan));
		tx_avail = dn200_tx_avail(priv, queue);
		netdev_info(dev, "TX queue %d time_running %d task_status %d time_status %d\n",
				queue, atomic_read(&tx_q->txtimer_running),
				tx_q->task_need_sch, tx_q->txtimer_need_sch);
		netdev_info(dev, "TX Queue %d channel %d %s\n", queue, hw_chan,
			   ((tx_avail < priv->dma_tx_size - 1) ||
			    (ch_tail != ch_curr)) ?
				   "- running" :
				   "");

		if (ch_tail != ch_curr)
			tobe_consume = (ch_curr > ch_tail) ?
					       (priv->dma_tx_size -
						((ch_curr - ch_tail) / 16)) :
					       ((ch_tail - ch_curr) / 16 + 1);

		netdev_info(dev,
			   "Desc sw ring curr_tx:%d, dirty_tx:%d, tx available:%d, result:%s\n",
			   tx_q->cur_tx, tx_q->dirty_tx, tx_avail,
			   tx_avail >
			   DN200_TX_THRESH(priv) ? "yes (good)" :
			   "no (abnormal)");

		netdev_info(dev,
			   "Desc hw ring start:%#x, tail:%#x, curr:%#x, to consume:%u, result:%s\n",
			   readl(priv->ioaddr +
				 XGMAC_DMA_CH_TxDESC_LADDR(hw_chan)), ch_tail,
			   ch_curr, tobe_consume,
			   tobe_consume >
			   (priv->dma_tx_size /
			    2) ? "no (abnormal)" : "yes (good)");

		/*2. get descs cache level */
		cache_lvl =
		    readl(priv->ioaddr + XGMAC_CH_DESC_CACHE_LVL(hw_chan));
		netdev_info(dev,
			   "[reg 3168]Desc tx cache levle is:%lu, cache_lvl:%#x\n",
			   (cache_lvl & XGMAC_TXLVL), cache_lvl);

		/*3. get dma channel debug status */
		dbg_status = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(hw_chan));
		netdev_info(dev,
			   "[reg 3164]DMA channel TDWS-des write state:%#lx, TDTS-data transfer state:%#lx, TDFS-des fetch state:%#lx, TDRS:%#lx, TDXS:%#lx, dbg_status:%#x\n",
			   (dbg_status & XGMAC_TDWS) >> XGMAC_TDWS_SHIFT,
			   (dbg_status & XGMAC_TDTS) >> XGMAC_TDTS_SHIFT,
			   (dbg_status & XGMAC_TDFS) >> XGMAC_TDFS_SHIFT,
			   (dbg_status & XGMAC_TDRS) >> XGMAC_TDRS_SHIFT,
			   (dbg_status & XGMAC_TDXS) >> XGMAC_TDXS_SHIFT,
			   dbg_status);

		/*4. get tx dma FSM debug status1(dma_debug_status1) */
		dbg_status1 = readl(priv->ioaddr + XGMAC_DEBUG_ST1);
		netdev_info(dev,
			   "[reg 3024]Chanel %d DMA FSMs are%s actively processing the descriptors or packet data, dma_debug_status1:%#x.\n",
			   hw_chan,
			   (dbg_status1 & (1 << hw_chan)) ? "" : " Not",
			   dbg_status1);

		/*5. get tx dma debug status0(dma_debug_status0) */
		dbg_status0 = readl(priv->ioaddr + XGMAC_DEBUG_ST0);
		netdev_info(dev,
			   "[reg 3020]AXI Master Read Channel is%s active(tx global status), dma_debug_status0:%#x.\n",
			   (dbg_status0 & XGMAC_AXRHSTS) ? "" : " Not",
			   dbg_status0);
	}

	dn200_global_err(priv, DN200_TX_TIMEOUT);
}

static void dn200_fdirs_reconfig(struct dn200_priv *priv, bool enable)
{
	struct dn200_fdir_filter *input;
	int i = priv->flow_entries_max - 5;

	if (HW_IS_VF(priv->hw))
		return;
	/* report total rule count */
	for (; i >= 0; i--) {
		input = &priv->fdir_enties[i];
		if (input->enable) {
			dn200_config_ntuple_filter(priv, priv->hw, i, input,
						   enable);
		}
	}
}

static void _dn200_set_filter(struct dn200_priv *priv, struct net_device *dev)
{
	u8 wakeup_wq = false;

	dn200_set_filter(priv, priv->hw, dev, &wakeup_wq);
	priv->pf_rxp_set |= RXP_SET_FIL;
	if (wakeup_wq && PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
		queue_work(priv->wq, &priv->rxp_task);
	else if (PRIV_SRIOV_SUPPORT(priv))
		set_bit(DN200_RXP_NEED_CHECK, &priv->state);

	if (wakeup_wq && PRIV_IS_VF(priv)) {
		netdev_dbg(priv->dev, "%s %d notify pf set filter\n", __func__, __LINE__);
		DN200_ITR_SYNC_SET(priv->hw, itr_sync_app, RXP_TASK, 1);
		irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
	}
}
/**
 *  dn200_set_rx_mode - entry point for multicast addressing
 *  @dev : pointer to the device structure
 *  Description:
 *  This function is a driver entry point which gets called by the kernel
 *  whenever multicast addresses must be enabled/disabled.
 *  Return value:
 *  void.
 */
static void dn200_set_rx_mode(struct net_device *dev)
{
	u32 chan;

	struct dn200_priv *priv = netdev_priv(dev);
	netdev_features_t features = dev->features;

	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(dev, "%s :%s\n", __func__, DN200_PCIE_BAR_ERR);
		return;
	}

	if (!test_bit(DN200_DEV_INIT, &priv->state)) {
		_dn200_set_filter(priv, dev);
		if (!!(features & NETIF_F_HW_VLAN_CTAG_FILTER) &&
				 !(dev->flags & IFF_PROMISC))
			_dn200_config_vlan_rx_fltr(priv, priv->hw, true);
		else
			_dn200_config_vlan_rx_fltr(priv, priv->hw, false);
	}
	if (!!(features & NETIF_F_HW_VLAN_CTAG_RX))
		dn200_rx_vlan_stripping_config(priv, priv->hw, true);
	else
		dn200_rx_vlan_stripping_config(priv, priv->hw, false);

	if (HW_IS_PUREPF(priv->hw)) {
		if (dev->features & NETIF_F_RXALL)
			priv->rec_all = true;
		else
			priv->rec_all = false;
		for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
			dn200_dma_rx_all_set(priv, priv->ioaddr, chan,
					     priv->rec_all, priv->hw);
		}

		if (!PRIV_IS_VF(priv)) {
			dn200_dma_rx_all_set(priv, priv->ioaddr,
					     DN200_LAST_QUEUE(priv),
					     priv->rec_all, priv->hw);
		}
	}
	if (dev->features & NETIF_F_NTUPLE) {
		dn200_l3_l4_filter_config(priv, priv->hw,
					!!(dev->features & NETIF_F_NTUPLE));
		dn200_fdirs_reconfig(priv, !!(dev->features & NETIF_F_NTUPLE));
	}
}

/**
 *  dn200_change_mtu - entry point to change MTU size for the device.
 *  @dev : device pointer.
 *  @new_mtu : the new MTU size for the device.
 *  Description: the Maximum Transfer Unit (MTU) is used by the network layer
 *  to drive packet transmission. Ethernet has an MTU of 1500 octets
 *  (ETH_DATA_LEN). This value can be changed with ifconfig.
 *  Return value:
 *  0 on success and an appropriate (-)ve integer as defined in errno.h
 *  file on failure.
 */
static int dn200_change_mtu(struct net_device *dev, int new_mtu)
{
	struct dn200_priv *priv = netdev_priv(dev);
	const int mtu = new_mtu;
	int min_mtu = 0, ret = 0;

	if (new_mtu == (int)dev->mtu) {
		netdev_warn(dev, "MTU is already %u\n", dev->mtu);
		return 0;
	}

	if (dn200_xdp_is_enabled(priv) && new_mtu > ETH_DATA_LEN) {
		netdev_dbg(priv->dev, "Jumbo frames not supported for XDP\n");
		return -EINVAL;
	}
	ret = dn200_max_mtu_get(priv, &priv->plat->maxmtu, &min_mtu);
	if (ret < 0) {
		dev_err(priv->device, "max_mtu alloc err!\n");
		return -EINVAL;
	}

	/* If condition true, FIFO is too small or MTU too large */
	if (new_mtu > priv->plat->maxmtu)
		return -EINVAL;

	if (new_mtu < min_mtu)
		return -EINVAL;

	netdev_info(priv->dev, "changing MTU from %d to %d\n", dev->mtu, mtu);
	dev->mtu = mtu;
	if (!priv->mii)
		netdev_update_features(dev);
	/* base on new mtu to update rx interrupt usec */
	dn200_rx_itr_usec_update(priv);
	usleep_range(10000, 15000);
	return 0;
}

static netdev_features_t dn200_fix_features(struct net_device *dev,
					    netdev_features_t features)
{
	struct dn200_priv *priv = netdev_priv(dev);

	if (priv->plat->rx_coe == DN200_RX_COE_NONE)
		features &= ~NETIF_F_RXCSUM;

	if (!priv->plat->tx_coe)
		features &= ~NETIF_F_CSUM_MASK;

	/* Some GMAC devices have a bugged Jumbo frame support that
	 * needs to have the Tx COE disabled for oversized frames
	 * (due to limited buffer sizes). In this case we disable
	 * the TX csum insertion in the TDES and not use SF.
	 */
	if (priv->plat->bugged_jumbo && dev->mtu > ETH_DATA_LEN)
		features &= ~NETIF_F_CSUM_MASK;

	/* Disable tso if asked by ethtool */
	if (priv->plat->tso_en && priv->dma_cap.tsoen) {
		if (features & NETIF_F_TSO)
			priv->tso = true;
		else
			priv->tso = false;
	}

	return features;
}

static int dn200_set_features(struct net_device *netdev,
			      netdev_features_t features)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	netdev_features_t changed = netdev->features ^ features;

	/* Keep the COE Type in case of csum is supporting */
	if (features & NETIF_F_RXCSUM)
		priv->hw->rx_csum = priv->plat->rx_coe;
	else
		priv->hw->rx_csum = 0;
	/* No check needed because rx_coe has been set before and it will be
	 * fixed in case of issue.
	 */
	dn200_rx_ipc(priv, priv->hw);

	if (priv->sph_cap) {
		bool sph_en = (priv->hw->rx_csum > 0) && priv->sph;
		u32 chan;

		for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++)
			dn200_enable_sph(priv, priv->ioaddr, sph_en, chan,
					 priv->hw);
	}
	netdev->features = features;
	if (changed & (NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_CTAG_FILTER))
		dn200_set_rx_mode(netdev);

	if (changed & NETIF_F_LOOPBACK)
		dn200_set_mac_loopback(priv, priv->ioaddr,
				       !!(features & NETIF_F_LOOPBACK));

	if (changed & NETIF_F_RXHASH) {
		if (features & NETIF_F_RXHASH)
			priv->rss.enable = true;
		else
			priv->rss.enable = false;

		dn200_rss_configure(priv, priv->hw, &priv->rss,
				    priv->plat->rx_queues_to_use);
	}
	if (changed & NETIF_F_NTUPLE) {
		dn200_l3_l4_filter_config(priv, priv->hw,
					  !!(netdev->features & NETIF_F_NTUPLE));
		dn200_fdirs_reconfig(priv,
				     !!(netdev->features & NETIF_F_NTUPLE));
	}
	return 0;
}

static void dn200_common_interrupt(struct dn200_priv *priv)
{
	u32 rx_cnt = priv->plat->rx_queues_to_use;
	u32 tx_cnt = priv->plat->tx_queues_to_use;
	u32 queues_count;
	u32 queue;
	bool xmac;

	xmac = priv->plat->has_gmac4 || priv->plat->has_xgmac;
	queues_count = (rx_cnt > tx_cnt) ? rx_cnt : tx_cnt;

	if (priv->dma_cap.estsel)
		dn200_est_irq_status(priv, priv->ioaddr, priv->dev,
				     &priv->xstats, tx_cnt);
	/* To handle GMAC own interrupts */
	if (priv->plat->has_gmac || xmac) {
		int status =
		    dn200_host_irq_status(priv, priv->hw, &priv->xstats);

		if (unlikely(status)) {
			/* For LPI we need to save the tx status */
			if (status & CORE_IRQ_TX_PATH_IN_LPI_MODE)
				priv->tx_path_in_lpi_mode = true;
			if (status & CORE_IRQ_TX_PATH_EXIT_LPI_MODE)
				priv->tx_path_in_lpi_mode = false;
		}

		for (queue = 0; queue < queues_count; queue++)
			status = dn200_host_mtl_irq_status(priv, priv->hw,
							   queue);

		dn200_timestamp_interrupt(priv, priv);
	}
}

/**
 *  dn200_interrupt - main ISR
 *  @irq: interrupt number.
 *  @dev_id: to pass the net device pointer.
 *  Description: this is the main driver interrupt service routine.
 *  It can call:
 *  o DMA service routine (to manage incoming frame reception and transmission
 *    status)
 *  o Core interrupts to manage: remote wake-up, management counter, LPI
 *    interrupts.
 */
static irqreturn_t dn200_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct dn200_priv *priv = netdev_priv(dev);

	/* Check if adapter is up */
	if (unlikely(test_bit(DN200_DOWN, &priv->state)))
		return IRQ_HANDLED;

	/* Check if a fatal error happened */
	if (dn200_safety_feat_interrupt(priv))
		return IRQ_HANDLED;

	/* To handle Common interrupts */
	dn200_common_interrupt(priv);

	/* To handle DMA interrupts */
	dn200_dma_interrupt(priv);

	return IRQ_HANDLED;
}

static irqreturn_t dn200_mac_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct dn200_priv *priv = netdev_priv(dev);

	if (unlikely(!dev)) {
		netdev_err(priv->dev, "%s: invalid dev pointer\n", __func__);
		return IRQ_NONE;
	}

	/* Check if adapter is up */
	if (test_bit(DN200_DOWN, &priv->state))
		return IRQ_HANDLED;

	/* To handle Common interrupts */
	dn200_common_interrupt(priv);

	return IRQ_HANDLED;
}

static irqreturn_t dn200_safety_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct dn200_priv *priv = netdev_priv(dev);

	if (unlikely(!dev)) {
		netdev_err(priv->dev, "%s: invalid dev pointer\n", __func__);
		return IRQ_NONE;
	}

	/* Check if adapter is up */
	if (test_bit(DN200_DOWN, &priv->state))
		return IRQ_HANDLED;

	/* Check if a fatal error happened */
	dn200_safety_feat_interrupt(priv);

	return IRQ_HANDLED;
}

static irqreturn_t dn200_msi_intr_tx(int irq, void *data)
{
	struct dn200_tx_queue *tx_q = (struct dn200_tx_queue *)data;
	int chan = tx_q->queue_index;
	struct dn200_priv *priv;
	int status;

	priv = container_of(tx_q, struct dn200_priv, tx_queue[chan]);
	priv->xstats.tx_normal_irq_n++;

	if (unlikely(!data)) {
		netdev_err(priv->dev, "%s: invalid dev pointer\n", __func__);
		return IRQ_NONE;
	}
	/* Check if adapter is up */
	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state)))
		return IRQ_HANDLED;
	if (unlikely(!dn200_dp_hwif_id_check(priv->ioaddr))) {
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		return IRQ_HANDLED;
	}

	status = dn200_napi_check(priv, chan, DMA_DIR_TX);

	if (unlikely(status == tx_hard_error))
		dn200_tx_err(priv, chan);

	return IRQ_HANDLED;
}

static irqreturn_t dn200_msi_intr_rx(int irq, void *data)
{
	struct dn200_rx_queue *rx_q = (struct dn200_rx_queue *)data;
	int chan = rx_q->queue_index;
	struct dn200_priv *priv;

	priv = container_of(rx_q, struct dn200_priv, rx_queue[chan]);
	priv->xstats.rx_normal_irq_n++;

	if (unlikely(!data)) {
		netdev_err(priv->dev, "%s: invalid dev pointer\n", __func__);
		return IRQ_NONE;
	}
	/* Check if adapter is up */
	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state)))
		return IRQ_HANDLED;
	if (unlikely(!dn200_dp_hwif_id_check(priv->ioaddr))) {
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		return IRQ_HANDLED;
	}
	dn200_napi_check(priv, chan, DMA_DIR_RX);

	return IRQ_HANDLED;
}

static irqreturn_t dn200_msi_intr_rxtx(int irq, void *data)
{
	struct dn200_channel *ch = (struct dn200_channel *)data;
	struct dn200_priv *priv;
	struct napi_struct *agg_napi;
	struct dn200_itr_info *rx_intr;
	struct dn200_itr_info *tx_intr;

	if (unlikely(!data))
		return IRQ_NONE;

	priv = ch->priv_data;
	rx_intr = &priv->rx_intr[ch->index];
	tx_intr = &priv->tx_intr[ch->index];

	if (irq == priv->tx_irq[ch->index])
		priv->xstats.tx_normal_irq_n++;
	if (irq == priv->rx_irq[ch->index])
		priv->xstats.rx_normal_irq_n++;

	/* Check if adapter is up */
	if (unlikely(test_bit(DN200_DOWN, &priv->state) ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state)))
		return IRQ_HANDLED;
	if (!dn200_dp_hwif_id_check(priv->ioaddr)) {
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		return IRQ_HANDLED;
	}
	agg_napi = &ch->agg_napi;
	if (napi_schedule_prep(agg_napi)) {
		if (rx_intr->itr_setting & DN200_ITR_DYNAMIC_ITR &&
			rx_intr->current_itr > DN200_ITR_RWT_BOUND)
			dn200_rx_watchdog(priv, priv->ioaddr,
					 DN200_ITR_MAX_RWT, ch->index, priv->hw);

		dn200_disable_tx_dma_irq(priv->ioaddr, ch->index, priv->hw);
		__napi_schedule(agg_napi);
	}

	return IRQ_HANDLED;
}

/* Polling receive - used by NETCONSOLE and other diagnostic tools
 * to allow network I/O with interrupts disabled.
 */
static void dn200_poll_controller(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int i;

	/* If adapter is down, do nothing */
	if (test_bit(DN200_DOWN, &priv->state))
		return;

	if (priv->plat->multi_msi_en) {
		for (i = 0; i < priv->plat->rx_queues_to_use; i++)
			dn200_msi_intr_rx(0, &priv->rx_queue[i]);

		for (i = 0; i < priv->plat->tx_queues_to_use; i++)
			dn200_msi_intr_tx(0, &priv->tx_queue[i]);
	} else {
		disable_irq(dev->irq);
		dn200_interrupt(dev->irq, dev);
		enable_irq(dev->irq);
	}
}

static int dn200_mii_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct dn200_priv *priv = netdev_priv(dev);
	struct mii_ioctl_data *data = if_mii(rq);
	int ret;

	if (!priv->mii)
		return -EOPNOTSUPP;

	switch (cmd) {
	case SIOCGMIIPHY:
		data->phy_id = priv->plat->phy_addr;
		break;
	case SIOCGMIIREG:
		ret = priv->mii->read(priv->mii, priv->plat->phy_addr,
			data->reg_num & 0x1F);
		if (ret < 0)
			return ret;
		data->val_out = ret;
		break;
	case SIOCSMIIREG:
	default:
		return -EOPNOTSUPP;
	}
	return 0;
}

/**
 *  dn200_ioctl - Entry point for the Ioctl
 *  @dev: Device pointer.
 *  @rq: An IOCTL specefic structure, that can contain a pointer to
 *  a proprietary structure used to pass information to the driver.
 *  @cmd: IOCTL command
 *  Description:
 *  Currently it supports the phy_mii_ioctl(...) and HW time stamping.
 */
static int dn200_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	int ret = -EOPNOTSUPP;

	if (!netif_running(dev))
		return -EINVAL;

	switch (cmd) {
	case SIOCGMIIPHY:
	case SIOCGMIIREG:
	case SIOCSMIIREG:
		return dn200_mii_ioctl(dev, rq, cmd);
	case SIOCSHWTSTAMP:
		ret = dn200_hwtstamp_set(dev, rq);
		break;
	case SIOCGHWTSTAMP:
		ret = dn200_hwtstamp_get(dev, rq);
		break;
	default:
		break;
	}

	return ret;
}

static LIST_HEAD(dn200_block_cb_list);
static u16 dn200_skb_tx_hash(struct net_device *dev,
			     const struct sk_buff *skb, u16 num_tx_queues)
{
	u32 jhash_initval_salt = 0xd631614b;
	u32 hash;

	if (skb->sk && skb->sk->sk_hash)
		hash = skb->sk->sk_hash;
	else
		hash = (__force u16)skb->protocol ^ skb->hash;

	hash = jhash_1word(hash, jhash_initval_salt);

	return (u16)(((u64)hash * num_tx_queues) >> 32);
}


static u16 dn200_select_queue(struct net_device *dev, struct sk_buff *skb,
			      struct net_device *sb_dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int gso = skb_shinfo(skb)->gso_type;
	int dscp = 0;
	/* for MTU bigger than 1500 and packet length bigger than 1518,
	 * always use queue 0 to xmit, as just queue 0 support jumbo
	 */
	if (dev->mtu > 1500 && skb->len > 1518)
		return 0;
	if (!netdev_get_num_tc(dev) || !priv->ets)
		goto kernel_pick;

	/* DSCP mode or PCP mode? */
	if (priv->dscp_app_cnt) {
		if (skb->protocol == htons(ETH_P_IP))
			dscp = ipv4_get_dsfield(ip_hdr(skb)) >> 2;
		else if (skb->protocol == htons(ETH_P_IPV6))
			dscp = ipv6_get_dsfield(ipv6_hdr(skb)) >> 2;

		if (dscp < 64)
			return priv->ets->prio_tc[priv->dscp2up[dscp]];
	} else if (skb_vlan_tag_present(skb)) {
		return priv->ets->prio_tc[skb_vlan_tag_get_prio(skb)];
	}

kernel_pick:
	if (gso & (SKB_GSO_TCPV4 | SKB_GSO_TCPV6))
		return 0;

	if ((skb->sk && skb->sk->sk_hash) || skb->hash)
		return dn200_skb_tx_hash(dev, skb,
				 dev->real_num_tx_queues) %
	    dev->real_num_tx_queues;
	else
		return netdev_pick_tx(dev, skb, sb_dev) % dev->real_num_tx_queues;
}

static int dn200_set_mac_address(struct net_device *ndev, void *addr)
{
	struct dn200_priv *priv = netdev_priv(ndev);
	struct sockaddr *paddr = addr;
	int ret = 0;

	if (!is_valid_ether_addr(paddr->sa_data))
		return -EADDRNOTAVAIL;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	eth_hw_addr_set(ndev, paddr->sa_data);
	/* netdev set mac before call dev open, down or opening state don't allow to set rpx */
	if (netif_running(ndev) &&
		!test_bit(DN200_DOWN, &priv->state) &&
		!test_bit(DN200_DEV_INIT, &priv->state)) {
		ret = _dn200_set_umac_addr(priv, (unsigned char *)ndev->dev_addr, 0);
	}

	/* when ret bigger than 0, means set umac filter success,
	 * but need to return 0 to dev layer, otherwise dev layer will
	 * not call notifier chain to flush dev arp entries and cause stop flow a moment
	 */
	if (ret > 0)
		ret = 0;

	return ret;
}

static bool dn200_netdev_is_memb_of_bond(struct net_device *netdev)
{
	struct net_device *upper_dev = NULL;

	upper_dev = netdev_master_upper_dev_get(netdev);
	if (upper_dev && netif_is_bond_master(upper_dev))
		return true;

	return false;
}

int dn200_dev_event(struct notifier_block *unused,
		    unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct dn200_priv *priv = netdev_priv(dev);

	if (dev->netdev_ops != &dn200_netdev_ops &&
			dev->netdev_ops != &dn200_vf_netdev_ops)
		goto done;

	switch (event) {
	case NETDEV_CHANGENAME:
		if (priv->dbgfs_dir)
			priv->dbgfs_dir =
			    debugfs_rename(priv->dbgfs_dir->d_parent,
					   priv->dbgfs_dir,
					   priv->dbgfs_dir->d_parent,
					   dev->name);
		break;
	case NETDEV_CHANGEUPPER:
		if (dev == priv->dev) {
			/* add slave netdev(bonding member) */
			if (dn200_netdev_is_memb_of_bond(dev))
				set_bit(DN200_IS_BONDING, &priv->state);
			else
				/* remove slave netdev(bonding member) */
				clear_bit(DN200_IS_BONDING, &priv->state);
		}

	default:
		break;
	}
done:
	return NOTIFY_DONE;
}

/* Use network device events to rename debugfs file entries.
 */
#define DN200_DEFINE_SHOW_ATTRIBUTE(__name)		\
	static int __name##_open(struct inode *inode, struct file *file)	\
	{																	\
		return single_open(file, __name##_show, inode->i_private);		\
	}																	\
																		\
	static const struct file_operations __name##_fops = {				\
		.owner = THIS_MODULE,											\
		.open = __name##_open,											\
		.read = seq_read,												\
		.llseek = seq_lseek,											\
		.release = single_release,										\
	}

static void sysfs_display_ring(void *head, int size, int extend_desc,
			       struct seq_file *seq, dma_addr_t dma_phy_addr)
{
	int i;
	struct dma_desc *p = (struct dma_desc *)head;
	dma_addr_t dma_addr;

	for (i = 0; i < size; i++) {
		dma_addr = dma_phy_addr + i * sizeof(*p);
		seq_printf(seq, "%d [%pad]: 0x%x 0x%x 0x%x 0x%x\n",
			   i, &dma_addr,
			   le32_to_cpu(p->des0), le32_to_cpu(p->des1),
			   le32_to_cpu(p->des2), le32_to_cpu(p->des3));
		p++;
		seq_puts(seq, "\n");
	}
}

static int dn200_rings_status_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue;

	if ((dev->flags & IFF_UP) == 0)
		return 0;

	for (queue = 0; queue < rx_count; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		seq_printf(seq, "RX Queue %d:\n", queue);

		seq_puts(seq, "Descriptor ring:\n");
		sysfs_display_ring((void *)rx_q->dma_rx,
				   priv->dma_rx_size, 0, seq, rx_q->dma_rx_phy);
	}

	for (queue = 0; queue < tx_count; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];

		seq_printf(seq, "TX Queue %d:\n", queue);
		seq_puts(seq, "Descriptor ring:\n");
		sysfs_display_ring((void *)tx_q->dma_tx,
				   priv->dma_tx_size, 0, seq, tx_q->dma_tx_phy);
	}

	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_rings_status);

static int dn200_dma_cap_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);

	if (!priv->hw_cap_support) {
		seq_puts(seq, "DMA HW features not supported\n");
		return 0;
	}

	seq_puts(seq, "==============================\n");
	seq_puts(seq, "\tDMA HW features\n");
	seq_puts(seq, "==============================\n");

	seq_printf(seq, "\t10/100 Mbps: %s\n",
		   (priv->dma_cap.mbps_10_100) ? "Y" : "N");
	seq_printf(seq, "\t1000 Mbps: %s\n",
		   (priv->dma_cap.mbps_1000) ? "Y" : "N");
	seq_printf(seq, "\tHalf duplex: %s\n",
		   (priv->dma_cap.half_duplex) ? "Y" : "N");
	seq_printf(seq, "\tHash Filter: %s\n",
		   (priv->dma_cap.hash_filter) ? "Y" : "N");
	seq_printf(seq, "\tMultiple MAC address registers: %s\n",
		   (priv->dma_cap.multi_addr) ? "Y" : "N");
	seq_printf(seq, "\tPCS (TBI/SGMII/RTBI PHY interfaces): %s\n",
		   (priv->dma_cap.pcs) ? "Y" : "N");
	seq_printf(seq, "\tSMA (MDIO) Interface: %s\n",
		   (priv->dma_cap.sma_mdio) ? "Y" : "N");
	seq_printf(seq, "\tPMT Remote wake up: %s\n",
		   (priv->dma_cap.pmt_remote_wake_up) ? "Y" : "N");
	seq_printf(seq, "\tPMT Magic Frame: %s\n",
		   (priv->dma_cap.pmt_magic_frame) ? "Y" : "N");
	seq_printf(seq, "\tRMON module: %s\n",
		   (priv->dma_cap.rmon) ? "Y" : "N");
	seq_printf(seq, "\tIEEE 1588-2002 Time Stamp: %s\n",
		   (priv->dma_cap.time_stamp) ? "Y" : "N");
	seq_printf(seq, "\tIEEE 1588-2008 Advanced Time Stamp: %s\n",
		   (priv->dma_cap.atime_stamp) ? "Y" : "N");
	seq_printf(seq, "\t802.3az - Energy-Efficient Ethernet (EEE): %s\n",
		   (priv->dma_cap.eee) ? "Y" : "N");
	seq_printf(seq, "\tAV features: %s\n", (priv->dma_cap.av) ? "Y" : "N");
	seq_printf(seq, "\tChecksum Offload in TX: %s\n",
		   (priv->dma_cap.tx_coe) ? "Y" : "N");
	if (priv->chip_id >= DWMAC_CORE_4_00) {
		seq_printf(seq, "\tIP Checksum Offload in RX: %s\n",
			   (priv->dma_cap.rx_coe) ? "Y" : "N");
	} else {
		seq_printf(seq, "\tIP Checksum Offload (type1) in RX: %s\n",
			   (priv->dma_cap.rx_coe_type1) ? "Y" : "N");
		seq_printf(seq, "\tIP Checksum Offload (type2) in RX: %s\n",
			   (priv->dma_cap.rx_coe_type2) ? "Y" : "N");
	}
	seq_printf(seq, "\tRXFIFO > 2048bytes: %s\n",
		   (priv->dma_cap.rxfifo_over_2048) ? "Y" : "N");
	seq_printf(seq, "\tNumber of Additional RX channel: %d\n",
		   priv->dma_cap.number_rx_channel);
	seq_printf(seq, "\tNumber of Additional TX channel: %d\n",
		   priv->dma_cap.number_tx_channel);
	seq_printf(seq, "\tNumber of Additional RX queues: %d\n",
		   priv->dma_cap.number_rx_queues);
	seq_printf(seq, "\tNumber of Additional TX queues: %d\n",
		   priv->dma_cap.number_tx_queues);
	seq_printf(seq, "\tEnhanced descriptors: %s\n",
		   (priv->dma_cap.enh_desc) ? "Y" : "N");
	seq_printf(seq, "\tTX Fifo Size: %d\n", priv->dma_cap.tx_fifo_size);
	seq_printf(seq, "\tRX Fifo Size: %d\n", priv->dma_cap.rx_fifo_size);
	seq_printf(seq, "\tHash Table Size: %d\n", priv->dma_cap.hash_tb_sz);
	seq_printf(seq, "\tTSO: %s\n", priv->dma_cap.tsoen ? "Y" : "N");
	seq_printf(seq, "\tNumber of PPS Outputs: %d\n",
		   priv->dma_cap.pps_out_num);
	seq_printf(seq, "\tSafety Features: %s\n",
		   priv->dma_cap.asp ? "Y" : "N");
	seq_printf(seq, "\tFlexible RX Parser: %s\n",
		   priv->dma_cap.frpsel ? "Y" : "N");
	seq_printf(seq, "\tEnhanced Addressing: %d\n", priv->dma_cap.addr64);
	seq_printf(seq, "\tReceive Side Scaling: %s\n",
		   priv->dma_cap.rssen ? "Y" : "N");
	seq_printf(seq, "\tVLAN Hash Filtering: %s\n",
		   priv->dma_cap.vlhash ? "Y" : "N");
	seq_printf(seq, "\tSplit Header: %s\n",
		   priv->dma_cap.sphen ? "Y" : "N");
	seq_printf(seq, "\tVLAN TX Insertion: %s\n",
		   priv->dma_cap.vlins ? "Y" : "N");
	seq_printf(seq, "\tDouble VLAN: %s\n", priv->dma_cap.dvlan ? "Y" : "N");
	seq_printf(seq, "\tNumber of L3/L4 Filters: %d\n",
		   priv->dma_cap.l3l4fnum);
	seq_printf(seq, "\tARP Offloading: %s\n",
		   priv->dma_cap.arpoffsel ? "Y" : "N");
	seq_printf(seq, "\tEnhancements to Scheduled Traffic (EST): %s\n",
		   priv->dma_cap.estsel ? "Y" : "N");
	seq_printf(seq, "\tFrame Preemption (FPE): %s\n",
		   priv->dma_cap.fpesel ? "Y" : "N");
	seq_printf(seq, "\tTime-Based Scheduling (TBS): %s\n",
		   priv->dma_cap.tbssel ? "Y" : "N");
	seq_printf(seq, "\tIs Bonding Member: %s\n",
		   test_bit(DN200_IS_BONDING, &priv->state) ? "Y" : "N");
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_dma_cap);

static int dn200_tx_diag_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue, hw_chan;
	u32 cache_lvl;
	u32 ch_tail, ch_curr, tx_avail;
	u32 dbg_status, dbg_status0, dbg_status1;

	if ((dev->flags & IFF_UP) == 0)
		return 0;

	seq_puts(seq, "=== Tx data path debug info ===\n");

	for (queue = 0; queue < tx_count; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
		u32 tobe_consume = 0;

		hw_chan = queue + DN200_RXQ_START_GET(priv->hw);

		/* get sw & hw ring status, include sw available descs number,
		 * hw to be consume descs number
		 */
		ch_tail = readl(priv->ioaddr +
				XGMAC_DMA_CH_TxDESC_TAIL_LPTR(hw_chan));
		ch_curr = readl(priv->ioaddr +
				XGMAC_DMA_CH_TxDESC_CURR_LPTR(hw_chan));
		tx_avail = dn200_tx_avail(priv, queue);

		seq_printf(seq, "TX Queue %d channel %d %s\n", queue, hw_chan,
			   ((tx_avail < priv->dma_tx_size - 1) ||
			    (ch_tail != ch_curr)) ?
				   "- running" :
				   "");

		if (ch_tail != ch_curr)
			tobe_consume = (ch_curr > ch_tail) ?
					       (priv->dma_tx_size -
						((ch_curr - ch_tail) / 16)) :
					       ((ch_tail - ch_curr) / 16 + 1);

		seq_printf(seq,
			   "Desc sw ring curr_tx:%d, dirty_tx:%d, tx available:%d, result:%s\n",
			   tx_q->cur_tx, tx_q->dirty_tx, tx_avail,
			   tx_avail >
			   DN200_TX_THRESH(priv) ? "yes (good)" :
			   "no (abnormal)");

		seq_printf(seq,
			   "Desc hw ring start:%#x, tail:%#x, curr:%#x, to consume:%u, result:%s\n",
			   readl(priv->ioaddr +
				 XGMAC_DMA_CH_TxDESC_LADDR(hw_chan)), ch_tail,
			   ch_curr, tobe_consume,
			   tobe_consume >
			   (priv->dma_tx_size /
			    2) ? "no (abnormal)" : "yes (good)");

		/*2. get descs cache level */
		cache_lvl =
		    readl(priv->ioaddr + XGMAC_CH_DESC_CACHE_LVL(hw_chan));
		seq_printf(seq,
			   "[reg 3168]Desc tx cache levle is:%lu, cache_lvl:%#x\n",
			   (cache_lvl & XGMAC_TXLVL), cache_lvl);

		/*3. get dma channel debug status */
		dbg_status = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(hw_chan));
		seq_printf(seq,
			   "[reg 3164]DMA channel TDWS-des write state:%#lx, TDTS-data transfer state:%#lx, TDFS-des fetch state:%#lx, TDRS:%#lx, TDXS:%#lx, dbg_status:%#x\n",
			   (dbg_status & XGMAC_TDWS) >> XGMAC_TDWS_SHIFT,
			   (dbg_status & XGMAC_TDTS) >> XGMAC_TDTS_SHIFT,
			   (dbg_status & XGMAC_TDFS) >> XGMAC_TDFS_SHIFT,
			   (dbg_status & XGMAC_TDRS) >> XGMAC_TDRS_SHIFT,
			   (dbg_status & XGMAC_TDXS) >> XGMAC_TDXS_SHIFT,
			   dbg_status);

		/*4. get tx dma FSM debug status1(dma_debug_status1) */
		dbg_status1 = readl(priv->ioaddr + XGMAC_DEBUG_ST1);
		seq_printf(seq,
			   "[reg 3024]Chanel %d DMA FSMs are%s actively processing the descriptors or packet data, dma_debug_status1:%#x.\n",
			   hw_chan,
			   (dbg_status1 & (1 << hw_chan)) ? "" : " Not",
			   dbg_status1);

		/*5. get tx dma debug status0(dma_debug_status0) */
		dbg_status0 = readl(priv->ioaddr + XGMAC_DEBUG_ST0);
		seq_printf(seq,
			   "[reg 3020]AXI Master Read Channel is%s active(tx global status), dma_debug_status0:%#x.\n",
			   (dbg_status0 & XGMAC_AXRHSTS) ? "" : " Not",
			   dbg_status0);
	}

	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_tx_diag);

static int dn200_rx_diag_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 queue, hw_chan;
	u32 cache_lvl;
	u32 mtl_rxq_dbg = 0;
	u32 mac_dbg = 0;
	u32 ch_tail, ch_curr, avail, tobe_refill, dbg_status, dbg_status0,
	    dma_chan_status;

	if ((dev->flags & IFF_UP) == 0)
		return 0;

	seq_puts(seq, "=== Rx data path diag info ===\n");

	mac_dbg = readl(priv->ioaddr + XGMAC_MAC_DEBUG);
	seq_printf(seq,
			"[reg 114]MAC RX dbg, MAC GMII/XGMII rx engine st:%#x, MAC rx small fifo st:%#x, reg:%#x\n",
			(mac_dbg & 0x1), (mac_dbg & 0x6 >> 1),
			mac_dbg);
	for (queue = 0; queue <= DN200_LAST_QUEUE(priv); queue++) {
		if (DN200_MTL_QUEUE_IS_VALID(priv, queue)) {
			mtl_rxq_dbg = readl(priv->ioaddr + XGMAC_MTL_RXQ_DEBUG(queue));
			seq_printf(seq,
					"[reg 1148]MTL FIFO:%d, Pkt Num in RXQ:%ld, RXQ fill level:%#lx, reg:%#x\n", queue,
					((mtl_rxq_dbg & XGMAC_PRXQ) >> 16),
					((mtl_rxq_dbg & XGMAC_RXQSTS) >> 4),
					mtl_rxq_dbg);
		}
	}

	for (queue = 0; queue < rx_count; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];

		hw_chan = queue + DN200_RXQ_START_GET(priv->hw);

		/* get sw & hw ring status, include sw to be refilled descs number,
		 * hw available descs number
		 */
		ch_tail = readl(priv->ioaddr +
				XGMAC_DMA_CH_RxDESC_TAIL_LPTR(hw_chan));
		ch_curr = readl(priv->ioaddr +
				XGMAC_DMA_CH_RxDESC_CURR_LPTR(hw_chan));
		avail = (ch_curr > ch_tail) ? (priv->dma_rx_size -
					       ((ch_curr - ch_tail) / 16 - 1)) :
					      ((ch_tail - ch_curr) / 16 + 1);
		tobe_refill = DN200_DESC_UNUSED(priv, rx_q);

		seq_printf(seq, "RX Queue %d channel %d %s\n", queue, hw_chan,
			   (tobe_refill > 0) ? "- running" : "");

		seq_printf(seq,
			   "Desc sw ring curr_rx:%d, dirty_rx:%d, tobe refilled:%d, rxi_us:%d, result:%s\n",
			   rx_q->cur_rx, rx_q->dirty_rx, tobe_refill, priv->rx_intr[queue].target_itr,
			   tobe_refill >
			   (priv->dma_rx_size /
			    2) ? "no (abnormal)" : "yes (good)");

		seq_printf(seq,
			   "Desc hw ring start:%#x, tail:%#x, curr:%#x, available:%u, result:%s.\n",
			   readl(priv->ioaddr +
				 XGMAC_DMA_CH_RxDESC_LADDR(hw_chan)), ch_tail,
			   ch_curr, avail,
			   avail >
			   (priv->dma_rx_size /
			    4) ? "yes (good)" : "no (abnormal)");

		/*2. if hw ring tail equal curr, or RBU/FBE occur, get the interrupt status */
		/* if hw ring tail == curr, means the hw have no descs to use, maybe sw stop to rx,
		 * e.g wihtout any hw interrupt is notified to cpu
		 *
		 * if RBU bit is set, means Receive Buffer Unavailable,
		 * or the application owns the next descriptor
		 * in the Receive list, and the DMA cannot acquire it.
		 * The Rx process is suspended.
		 * e.g. rx interrupt slow to trigger caused by firmware local cpu to
		 * process other jobs
		 */
		dma_chan_status =
		    readl(priv->ioaddr + XGMAC_DMA_CH_STATUS(hw_chan));
		/* RBU - rx buffer unavailable, FBE - fatal bus error */
		seq_printf(seq,
			   "[reg 3160]%s, all queue rx_normal_irq_n:%llu, reg val:%#x.\n",
			   ((ch_tail == ch_curr) ||
				 (dma_chan_status & (XGMAC_RBU | XGMAC_FBE))) ?
			   "Error:slow or no itr lead to RBU or hw ring empty(or FBE)"
			   : "Good:rx dma channel status ok.",
			   priv->xstats.rx_normal_irq_n, dma_chan_status);

		/*3. get descs cache level */
		cache_lvl =
		    readl(priv->ioaddr + XGMAC_CH_DESC_CACHE_LVL(hw_chan));
		seq_printf(seq,
			   "[reg 3168]Desc rx cache level is:%lu, tx cache levle is:%lu, cache_lvl:%#x\n",
			   (cache_lvl & XGMAC_RXLVL) >> XGMAC_RXLVL_SHIFT,
			   (cache_lvl & XGMAC_TXLVL), cache_lvl);

		/*4. get dma channel debug status */
		dbg_status = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(hw_chan));
		seq_printf(seq,
			   "[reg 3164]DMA channel RDWS-des write state:%#lx, RDTS-data transfer state:%#lx, RDFS-des fetch state:%#lx, dbg_status:%#x\n",
			   (dbg_status & XGMAC_RDWS) >> XGMAC_RDWS_SHIFT,
			   (dbg_status & XGMAC_RDTS) >> XGMAC_RDTS_SHIFT,
			   (dbg_status & XGMAC_RDFS) >> XGMAC_RDFS_SHIFT,
			   dbg_status);

		/*5. get rx dma debug status0(dma_debug_status0) */
		dbg_status0 = readl(priv->ioaddr + XGMAC_DEBUG_ST0);
		seq_printf(seq,
			   "[reg 3020]AXI Master Write Channel is%s active(rx global status), dma_debug_status0:%#x.\n",
			   (dbg_status0 & XGMAC_AXWHSTS) ? "" : " Not",
			   dbg_status0);
	}

	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_rx_diag);

static int dn200_rx_buf_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	u32 rx_count = priv->plat->rx_queues_to_use;
	u32 queue, cache_id;
	u32 head, tail, stride, avail_entry;
	struct dn200_bufring *r;
	struct dn200_page_buf *rx_buf;
	int idx = 0;

	if ((dev->flags & IFF_UP) == 0)
		return 0;

	seq_puts(seq, "=== Rx Buffer debug info ===\n");

	for (queue = 0; queue < rx_count; queue++) {
		struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
		struct dn200_bufpool_cache *local_cache =
		    &rx_q->rx_pool->local_cache[queue];
		struct dn200_buf_cache_ring *buf_cached =
		    &local_cache->buf_cached;
		struct dn200_buf_refill_stack *buf_refill =
		    &local_cache->buf_refill;
		seq_printf(seq, "=== Rx buf queue:%d ===\n", queue);
		seq_printf(seq,
			   "-- Refill stack: cache size:%d, current len:%d --\n",
			   buf_refill->cache_size, buf_refill->current_len);

		for (cache_id = 0; cache_id < buf_refill->current_len;
		     ++cache_id) {
			rx_buf =
			    (struct dn200_page_buf *)buf_refill->objs[cache_id];
			if (!rx_buf->page)
				continue;

			seq_printf(seq,
				   "cache id:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
				   cache_id, rx_buf->kernel_addr,
				   rx_buf->desc_addr, rx_buf->page_offset,
				   page_to_phys(rx_buf->page), rx_buf->buf_len,
				   rx_buf->busy_cnt, rx_buf->page,
				   page_ref_count(rx_buf->page));
		}

		seq_printf(seq,
			   "-- Cache ring: cache size:%d, head:%d, tail:%d --\n",
			   buf_cached->cache_size, buf_cached->head,
			   buf_cached->tail);
		if (buf_cached->head >= buf_cached->tail) {
			for (cache_id = buf_cached->tail;
			     cache_id < buf_cached->head; cache_id++) {
				rx_buf =
				    (struct dn200_page_buf *)buf_cached->objs[cache_id];
				if (!rx_buf->page)
					continue;

				seq_printf(seq,
					   "cache id:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
					   cache_id, rx_buf->kernel_addr,
					   rx_buf->desc_addr,
					   rx_buf->page_offset,
					   page_to_phys(rx_buf->page),
					   rx_buf->buf_len, rx_buf->busy_cnt,
					   rx_buf->page,
					   page_ref_count(rx_buf->page));
			}
		} else {
			for (cache_id = buf_cached->tail;
			     cache_id < buf_cached->cache_size; cache_id++) {
				rx_buf =
				    (struct dn200_page_buf *)buf_cached->objs[cache_id];
				if (!rx_buf->page)
					continue;

				seq_printf(seq,
					   "cache id:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
					   cache_id, rx_buf->kernel_addr,
					   rx_buf->desc_addr,
					   rx_buf->page_offset,
					   page_to_phys(rx_buf->page),
					   rx_buf->buf_len, rx_buf->busy_cnt,
					   rx_buf->page,
					   page_ref_count(rx_buf->page));
			}
			for (cache_id = 0; cache_id < buf_cached->head;
			     cache_id++) {
				rx_buf = (struct dn200_page_buf *)buf_cached->objs[cache_id];
				if (!rx_buf->page)
					continue;

				seq_printf(seq,
					   "cache id:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
					   cache_id, rx_buf->kernel_addr,
					   rx_buf->desc_addr,
					   rx_buf->page_offset,
					   page_to_phys(rx_buf->page),
					   rx_buf->buf_len, rx_buf->busy_cnt,
					   rx_buf->page,
					   page_ref_count(rx_buf->page));
			}
		}
	}
	r = priv->buf_pool.pool_ring;
	head = atomic_read(&r->prod.tail);
	tail = atomic_read(&r->cons.head);
	if (head >= tail)
		avail_entry = head - tail;
	else
		avail_entry = r->ring_size + head - tail;
	seq_printf(seq,
		   "==== total rx_buf %d pool_ring availd %d head %d tail %d ring_size %d====\n",
		   priv->page_pool.total_pages *
		   priv->buf_pool.buf_num_per_page, avail_entry, head, tail,
		   r->ring_size);

	r = priv->buf_pool.cached_ring;
	head = atomic_read(&r->prod.tail);
	tail = atomic_read(&r->cons.head);
	if (head >= tail)
		avail_entry = head - tail;
	else
		avail_entry = r->ring_size + head - tail;

	seq_printf(seq,
		   "====cached_ring availd %d head %d tail %d ring_size %d====\n",
		   avail_entry, head, tail, r->ring_size);
	stride = priv->buf_pool.buf_num_per_page;
	if (head >= tail) {
		for (idx = tail; idx < head; idx += stride) {
			rx_buf = (struct dn200_page_buf *)r->ring_objs[idx];
			if (page_ref_count(rx_buf->page) == 1)
				continue;

			seq_printf(seq,
				   "idx:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
				   idx, rx_buf->kernel_addr, rx_buf->desc_addr,
				   rx_buf->page_offset,
				   page_to_phys(rx_buf->page), rx_buf->buf_len,
				   rx_buf->busy_cnt, rx_buf->page,
				   page_ref_count(rx_buf->page));
		}
	} else {
		for (idx = tail; idx < r->ring_size; idx += stride) {
			rx_buf = (struct dn200_page_buf *)r->ring_objs[idx];
			if (page_ref_count(rx_buf->page) == 1)
				continue;

			seq_printf(seq,
				   "idx:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
				   idx, rx_buf->kernel_addr, rx_buf->desc_addr,
				   rx_buf->page_offset,
				   page_to_phys(rx_buf->page), rx_buf->buf_len,
				   rx_buf->busy_cnt, rx_buf->page,
				   page_ref_count(rx_buf->page));
		}
		for (idx = 0; idx < head; idx += stride) {
			rx_buf = (struct dn200_page_buf *)r->ring_objs[idx];
			if (page_ref_count(rx_buf->page) == 1)
				continue;

			seq_printf(seq,
				   "idx:%d, dma:%#llx, descaddr:%#llx, offset:%d, page_to_phys:%#llx, len:%d, busy_cnt:%d, page:%p, page_ref_count:%d\n",
				   idx, rx_buf->kernel_addr, rx_buf->desc_addr,
				   rx_buf->page_offset,
				   page_to_phys(rx_buf->page), rx_buf->buf_len,
				   rx_buf->busy_cnt, rx_buf->page,
				   page_ref_count(rx_buf->page));
		}
	}
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_rx_buf);

static int dn200_ring_status_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	int rx_chan = priv->plat->rx_queues_to_use;
	struct dn200_rx_queue *rx_q = NULL;
	int i = 0;
	int count_rx = 0;

	if (!netif_running(dev))
		return 0;
	seq_puts(seq, "=== Get RX ring status ===\n");
	for (; i < rx_chan; i++) {
		rx_q = &priv->rx_queue[i];
		seq_printf(seq, "queue %d cur_rx %#x dirty_rx %#x\n", i,
			   rx_q->cur_rx, rx_q->dirty_rx);
		seq_printf(seq, "init %#x tail_phy %#x curr %#x\n",
			   readl(priv->ioaddr + XGMAC_DMA_CH_RxDESC_LADDR(i)),
			   readl(priv->ioaddr +
				 XGMAC_DMA_CH_RxDESC_TAIL_LPTR(i)),
			   readl(priv->ioaddr +
				 XGMAC_DMA_CH_RxDESC_CURR_LPTR(i)));
		if (readl(priv->ioaddr + XGMAC_DMA_CH_RxDESC_TAIL_LPTR(i)) ==
		    readl(priv->ioaddr + XGMAC_DMA_CH_RxDESC_CURR_LPTR(i))) {
			count_rx = dn200_rx(priv, 64, i, priv->dma_rx_size);
			seq_printf(seq, "queue %d dn200_rx reture %d\n", i,
				   count_rx);
		}
	}
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_ring_status);

static int dn200_hw_lock_test_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	unsigned long start_time, end_time;
	bool is_locked = false;
	int ret = 0;

	seq_puts(seq, "=== Test HW lock & unlock ===\n");
	start_time = ktime_get_ns();
	ret = dn200_hw_lock(priv->hw, &is_locked);
	end_time = ktime_get_ns();
	if (ret == 0) {
		seq_printf(seq, "HW locked successful, ret: %d, cost ns:%lu\n",
			ret, end_time - start_time);
		start_time = ktime_get_ns();
		dn200_hw_unlock(priv->hw, &is_locked);
		end_time = ktime_get_ns();
		if (is_locked == false) {
			seq_printf(seq, "HW unlock successful, ret: %d, cost ns:%lu\n",
				ret, end_time - start_time);
		}
	} else {
		seq_printf(seq, "HW lock failure, ret: %d\n", ret);
	}
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_hw_lock_test);

static int dn200_fifo_size_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	int rx_chan = priv->plat_ex->rx_queues_total;
	int tx_chan = priv->plat_ex->tx_queues_total;
	int i = 0;
	u32 fifosize = 0;
	u32 value = 0;
	bool pf_use = false;
	bool vf_use = false;

	if (PRIV_IS_VF(priv))
		return 0;
	if (!netif_running(dev))
		return 0;

	seq_puts(seq, "=== GET HW RX FIFOSIZE ===\n");
	for ( ; i < rx_chan; i++) {
		value = readl(priv->ioaddr + XGMAC_MTL_RXQ_OPMODE(i));
		fifosize = (value & XGMAC_RQS) >> XGMAC_RQS_SHIFT;
		fifosize = (fifosize + 1) * 256;
		seq_printf(seq, "mtl rx chan: %#x fifosize %#x\n",
			   i, fifosize);
	}

	seq_puts(seq, "=== GET HW TX FIFOSIZE ===\n");
	for (i = 0; i < tx_chan; i++) {
		value = readl(priv->ioaddr + XGMAC_MTL_TXQ_OPMODE(i));
		fifosize = (value & XGMAC_TQS) >> XGMAC_TQS_SHIFT;
		fifosize = (fifosize + 1) * 256;
		if (i < priv->plat->tx_queues_to_use)
			pf_use = true;
		else
			pf_use = false;
		if (PRIV_SRIOV_SUPPORT(priv)) {
			if (i >= 8)
				vf_use = true;
			else
				vf_use = false;
		}
		seq_printf(seq, "mtl tx chan: %#x fifosize %#x %s\n",
				 i, fifosize,
				 vf_use ? "vf use" : (pf_use ? "pf enable" : "pf disable"));
	}
	return 0;
}
DN200_DEFINE_SHOW_ATTRIBUTE(dn200_fifo_size);

static int dn200_rxp_satus_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);

	dn200_rxp_filter_get(priv, priv->hw, seq);
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_rxp_satus);

static int dn200_iatu_show(struct seq_file *seq, void *v)
{
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);

	if (!netif_running(dev))
		return 0;

	dn200_iatu_display(priv, seq);
	return 0;
}

DN200_DEFINE_SHOW_ATTRIBUTE(dn200_iatu);

static char dn200_ext_phy_buf[128];
static int dn200_ext_phy_show(struct seq_file *seq, void *v)
{
	if (strlen(dn200_ext_phy_buf))
		seq_printf(seq, "%s\n", dn200_ext_phy_buf);
	return 0;
}

static ssize_t dn200_ext_phy_write(struct file *file,
		 const char __user *buf, size_t size, loff_t *ppos)
{
	struct seq_file *seq = file->private_data;
	struct net_device *dev = seq->private;
	struct dn200_priv *priv = netdev_priv(dev);
	struct phy_device *phydev;
	char temp[64] = {};
	char *cur;
	unsigned long value;
	int addr;
	u32 reg = 0, val = 0, len = 0;

	addr = priv->plat->phy_addr;
	phydev = mdiobus_get_phy(priv->mii, addr);
	if (!phydev)
		return size;

	if (size > sizeof(temp)) {
		sprintf(dn200_ext_phy_buf, "cmd is too long\n");
		return size;
	}
	if (copy_from_user(temp, buf, size))
		return size;

	cur = strchr(temp, ' ');
	if (!cur) {
		sprintf(dn200_ext_phy_buf, "\"r reg\" or \"w reg val\"\n");
		return 0;
	}
	while (*cur == ' ')
		cur++;

	if (kstrtoul(cur, 0, &value))
		return 0;
	reg = value;

	cur = strchr(cur, ' ');
	if (cur) {
		while (*cur == ' ')
			cur++;
		if (kstrtoul(cur, 0, &value))
			return 0;
		val = value;
	}

	if (reg >= 0x20) {
		reg &= 0xffff;
		if (temp[0] == 'w') {
			ytphy_write_ext(phydev, reg, val);
			len = sprintf(dn200_ext_phy_buf,
				 "Write phy %d ext.%#x %#x\n", addr, reg, val);
		}

		sprintf(dn200_ext_phy_buf + len,
				 "Read phy %d ext.%#x val: %#x\n", addr, reg,
			ytphy_read_ext(phydev, reg));
	} else {
		if (temp[0] == 'w') {
			mdiobus_write(priv->mii, priv->plat->phy_addr, reg, val);
			len = sprintf(dn200_ext_phy_buf,
					 "Write phy %d %#x %#x\n", addr, reg, val);
		}

		sprintf(dn200_ext_phy_buf + len,
				 "Read phy %d %#x val: %#x\n", addr, reg,
			mdiobus_read(priv->mii, priv->plat->phy_addr, reg));
	}

	return size;
}

static int dn200_ext_phy_open(struct inode *inode, struct file *file)
{
	return single_open(file, dn200_ext_phy_show, inode->i_private);
}

static const struct file_operations dn200_ext_phy_fops = {
	.owner = THIS_MODULE,
	.open = dn200_ext_phy_open,
	.read = seq_read,
	.write = dn200_ext_phy_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void dn200_init_fs(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	rtnl_lock();
	/* Create per netdev entries */
	priv->dbgfs_dir = debugfs_create_dir(dev->name, NULL);

	/* Entry to report DMA RX/TX rings */
	debugfs_create_file("descriptors_status", 0444, priv->dbgfs_dir, dev,
			    &dn200_rings_status_fops);

	/* Entry to report the DMA HW features */
	debugfs_create_file("dma_cap", 0444, priv->dbgfs_dir, dev,
			    &dn200_dma_cap_fops);

	debugfs_create_file("ring_status", 0444, priv->dbgfs_dir, dev,
			    &dn200_ring_status_fops);

	debugfs_create_file("lock_test", 0444, priv->dbgfs_dir, dev,
			    &dn200_hw_lock_test_fops);

	debugfs_create_file("fifo_size", 0444, priv->dbgfs_dir, dev,
			    &dn200_fifo_size_fops);

	debugfs_create_file("rxp_status", 0444, priv->dbgfs_dir, dev,
			    &dn200_rxp_satus_fops);

	/* Entry to report TX/RX data path diagnostic info */
	debugfs_create_file("tx_diag", 0444, priv->dbgfs_dir, dev,
			    &dn200_tx_diag_fops);
	debugfs_create_file("rx_diag", 0444, priv->dbgfs_dir, dev,
			    &dn200_rx_diag_fops);

	debugfs_create_file("rx_buf", 0444, priv->dbgfs_dir, dev,
			    &dn200_rx_buf_fops);
	debugfs_create_file("iatu", 0444, priv->dbgfs_dir, dev,
			    &dn200_iatu_fops);
	if (priv->mii)
		debugfs_create_file("ext_phy", 0644, priv->dbgfs_dir, dev,
			    &dn200_ext_phy_fops);

	rtnl_unlock();
}

static void dn200_exit_fs(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);

	debugfs_remove_recursive(priv->dbgfs_dir);
}

static int dn200_vlan_update(struct dn200_priv *priv)
{
	int count = 0;
	u16 vid = 0;

	for_each_set_bit(vid, priv->active_vlans, VLAN_N_VID) {
		if (count >= 4095)
			return -1;
		count++;
	}
	priv->plat_ex->vlan_num = count;
	if (!(PRIV_IS_PUREPF(priv) || PRIV_IS_VF(priv)) &&
		!test_bit(DN200_DEV_INIT, &priv->state))
		_dn200_config_vlan_rx_fltr(priv, priv->hw,
					  !(priv->dev->flags & IFF_PROMISC));
	return priv->plat_ex->vlan_num;
}

static int dn200_vlan_rx_add_vid(struct net_device *ndev, __be16 proto, u16 vid)
{
	struct dn200_priv *priv = netdev_priv(ndev);
	bool is_double = false;
	int ret;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	if (HW_IS_VF(priv->hw))
		return 0;
	if (be16_to_cpu(proto) == ETH_P_8021AD)
		is_double = true;

	if (is_double && !HW_IS_PUREPF(priv->hw))
		return -EOPNOTSUPP;

	set_bit(vid, priv->active_vlans);
	ret = dn200_vlan_update(priv);
	if (ret < 0) {
		clear_bit(vid, priv->active_vlans);
		return ret;
	}
	if (HW_IS_PUREPF(priv->hw)) {
		ret =
		    dn200_add_hw_vlan_rx_fltr(priv, ndev, priv->hw, proto, vid,
					      0, 0);
		if (ret)
			return ret;
	} else {
		priv->pf_rxp_set |= RXP_SET_VLAN_ID;
		if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
			queue_work(priv->wq, &priv->rxp_task);
		else if (PRIV_SRIOV_SUPPORT(priv))
			set_bit(DN200_RXP_NEED_CHECK, &priv->state);
	}
	return 0;
}

static void dn200_vlan_reconfig(struct dn200_priv *priv)
{
	u16 vid = 0;

	if (PRIV_IS_VF(priv))
		return;

	if (HW_IS_PUREPF(priv->hw)) {
		for_each_set_bit(vid, priv->active_vlans, VLAN_N_VID) {
			__le16 vid_le = cpu_to_le16(vid);

			dn200_add_hw_vlan_rx_fltr(priv, priv->dev, priv->hw, 0,
						  vid_le, 0, 0);
		}
	} else {
		priv->pf_rxp_set |= RXP_SET_VLAN_ID;
		if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
			queue_work(priv->wq, &priv->rxp_task);
		else if (PRIV_SRIOV_SUPPORT(priv))
			set_bit(DN200_RXP_NEED_CHECK, &priv->state);
	}
}

static int dn200_vlan_rx_kill_vid(struct net_device *ndev, __be16 proto,
				  u16 vid)
{
	struct dn200_priv *priv = netdev_priv(ndev);
	bool is_double = false;
	int ret;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	if (HW_IS_VF(priv->hw))
		return 0;
	if (be16_to_cpu(proto) == ETH_P_8021AD)
		is_double = true;

	clear_bit(vid, priv->active_vlans);
	ret = dn200_vlan_update(priv);
	if (ret < 0)
		return ret;

	if (HW_IS_PUREPF(priv->hw)) {
		ret =
		    dn200_del_hw_vlan_rx_fltr(priv, ndev, priv->hw, proto, vid,
					      0, 0);
		if (ret)
			return ret;
	} else {
		priv->pf_rxp_set |= RXP_SET_VLAN_ID;
		if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
			queue_work(priv->wq, &priv->rxp_task);
		else if (PRIV_SRIOV_SUPPORT(priv))
			set_bit(DN200_RXP_NEED_CHECK, &priv->state);
	}
	return 0;
}

void dn200_disable_rx_queue(struct dn200_priv *priv, u32 queue)
{
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned long flags;

	spin_lock_irqsave(&ch->lock, flags);
	dn200_disable_dma_irq(priv, priv->ioaddr, queue, 1, 0, priv->hw);
	spin_unlock_irqrestore(&ch->lock, flags);

	dn200_stop_rx_dma(priv, queue);
	__free_dma_rx_desc_resources(priv, queue);
}

void dn200_enable_rx_queue(struct dn200_priv *priv, u32 queue)
{
	struct dn200_rx_queue *rx_q = &priv->rx_queue[queue];
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned long flags;
	int ret;

	ret = __alloc_dma_rx_desc_resources(priv, queue);
	if (ret) {
		netdev_err(priv->dev, "Failed to alloc RX desc.\n");
		return;
	}

	ret = __init_dma_rx_desc_rings(priv, queue, GFP_KERNEL);
	if (ret) {
		__free_dma_rx_desc_resources(priv, queue);
		netdev_err(priv->dev, "Failed to init RX desc.\n");
		return;
	}

	dn200_clear_rx_descriptors(priv, queue);

	dn200_init_rx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
			   rx_q->dma_rx_phy, rx_q->queue_index, priv->hw);

	rx_q->rx_tail_addr = rx_q->dma_rx_phy + (rx_q->buf_alloc_num *
						 sizeof(struct dma_desc));
	dn200_set_rx_tail_ptr(priv, priv->ioaddr,
			      rx_q->rx_tail_addr, rx_q->queue_index, priv->hw);
	dn200_set_dma_bfsize(priv, priv->ioaddr,
			     priv->dma_buf_sz, rx_q->queue_index, priv->hw);
	dn200_start_rx_dma(priv, queue);

	spin_lock_irqsave(&ch->lock, flags);
	dn200_enable_dma_irq(priv, priv->ioaddr, queue, 1, 0, priv->hw);
	spin_unlock_irqrestore(&ch->lock, flags);
}

void dn200_disable_tx_queue(struct dn200_priv *priv, u32 queue)
{
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned long flags;

	spin_lock_irqsave(&ch->lock, flags);
	dn200_disable_dma_irq(priv, priv->ioaddr, queue, 0, 1, priv->hw);
	spin_unlock_irqrestore(&ch->lock, flags);

	dn200_stop_tx_dma(priv, queue);
	__free_dma_tx_desc_resources(priv, queue);
}

void dn200_enable_tx_queue(struct dn200_priv *priv, u32 queue)
{
	struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];
	struct dn200_channel *ch = &priv->channel[queue];
	unsigned long flags;
	int ret;

	ret = __alloc_dma_tx_desc_resources(priv, queue);
	if (ret) {
		netdev_err(priv->dev, "Failed to alloc TX desc.\n");
		return;
	}

	ret = __init_dma_tx_desc_rings(priv, queue);
	if (ret) {
		__free_dma_tx_desc_resources(priv, queue);
		netdev_err(priv->dev, "Failed to init TX desc.\n");
		return;
	}

	dn200_clear_tx_descriptors(priv, queue);

	dn200_init_tx_chan(priv, priv->ioaddr, priv->plat->dma_cfg,
			   tx_q->dma_tx_phy, tx_q->queue_index, priv->hw);

	tx_q->tx_tail_addr = tx_q->dma_tx_phy;
	dn200_set_tx_tail_ptr(priv, priv->ioaddr,
			      tx_q->tx_tail_addr, tx_q->queue_index, priv->hw);

	dn200_start_tx_dma(priv, queue);

	spin_lock_irqsave(&ch->lock, flags);
	dn200_enable_dma_irq(priv, priv->ioaddr, queue, 0, 1, priv->hw);
	spin_unlock_irqrestore(&ch->lock, flags);
}

static netdev_features_t
dn200_features_check(struct sk_buff *skb,
		     struct net_device __always_unused *netdev,
		     netdev_features_t features)
{
	/* No point in doing any of this if neither checksum nor GSO are
	 * being requested for this frame.  We can rule out both by just
	 * checking for CHECKSUM_PARTIAL
	 */
	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return features;

	/* We cannot support GSO if the MSS is going to be less than
	 * 64 bytes.  If it is then we need to drop support for GSO.
	 */
	if (skb_is_gso(skb) && (skb_shinfo(skb)->gso_size < 64))
		features &= ~NETIF_F_GSO_MASK;

	return features;
}

static void dn200_get_stats64(struct net_device *netdev,
			      struct rtnl_link_stats64 *s)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_counters *mmc = &priv->mmc;

	if (test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return;
	if (PRIV_IS_VF(priv)) {
		s->rx_packets = netdev->stats.rx_packets;
		s->rx_bytes = netdev->stats.rx_bytes;
		s->rx_errors = netdev->stats.rx_errors;
		s->rx_dropped = netdev->stats.rx_dropped;

		s->tx_packets = netdev->stats.tx_packets;
		s->tx_bytes = netdev->stats.tx_bytes;
		s->tx_errors = netdev->stats.tx_errors;
		s->tx_dropped = netdev->stats.tx_dropped;
	} else {
		if (!test_bit(DN200_SUSPENDED, &priv->state) && !test_bit(DN200_DOWN, &priv->state))
			dn200_mmc_read(priv, priv->mmcaddr, &priv->mmc);

		s->rx_packets = netdev->stats.rx_packets;
		s->rx_bytes = netdev->stats.rx_bytes;
		s->rx_errors = mmc->mmc_rx_crc_error +
		    mmc->mmc_rx_align_error +
		    mmc->mmc_rx_run_error +
		    mmc->mmc_rx_jabber_error +
		    mmc->mmc_rx_length_error +
		    mmc->mmc_rx_watchdog_error +
		    mmc->mmc_rx_udp_err +
		    mmc->mmc_rx_tcp_err +
		    mmc->mmc_rx_icmp_err +
		    mmc->mmc_rx_packet_assembly_err_cntr;
		s->multicast = mmc->mmc_rx_multicastframe_g;
		s->rx_length_errors = mmc->mmc_rx_length_error;
		s->rx_crc_errors = mmc->mmc_rx_crc_error;

		s->tx_packets = netdev->stats.tx_packets;
		s->tx_bytes = netdev->stats.tx_bytes;
		s->tx_errors = mmc->mmc_tx_underflow_error;
		s->tx_dropped = netdev->stats.tx_dropped;
	}
}

static void dn200_linkset_subtask(struct dn200_priv *priv)
{
	int retry = 0;

	netdev_info(priv->dev, "vf adaptor link status set.\n");
	if (test_bit(DN200_DOWN, &priv->state) &&
			 (priv->vf_link_action & LINK_DOWN_SET))
		return;

	if (!test_bit(DN200_DOWN, &priv->state) &&
			 (priv->vf_link_action & LINK_UP_SET))
		return;

	if (test_bit(DN200_DOWN, &priv->state) &&
			 (priv->vf_link_action & LINK_UP_SET)) {
		netif_trans_update(priv->dev);
		rtnl_lock();
		dev_open(priv->dev, NULL);
		rtnl_unlock();
	} else if (!test_bit(DN200_DOWN, &priv->state) &&
			 (priv->vf_link_action & LINK_DOWN_SET)) {
		netif_trans_update(priv->dev);
		while (test_and_set_bit(DN200_RESETING, &priv->state)) {
			usleep_range(1000, 2000);
			if (retry++ >= 3)
				return;
		}
		rtnl_lock();
		dev_close(priv->dev);
		rtnl_unlock();
		clear_bit(DN200_RESETING, &priv->state);
	}
	priv->vf_link_action = 0;
}

static void dn200_linkset_task(struct work_struct *work)
{
	struct dn200_priv *priv = container_of(work, struct dn200_priv,
					       vf_linkset_task);

	dn200_linkset_subtask(priv);
}

static void dn200_set_vf_rxp(struct dn200_priv *priv, u8 vf_num, struct dn200_vf_rxp_async_info *info)
{
	if (info->type & DN200_VF_CLEAR_RXP) {
		dn200_wq_vf_del_rxp(priv, priv->hw, 1 + vf_num, info->rxq_start);
	} else {
		dn200_vf_append_rxp_bc(priv, priv->hw, info->rxq_start);
		//if (info->type & DN200_VF_SET_UMAC)
			dn200_wq_set_umac_addr(priv, priv->hw, NULL, 0, info);
		//if (info->type & DN200_VF_SET_FLT)
			dn200_wq_set_filter(priv, priv->hw, priv->dev, true, info);
	}
}

static void dn200_set_pf_rxp(struct dn200_priv *priv, struct dn200_vf_rxp_async_info *info)
{
	u8 vf_offset = 0;

	if (priv->pf_rxp_set & RXP_SET_UMAC) {
		priv->pf_rxp_set &= ~RXP_SET_UMAC;
		dn200_wq_set_umac_addr(priv, priv->hw, (unsigned char *)priv->dev->dev_addr, 0, info);
	}
	if (priv->pf_rxp_set & RXP_SET_FIL) {
		priv->pf_rxp_set &= ~RXP_SET_FIL;
		dn200_wq_set_filter(priv, priv->hw, priv->dev, false, NULL);
	}
	if (priv->pf_rxp_set & RXP_SET_VLAN_FIL) {
		priv->pf_rxp_set &= ~RXP_SET_VLAN_FIL;
		dn200_config_vlan_rx_fltr(priv, priv->hw, priv->vlan_fil_enable);
	}
	if (priv->pf_rxp_set & RXP_SET_VLAN_ID) {
		priv->pf_rxp_set &= ~RXP_SET_VLAN_ID;
		dn200_sriov_vlan_entry_update(priv);
	}
	if (priv->pf_rxp_set & RXP_CLEAR_VF_RXP) {
		priv->pf_rxp_set &= ~RXP_CLEAR_VF_RXP;
		for (vf_offset = 0; vf_offset < DN200_MAX_VF_NUM; vf_offset++) {
			if (priv->clear_vf_rxp_bitmap & (1 << vf_offset)) {
				dn200_clear_vf_rxp(priv, priv->hw, vf_offset);
				priv->clear_vf_rxp_bitmap &= ~(1 << vf_offset);
			}
		}
	}
}

void dn200_async_rxp_work(struct dn200_priv *priv)
{
	netdev_dbg(priv->dev, "%s: pf ready to set rxp.\n", __func__);
	if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
		queue_work(priv->wq, &priv->rxp_task);
	else if (PRIV_SRIOV_SUPPORT(priv))
		set_bit(DN200_RXP_NEED_CHECK, &priv->state);
}

static void dn200_rxp_task(struct work_struct *work)
{
	struct dn200_priv *priv = container_of(work, struct dn200_priv,
					       rxp_task);
	struct dn200_vf_rxp_async_info *info;
	struct dn200_vf_rxp_async_info *lram_info;
	size_t info_size = sizeof(struct dn200_vf_rxp_async_info);
	u8 i = 0;
	u32 tmp_crc32;

	if (!netif_carrier_ok(priv->dev)) {
		set_bit(DN200_RXP_NEED_CHECK, &priv->state);
		return;
	}
	if (test_and_set_bit(DN200_RXP_SETTING, &priv->state))
		return;

	lram_info = devm_kzalloc(priv->device, info_size, GFP_KERNEL);
	if (!lram_info)
		return;
	/* check priv wq status first */
	if (priv->pf_rxp_set)
		netdev_dbg(priv->dev, "%s %d pf_set_rxp %#x\n", __func__, __LINE__, priv->pf_rxp_set);
	if (priv->pf_rxp_set != 0)
		dn200_set_pf_rxp(priv, lram_info);

	/* check vf wq status*/
	for (i = 0; i < priv->plat_ex->pf.registered_vfs; i++) {
		info = &priv->async_info[i];
		dn200_get_lram_rxp_async_info(priv->hw, (u8 *)lram_info, i);
		if (lram_info->crc32)
			netdev_dbg(priv->dev, "%s %d vf %d cfg_seq %d set seq %d, cfg crc32 %#x cur_crc32 %#x\n", __func__, __LINE__, i,
				lram_info->seq, info->seq, lram_info->crc32, info->crc32);
		if (info->seq == lram_info->seq && info->crc32 == lram_info->crc32)
			continue;
		tmp_crc32 = crc32_le(~0, (u8 *)lram_info + sizeof(u32), info_size - sizeof(u32));
		netdev_dbg(priv->dev, "%s %d vf %d tmp crc32 %#x\n", __func__, __LINE__, i, tmp_crc32);
		if (lram_info->crc32 != tmp_crc32)
			continue;
		memcpy(info, lram_info, info_size);
		dn200_set_vf_rxp(priv, i, info);
	}
	devm_kfree(priv->device, lram_info);
	clear_bit(DN200_RXP_SETTING, &priv->state);
}

static void dn200_uc_addr_get(struct dn200_priv *priv, int vf_id, u8 *addr);
static void dn200_uc_addr_set(struct dn200_priv *priv, int vf_id, u8 *addr);
static int dn200_ndo_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac)
{
	u8 mac_addr[ETH_ALEN];
	struct dn200_priv *priv = netdev_priv(netdev);

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	if (PRIV_IS_PUREPF(priv)) {
		netdev_warn(priv->dev, "PURE PF not support vf\n");
		return -EOPNOTSUPP;
	}

	if (!priv->plat_ex->pf.registered_vfs) {
		netdev_warn(priv->dev, "VF not exist\n");
		return -EAGAIN;
	} else if (vf_id >= priv->plat_ex->pf.registered_vfs) {
		netdev_err(priv->dev, "Invalid VF Identifier %d\n", vf_id);
		return -EINVAL;
	}

	if (is_valid_ether_addr(mac)) {
		netdev_info(priv->dev, "setting MAC %pM on VF %d\n",
			    mac, vf_id);
		dn200_uc_addr_get(priv, vf_id, mac_addr);
		if (memcmp(mac, mac_addr, ETH_ALEN) == 0) {
			netdev_warn(priv->dev,
				    "setting MAC and existed mac are the same\n");
			return 0;
		}
		dn200_uc_addr_set(priv, vf_id, mac);
		DN200_ITR_SYNC_SET(priv->hw, vf_reset_mac_list, vf_id, 1);
		irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
		return 0;
	}
	return -EINVAL;
}

/**
 * dn200_ndo_get_vf_config
 * @netdev: network interface device structure
 * @vf_id: VF identifier
 * @ivi: VF configuration structure
 *
 * return VF configuration
 **/
static int dn200_ndo_get_vf_config(struct net_device *netdev,
				   int vf_id, struct ifla_vf_info *ivi)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);
	u8 mac[ETH_ALEN];
	int ret = 0;
	u8 vf_link_notify = 0;

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			 test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}
	ivi->vf = vf_id;
	if (!priv->plat_ex->pf.registered_vfs) {
		netdev_warn(priv->dev, "VF not exist\n");
		return -EAGAIN;
	} else if (vf_id >= priv->plat_ex->pf.registered_vfs) {
		netdev_err(priv->dev, "Invalid VF Identifier %d\n", vf_id);
		return -EINVAL;
	}

	dn200_uc_addr_get(priv, vf_id, mac);
	ether_addr_copy(ivi->mac, mac);
	DN200_VF_LINK_GET(priv, vf_id, &vf_link_notify);
	if (priv->vf_link_forced[vf_id] == 0)
		ivi->linkstate = IFLA_VF_LINK_STATE_AUTO;
	else if (vf_link_notify && phy_info->link_status)
		ivi->linkstate = IFLA_VF_LINK_STATE_ENABLE;
	else
		ivi->linkstate = IFLA_VF_LINK_STATE_DISABLE;
	return ret;
}

static void dn200_link_notify(struct dn200_priv *priv, int vf_id,
			      bool up_notify)
{
	if (up_notify)
		DN200_ITR_SYNC_SET(priv->hw, vf_link_list, vf_id, LINK_UP_SET);
	else
		DN200_ITR_SYNC_SET(priv->hw, vf_link_list, vf_id,
				   LINK_DOWN_SET);
	irq_peer_notify(priv->plat_ex->pdev, &priv->plat_ex->ctrl);
}

/**
 * stmamc_ndo_set_vf_link_state
 * @netdev: network interface device structure
 * @vf_id: VF identifier
 * @link: required link state
 *
 * Set the link state of a specified VF, regardless of physical link state
 **/
static int dn200_ndo_set_vf_link_state(struct net_device *netdev, int vf_id,
				       int link)
{
	struct dn200_priv *priv = netdev_priv(netdev);
	struct dn200_phy_info *phy_info = PRIV_PHY_INFO(priv);

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return -EIO;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return -EIO;
	}

	if (!priv->plat_ex->pf.registered_vfs) {
		netdev_warn(priv->dev, "VF not exist\n");
		return -EAGAIN;
	} else if (vf_id >= priv->plat_ex->pf.registered_vfs) {
		netdev_err(priv->dev, "Invalid VF Identifier %d\n", vf_id);
		return -EINVAL;
	}
	switch (link) {
	case IFLA_VF_LINK_STATE_AUTO:
		priv->vf_link_forced[vf_id] = false;
		break;
	case IFLA_VF_LINK_STATE_ENABLE:
		priv->vf_link_forced[vf_id] = true;
		if (!phy_info->link_status) {
			netdev_warn(priv->dev,
				    "vf%d can't be up for PF is not link up\n",
				    vf_id);
			break;
		}
		dn200_link_notify(priv, vf_id, true);
		break;
	case IFLA_VF_LINK_STATE_DISABLE:
		priv->vf_link_forced[vf_id] = true;
		dn200_link_notify(priv, vf_id, false);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct net_device_ops dn200_netdev_ops = {
	.ndo_open = dn200_open,
	.ndo_start_xmit = dn200_xmit,
	.ndo_stop = dn200_release,
	.ndo_change_mtu = dn200_change_mtu,
	.ndo_fix_features = dn200_fix_features,
	.ndo_set_features = dn200_set_features,
	.ndo_set_rx_mode = dn200_set_rx_mode,
	.ndo_tx_timeout = dn200_tx_timeout,
	.ndo_features_check = dn200_features_check,
	.ndo_eth_ioctl = dn200_ioctl,
	.ndo_select_queue = dn200_select_queue,
	.ndo_poll_controller = dn200_poll_controller,
	.ndo_set_mac_address = dn200_set_mac_address,
	.ndo_vlan_rx_add_vid = dn200_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = dn200_vlan_rx_kill_vid,
	.ndo_get_stats64 = dn200_get_stats64,
	.ndo_set_vf_mac = dn200_ndo_set_vf_mac,
	.ndo_get_vf_config = dn200_ndo_get_vf_config,
	.ndo_set_vf_link_state = dn200_ndo_set_vf_link_state,
};

static const struct net_device_ops dn200_vf_netdev_ops = {
	.ndo_open = dn200_open,
	.ndo_start_xmit = dn200_xmit,
	.ndo_stop = dn200_release,
	.ndo_change_mtu = dn200_change_mtu,
	.ndo_fix_features = dn200_fix_features,
	.ndo_set_features = dn200_set_features,
	.ndo_set_rx_mode = dn200_set_rx_mode,
	.ndo_features_check = dn200_features_check,
	.ndo_eth_ioctl = dn200_ioctl,
	.ndo_select_queue = dn200_select_queue,
	.ndo_poll_controller = dn200_poll_controller,
	.ndo_set_mac_address = dn200_set_mac_address,
	.ndo_get_stats64 = dn200_get_stats64,
};

static void dn200_stop_open_subtask(struct dn200_priv *priv)
{
	int retry = 0;

	if (test_bit(DN200_IN_REMOVE, &priv->state))
		goto result;

	if (test_bit(DN200_DEV_ERR_CLOSE, &priv->state) ||
				test_bit(DN200_PF_NORMAL_CLOSE, &priv->state)) {
		if (test_bit(DN200_DOWN, &priv->state))
			goto result;
		while (test_and_set_bit(DN200_RESETING, &priv->state)) {
			usleep_range(1000, 2000);
			if (retry++ >= 3)
				return;
		};
		rtnl_lock();
		dev_close(priv->dev);
		rtnl_unlock();
		clear_bit(DN200_RESETING, &priv->state);
	}

	if (test_bit(DN200_PF_NORMAL_OPEN, &priv->state)) {
		rtnl_lock();
		dev_open(priv->dev, NULL);
		rtnl_unlock();
	}
result:
	if (test_bit(DN200_PF_NORMAL_OPEN, &priv->state) ||
		test_bit(DN200_PF_NORMAL_CLOSE, &priv->state)) {
		clear_bit(DN200_PF_FLOW_NORMAL_SET, &priv->state);

		if (test_bit(DN200_PF_NORMAL_OPEN, &priv->state))
			clear_bit(DN200_PF_NORMAL_OPEN, &priv->state);

		if (test_bit(DN200_PF_NORMAL_CLOSE, &priv->state))
			clear_bit(DN200_PF_NORMAL_CLOSE, &priv->state);
	}
}

static void dn200_reset_subtask(struct dn200_priv *priv)
{
	int retry = 0;
	u8 states;

	if (test_bit(DN200_DOWN, &priv->state))
		return;
	if (test_bit(DN200_IN_REMOVE, &priv->state))
		return;

	DN200_GET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, &states);
	if (!states)
		return;

	netif_trans_update(priv->dev);
	while (test_and_set_bit(DN200_RESETING, &priv->state)) {
		usleep_range(1000, 2000);
		if (retry++ >= 3)
			return;
	}

	if (test_bit(DN200_ERR_RESET, &priv->state)) {
		/* sriov pf: when tx timeout or occur dma channel err in open,
		 * will reset hw to recover
		 * pure pf: just reset hw in netdev open
		 * vf: don't run this task and reset hw,
		 * will notify pf to do it when occur dma channel err or tx timeout
		 */
		if (netif_running(priv->dev)) {
			rtnl_lock();
			dev_close(priv->dev);
			rtnl_unlock();

			rtnl_lock();
			dev_open(priv->dev, NULL);
			priv->dev->netdev_ops->ndo_set_rx_mode(priv->dev);
			rtnl_unlock();
		}
		clear_bit(DN200_ERR_RESET, &priv->state);
	}

	clear_bit(DN200_RESETING, &priv->state);
}

static void dn200_service_task(struct work_struct *work)
{
	struct dn200_priv *priv = container_of(work, struct dn200_priv,
					       service_task);

	if (test_bit(DN200_DEV_ERR_CLOSE, &priv->state) ||
		test_bit(DN200_PF_NORMAL_CLOSE, &priv->state) ||
			test_bit(DN200_PF_NORMAL_OPEN, &priv->state))
		dn200_stop_open_subtask(priv);
	else if (test_bit(DN200_ERR_RESET, &priv->state))
		dn200_reset_subtask(priv);
	else if (test_bit(DN200_MAC_LINK_DOWN, &priv->state))
		dn200_wq_mac_link_down(priv);
	if (test_bit(DN200_VF_FLOW_OPEN, &priv->state))
		dn200_vf_flow_open(priv);


}

/**
 *  dn200_hw_init - Init the MAC device
 *  @priv: driver private structure
 *  Description: this function is to configure the MAC device according to
 *  some platform parameters or the HW capability register. It prepares the
 *  driver to use either ring or chain modes and to setup either enhanced or
 *  normal descriptors.
 */
static int dn200_hw_init(struct dn200_priv *priv)
{
	int ret;
	bool is_locked = true;

	/* Initialize HW Interface */
	ret = dn200_hwif_init(priv);
	if (ret)
		return ret;

	/* Get the HW capability (new GMAC newer than 3.50a) */
	priv->hw_cap_support = dn200_check_hw_features_support(priv);
	if (priv->hw_cap_support) {
		/* We can override some gmac/dma configuration fields: e.g.
		 * enh_desc, tx_coe (e.g. that are passed through the
		 * platform) with the values from the HW capability
		 * register (if supported).
		 */
		priv->plat->enh_desc = priv->dma_cap.enh_desc;
		if (priv->dma_cap.hash_tb_sz) {
			priv->hw->multicast_filter_bins =
			    (BIT(priv->dma_cap.hash_tb_sz) << 5);
			priv->hw->mcast_bits_log2 =
			    ilog2(priv->hw->multicast_filter_bins);
		}

		/* TXCOE doesn't work in thresh DMA mode */
		priv->plat->tx_coe = priv->dma_cap.tx_coe;

		/* In case of GMAC4 rx_coe is from HW cap register. */
		priv->plat->rx_coe = priv->dma_cap.rx_coe;

		if (priv->dma_cap.rx_coe_type2)
			priv->plat->rx_coe = DN200_RX_COE_TYPE2;
		else if (priv->dma_cap.rx_coe_type1)
			priv->plat->rx_coe = DN200_RX_COE_TYPE1;
	} else {
		dev_info(priv->device,
			 "No HW DMA feature register supported\n");
	}

	if (priv->plat->rx_coe) {
		priv->hw->rx_csum = priv->plat->rx_coe;
		if (netif_msg_probe(priv))
			dev_info(priv->device,
				 "RX Checksum Offload Engine supported\n");
		if (priv->chip_id < DWMAC_CORE_4_00)
			if (netif_msg_probe(priv))
				dev_info(priv->device, "COE Type %d\n",
					 priv->hw->rx_csum);
	}
	if (priv->plat->tx_coe)
		if (netif_msg_probe(priv))
			dev_info(priv->device,
				 "TX Checksum insertion supported\n");

	priv->hw->vlan_fail_q_en = priv->plat->vlan_fail_q_en;
	priv->hw->vlan_fail_q = priv->plat->vlan_fail_q;

	/* Run HW quirks, if any */
	if (priv->hwif_quirks) {
		ret = priv->hwif_quirks(priv);
		if (ret)
			return ret;
	}

	/* Rx Watchdog is available in the COREs newer than the 3.40.
	 * In some case, for example on bugged HW this feature
	 * has to be disable and this can be done by passing the
	 * riwt_off field from the platform.
	 */
	if ((priv->chip_id >= DWMAC_CORE_3_50 || priv->plat->has_xgmac) &&
	    !priv->plat->riwt_off) {
		priv->use_riwt = 1;
	}

	if (PRIV_SRIOV_SUPPORT(priv) && !PRIV_IS_VF(priv)) {
		dn200_sriov_mail_init(priv);
		/* clear fw lock state and lram lock info */
		dn200_hw_unlock(priv->hw, &is_locked);
	}
	return 0;
}

static void dn200_napi_add(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 queue, maxq;
	u8 rx_num = priv->plat_ex->default_rx_queue_num;
	u8 tx_num = priv->plat_ex->default_tx_queue_num;

	if (PRIV_IS_PUREPF(priv)) {
		rx_num = priv->plat->rx_queues_to_use;
		tx_num = priv->plat->tx_queues_to_use;
	}
	maxq = max(rx_num, tx_num);

	for (queue = 0; queue < maxq; queue++) {
		struct dn200_channel *ch = &priv->channel[queue];

		ch->priv_data = priv;
		ch->index = queue;
		ch->in_sch = false;
		spin_lock_init(&ch->lock);

		if (queue < rx_num && queue < tx_num &&
					 priv->txrx_itr_combined) {
			netif_napi_add(dev, &ch->agg_napi, dn200_napi_poll_agg);
		} else {
			if (queue < rx_num) {
				netif_napi_add(dev, &ch->rx_napi,
					       dn200_napi_poll_rx);
			}
			if (queue < tx_num) {
				netif_napi_add(dev, &ch->tx_napi,
					       dn200_napi_poll_tx);
			}
		}
	}
}

static void dn200_napi_del(struct net_device *dev)
{
	struct dn200_priv *priv = netdev_priv(dev);
	u32 queue, maxq;
	u8 rx_num = priv->plat_ex->default_rx_queue_num;
	u8 tx_num = priv->plat_ex->default_tx_queue_num;

	if (PRIV_IS_PUREPF(priv)) {
		rx_num = priv->plat->rx_queues_to_use;
		tx_num = priv->plat->tx_queues_to_use;
	}
	maxq = max(rx_num, tx_num);

	for (queue = 0; queue < maxq; queue++) {
		struct dn200_channel *ch = &priv->channel[queue];

		if (queue < rx_num &&
			 queue < tx_num &&
			 priv->txrx_itr_combined) {
			netif_napi_del(&ch->agg_napi);
		} else {
			if (queue < rx_num)
				netif_napi_del(&ch->rx_napi);
			if (queue < tx_num)
				netif_napi_del(&ch->tx_napi);
		}
	}
}

/**
 * dn200_disable_all_queues - Disable all queues
 * @priv: driver private structure
 */
static void dn200_disable_all_queues(struct dn200_priv *priv)
{
	/* at first disable napi and then delete it */
	__dn200_disable_all_queues(priv);
}

/**
 * dn200_enable_all_queues - Enable all queues
 * @priv: driver private structure
 */
void dn200_enable_all_queues(struct dn200_priv *priv)
{
	u32 rx_queues_cnt = priv->plat->rx_queues_to_use;
	u32 tx_queues_cnt = priv->plat->tx_queues_to_use;
	u32 maxq = max(rx_queues_cnt, tx_queues_cnt);
	u32 queue;

	/* add napi to netdev poll list and then enable it */
	for (queue = 0; queue < maxq; queue++) {
		struct dn200_channel *ch = &priv->channel[queue];

		if (queue < rx_queues_cnt && queue < tx_queues_cnt &&
			 priv->txrx_itr_combined) {
			napi_enable(&ch->agg_napi);
		} else {
			if (queue < rx_queues_cnt)
				napi_enable(&ch->rx_napi);
			if (queue < tx_queues_cnt)
				napi_enable(&ch->tx_napi);
		}
	}
}

static bool dn200_tx_queue_clean(struct dn200_priv *priv, u8 queue_index)
{
	unsigned long start_time, end_time;
	u32 value;
	bool is_in_task = !!test_bit(DN200_IN_TASK, &priv->state);

	queue_index += DN200_TXQ_START_GET(priv->hw);
	/* max wait 500ms to flush tx queue */
#define MAX_FLUSH_TIME_NS (500 * 1000 * 1000)
	start_time = ktime_get_ns();
	while (true) {
		value = readl(priv->ioaddr + XGMAC_CH_DESC_CACHE_LVL(queue_index)) & XGMAC_TXLVL;
		if (value == 0x0)
			break;
		if (is_in_task)
			usleep_range(100, 200);
		else
			udelay(10);
		end_time = ktime_get_ns();
		if ((end_time - start_time) > MAX_FLUSH_TIME_NS) {
			netif_info(priv, ifdown, priv->dev,
				 "TXQ:%d exceed max time:%u cache value = 0x%x\n",
				 queue_index, MAX_FLUSH_TIME_NS, value);
			return false;
		}
	}

	while (true) {
		value = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(queue_index)) & 0xffff;
		if (value == 0x0 || value == 0x100)
			break;
		if (is_in_task)
			usleep_range(100, 200);
		else
			udelay(10);
		end_time = ktime_get_ns();
		if ((end_time - start_time) > MAX_FLUSH_TIME_NS) {
			netif_info(priv, ifdown, priv->dev,
				 "TXQ:%d exceed max time:%u dbg_status = 0x%x\n",
				 queue_index, MAX_FLUSH_TIME_NS, value);
			return false;
		}
	}

	return true;
}

int dn200_clean_all_tx_queues(struct dn200_priv *priv, u8 tx_queue_num)
{
	int i = 0;
	bool clean_succ = false;
	u8 flow_state;

	netif_tx_disable(priv->dev);
	netif_carrier_off(priv->dev);
	if (PRIV_IS_VF(priv)) {
		DN200_ITR_SYNC_GET(priv->hw, vf_flow_state_event,
				DN200_VF_OFFSET_GET(priv->hw), &flow_state);
		if (flow_state == FLOW_CLOSE_START)
			return 0;
		if (test_bit(DN200_VF_FLOW_CLOSE, &priv->state))
			return 0;
	}
	/* vf do not check tx queue when pf notify vf to sw reset*/
	for (; i < tx_queue_num; i++) {
		clean_succ = dn200_tx_queue_clean(priv, i);
		if (!clean_succ)
			return -EBUSY;
	}
	return 0;
}

static bool dn200_rx_queue_clean(struct dn200_priv *priv, u8 queue_index)
{
	u32 ch_dbg_st = 0;
	u32 rxdma_fsm_st = 0;
	u32 mtl_rxq_dbg = 0;
	u32 mac_dbg = 0, mac_rx_dbg = 0;
	unsigned long start_time, end_time;

#define RX_MAX_FLUSH_TIME_NS (10 * 1000 * 1000) /* 10ms */
	start_time = ktime_get_ns();
	while (true) {
		ch_dbg_st = readl(priv->ioaddr + XGMAC_CH_DEBUG_ST(queue_index));
		rxdma_fsm_st = ch_dbg_st & XGMAC_RXDMA_FSM_STATE_MASK;

		mtl_rxq_dbg = readl(priv->ioaddr + XGMAC_MTL_RXQ_DEBUG(queue_index));
		mac_dbg = readl(priv->ioaddr + XGMAC_MAC_DEBUG);
		mac_rx_dbg = mac_dbg & XGMAC_MAC_RX_FIFO_ACT;

		if ((rxdma_fsm_st == XGMAC_RXDMA_FSM_STATE || rxdma_fsm_st == 0)
			&& mtl_rxq_dbg == 0
			&& mac_rx_dbg == 0)
			break;

		end_time = ktime_get_ns();
		if ((end_time - start_time) > RX_MAX_FLUSH_TIME_NS) {
			netdev_info(priv->dev,
				 "RXQ:%d exceed max flush time:%u, ch_dbg_st:%#x, mtl_rxq_dbg:%#x, mac_rx_dbg:%#x\n",
				 queue_index, RX_MAX_FLUSH_TIME_NS, ch_dbg_st, mtl_rxq_dbg, mac_rx_dbg);
			return false;
		}
	}
	return true;
}

int dn200_clean_all_rx_queues(struct dn200_priv *priv)
{
	int queue = 0;
	bool ret = true;

	for (queue = 0; queue <= DN200_LAST_QUEUE(priv); queue++) {
		ret = dn200_rx_queue_clean(priv, queue);
		if (!ret)
			return -EBUSY;
	}
	return 0;
}

static int dn200_sw_resc_reinit(struct dn200_priv *priv, bool rxp_clean);
static int dn200_sw_resc_close(struct dn200_priv *priv);
int dn200_reinit_hwts(struct net_device *dev, bool initial, u32 new_flags)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret = 0;
	bool need_update;
	bool need_hw_reset = false;
	int rx_state = 0;

	rx_state = dn200_mac_rx_get(priv, priv->ioaddr);
	need_update = netif_running(dev);
	if (test_and_set_bit(DN200_NET_SUSPENDED, &priv->state))
		return -EINVAL;

	netif_trans_update(priv->dev);
	if (need_update) {
		ret = dn200_sw_resc_close(priv);
		if (ret < 0) {
			need_hw_reset = true;
			dev_close(dev);
		}
	}

	priv->eth_priv_flags = new_flags;
	if (initial) {
		priv->hwts_rx_en = 1;
		priv->hwts_tx_en = 1;
	} else {
		priv->hwts_rx_en = 0;
		priv->hwts_tx_en = 0;
	}
	if (need_update) {
		if (need_hw_reset)
			ret = dev_open(dev, NULL);
		else
			ret = dn200_sw_resc_reinit(priv, false);
	}
	/* enable mac rx engine state to deal with vf down but pf up */
	if (rx_state)
		dn200_mac_rx_set(priv, priv->ioaddr, true);
	clear_bit(DN200_NET_SUSPENDED, &priv->state);

	return ret;
}

int dn200_reinit_queues(struct net_device *dev, u32 rx_cnt, u32 tx_cnt)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret = 0;
	int i = 0;
	bool need_update;

	need_update = netif_running(dev);
	if (test_and_set_bit(DN200_NET_SUSPENDED, &priv->state))
		return -EINVAL;

	netif_trans_update(priv->dev);
	if (need_update)
		dev_close(dev);

	priv->plat->rx_queues_to_use = rx_cnt;
	priv->plat->tx_queues_to_use = tx_cnt;
	if (!PRIV_IS_VF(priv)) {
		for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
			priv->rss.table[i] =
			    ethtool_rxfh_indir_default(i, rx_cnt);
	}
	dn200_rss_configure(priv, priv->hw, &priv->rss,
			   rx_cnt);

	if (priv->dma_cap.rmon)
		memset(&priv->mmc, 0, sizeof(struct dn200_counters));

	memset(&priv->xstats, 0, sizeof(struct dn200_extra_stats));

	if (need_update)
		ret = dev_open(dev, NULL);

	clear_bit(DN200_NET_SUSPENDED, &priv->state);
	return ret;
}

static bool dn200_all_rx_queue_fc_act_check(struct dn200_priv *priv)
{
	u8 count = priv->plat_ex->rx_queues_total;
	int i = 0;
	u32 debug_val = 0;
	u32 fc_status = 0;

	for (; i < count; i++) {
		debug_val = readl(priv->ioaddr + XGMAC_MTL_RXQ_DEBUG(i));
		fc_status = (debug_val & XGMAC_RXQSTS) >> XGMAC_RXQSTS_SHIFT;
		if (fc_status == XGMAC_FC_OVER_TH || fc_status == XGMAC_FC_QUEUE_FULL)
			return true;
	}
	return false;
}

int dn200_vf_flow_state_process(struct dn200_priv *priv)
{
	int ret = 0;
	u8 flow_state = 0;
	struct net_device *dev = priv->dev;

	if (!PRIV_IS_VF(priv))
		return ret;

	set_bit(DN200_IN_TASK, &priv->state);
	DN200_ITR_SYNC_GET(priv->hw, vf_flow_state_event,
			   DN200_VF_OFFSET_GET(priv->hw), &flow_state);

	if (test_bit(DN200_VF_FLOW_CLOSE, &priv->state)) {
		/*stop flow and free tx/rx related sw resource */
		if (netif_running(dev)) {
			dn200_sw_resc_close(priv);
			dev_dbg(priv->device,
				"%s, %d, vf funcid:%#x, vf offset:%d have been close cpu_id = %d\n",
				__func__, __LINE__, priv->plat_ex->funcid,
				DN200_VF_OFFSET_GET(priv->hw), smp_processor_id());
		}
		clear_bit(DN200_VF_FLOW_CLOSE, &priv->state);
		DN200_VF_UPGRADE_SET(priv, priv->plat_ex->vf_offset, BIT(0));
	}

	if (test_bit(DN200_VF_FLOW_OPEN_SET, &priv->state)) {
		if (netif_running(dev)) {
			dn200_sw_resc_reinit(priv, true);
			dev_dbg(priv->device,
				"%s, %d, vf funcid:%#x, vf offset:%d, have been open.\n",
				__func__, __LINE__, priv->plat_ex->funcid,
				DN200_VF_OFFSET_GET(priv->hw));
		}
		clear_bit(DN200_VF_FLOW_OPEN_SET, &priv->state);
		DN200_VF_UPGRADE_SET(priv, priv->plat_ex->vf_offset, BIT(0));
	}
	clear_bit(DN200_IN_TASK, &priv->state);
	return ret;
}

static int dn200_sw_resc_close(struct dn200_priv *priv)
{
	int ret = 0;
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 i;
	u32 chan = 0;

	if (!netif_running(priv->dev))
		return 0;

	if (test_bit(DN200_DOWN, &priv->state))
		return 0;
	/* set vf in stop to forbid get hw lock to set rxp */
	if (PRIV_IS_VF(priv))
		set_bit(DN200_VF_IN_STOP, &priv->state);
	netif_tx_disable(priv->dev);
	netif_carrier_off(priv->dev);
	for (i = 0; i < tx_count; i++) {
		if (priv->tx_queue[i].txtimer.function)
			hrtimer_cancel(&priv->tx_queue[i].txtimer);
		if (priv->tx_queue[i].tx_task.func)
			cancel_work_sync(&priv->tx_queue[i].tx_task);
		if (priv->tx_queue[chan].poll_txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].poll_txtimer);
		memset(&priv->tx_queue[chan].poll_txtimer, 0,
		       sizeof(struct hrtimer));
		if (priv->tx_queue[chan].poll_tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].poll_tx_task);
	}
	for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
		if (priv->rx_queue[chan].poll_rxtimer.function)
			hrtimer_cancel(&priv->rx_queue[chan].poll_rxtimer);
		memset(&priv->rx_queue[chan].poll_rxtimer, 0,
		       sizeof(struct hrtimer));
		if (priv->rx_queue[chan].poll_rx_task.func)
			cancel_work_sync(&priv->rx_queue[chan].poll_rx_task);
	}
	if (!PRIV_IS_VF(priv))
		dn200_flow_ctrl(priv, priv->hw, false, FLOW_OFF,
				 0, priv->plat->tx_queues_to_use);
	ret = dn200_datapath_close(priv);

	if (ret < 0)
		return ret;
	if (test_and_set_bit(DN200_DOWN, &priv->state))
		return 0;
	priv->vf_sw_close_flag = true;
	dn200_stop_all_dma(priv);

	dn200_disable_all_queues(priv);
	udelay(10);
	dn200_free_irq(priv->dev, REQ_IRQ_ERR_ALL, 0);
	if (PRIV_IS_PUREPF(priv))
		dn200_napi_del(priv->dev);
	/*reclaim tx resource*/
	for (i = 0; i < tx_count; i++) {
		if (priv->tx_queue[i].cur_tx != priv->tx_queue[i].dirty_tx)
			dn200_sw_tx_clean(priv, 64, i);
	}
	free_dma_desc_resources(priv);

	return 0;
}

static int dn200_sw_resc_reinit(struct dn200_priv *priv, bool rxp_clean)
{
	struct dn200_phy_info *phy_info = priv->plat_ex->phy_info;
	int ret = 0;
	int chan = 0;

	if (!netif_running(priv->dev))
		return 0;
	if (!test_bit(DN200_DOWN, &priv->state))
		return 0;
	priv->vf_sw_close_flag = false;
	ret = dn200_iatu_init(priv);
	if (ret)
		return ret;
	ret = alloc_dma_desc_resources(priv);
	if (ret) {
		netdev_err(priv->dev, "%s: DMA descriptors allocation failed\n",
					__func__);
		goto dma_desc_error;
	}
	ret = init_dma_desc_rings(priv->dev, GFP_KERNEL);
	if (ret) {
		netdev_err(priv->dev,
					"%s: DMA descriptors initialization failed\n",
					__func__);
		goto init_error;
	}
	dn200_add_rx_iatu2tx(priv);
	if (PRIV_IS_VF(priv))
		clear_bit(DN200_VF_IN_STOP, &priv->state);
	ret = dn200_hw_setup(priv->dev, false);
	if (ret) {
		netdev_err(priv->dev, "%s: Hw setup failed\n", __func__);
		goto init_error;
	}
	dn200_init_coalesce(priv);
	ret = dn200_request_irq(priv->dev);
	if (ret)
		goto irq_error;
	if (PRIV_IS_PUREPF(priv))
		dn200_napi_add(priv->dev);
	dn200_enable_all_queues(priv);
	dn200_enable_all_dma_irq(priv);
	if (rxp_clean)
		dn200_eth_reconfig(priv);
	netif_tx_start_all_queues(priv->dev);
	if (PRIV_IS_VF(priv))
		dn200_start_all_dma(priv);
	if (priv->plat_ex->phy_info->link_status) {
		if (!PRIV_IS_VF(priv))
			dn200_wq_mac_link_down(priv);
		phy_info->mac_ops->mac_link_up(priv, phy_info->phydev, 0,
						phy_info->phy_interface,
						phy_info->speed, phy_info->dup,
						0, 0);
		netif_carrier_on(priv->dev);
		linkwatch_fire_event(priv->dev);
	}
	clear_bit(DN200_DOWN, &priv->state);
	return 0;
irq_error:
	for (chan = 0; chan < priv->plat->tx_queues_to_use; chan++) {
		if (priv->tx_queue[chan].tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].tx_task);
		if (priv->tx_queue[chan].txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].txtimer);
		memset(&priv->tx_queue[chan].txtimer, 0,
		       sizeof(struct hrtimer));
		if (priv->tx_queue[chan].poll_tx_task.func)
			cancel_work_sync(&priv->tx_queue[chan].poll_tx_task);
		if (priv->tx_queue[chan].poll_txtimer.function)
			hrtimer_cancel(&priv->tx_queue[chan].poll_txtimer);
		memset(&priv->tx_queue[chan].poll_txtimer, 0,
		       sizeof(struct hrtimer));
	}
	for (chan = 0; chan < priv->plat->rx_queues_to_use; chan++) {
		if (priv->rx_queue[chan].poll_rx_task.func)
			cancel_work_sync(&priv->rx_queue[chan].poll_rx_task);
		if (priv->rx_queue[chan].poll_rxtimer.function)
			hrtimer_cancel(&priv->rx_queue[chan].poll_rxtimer);
		memset(&priv->rx_queue[chan].poll_rxtimer, 0,
		       sizeof(struct hrtimer));
	}
	dn200_hw_teardown(priv->dev);
init_error:
	free_dma_desc_resources(priv);
dma_desc_error:
	return ret;
}

static void dn200_retask(struct work_struct *work)
{
	struct dn200_priv *priv;

	priv = container_of(work, struct dn200_priv, retask);
	ctrl_reset(&priv->plat_ex->ctrl, true);

	if (priv->mii)
		dn200_xgmac_clock_ctl(priv);
	dn200_resume(priv->device);
}

static void dn200_reconfig_task(struct work_struct *work)
{
	struct dn200_priv *priv;

	priv = container_of(work, struct dn200_priv, reconfig_task);
	dn200_eth_reconfig(priv);
}

static void dn200_vf_process_task(struct work_struct *work)
{
	struct dn200_priv *priv = container_of(work, struct dn200_priv,
					       vf_process_task);

	if (test_bit(DN200_VF_FLOW_STATE_SET, &priv->state)) {
		dn200_vf_flow_state_process(priv);
		clear_bit(DN200_VF_FLOW_STATE_SET, &priv->state);
	}

	if (test_bit(DN200_VF_NOTIFY_PF_RESET, &priv->state)) {
		dn200_vf_glb_err_rst_notify(priv);
		clear_bit(DN200_VF_NOTIFY_PF_RESET, &priv->state);
	}
}

static void dn200_tx_reset(struct timer_list *t)
{
	struct dn200_priv *priv = from_timer(priv, t, reset_timer);

	/* tx abnormal should call global err processing to reset hw */
	dn200_global_err(priv, DN200_TX_RESET);
}

static void dn200_reset_vf_rxp(struct dn200_priv *priv, u8 vf_offset)
{
	priv->pf_rxp_set |= RXP_CLEAR_VF_RXP;
	priv->clear_vf_rxp_bitmap |= (1 << vf_offset);
	if (PRIV_SRIOV_SUPPORT(priv) && !test_bit(DN200_RXP_SETTING, &priv->state))
		queue_work(priv->wq, &priv->rxp_task);
	DN200_HEARTBEAT_SET(priv->hw, registered_vf_state, vf_offset,
			    DN200_VF_REG_STATE_NONE);
	priv->plat_ex->vf_loss_hb_cnt[vf_offset] = 0;
}

static void dn200_check_vf_alive(struct dn200_priv *priv)
{
	u32 vf_num;
	u8 last_beat, reg_info, beat;

	for (vf_num = 0; vf_num < priv->plat_ex->pf.registered_vfs; vf_num++) {
		DN200_HEARTBEAT_GET(priv->hw, registered_vf_state, vf_num,
				    &reg_info);
		if (!(reg_info & DN200_VF_REG_STATE_OPENED)) {
			priv->plat_ex->vf_loss_hb_cnt[vf_num] = 0;
			/*vf not register now, skip it */
			continue;
		}
		DN200_HEARTBEAT_GET(priv->hw, heartbeat, vf_num, &beat);
		DN200_HEARTBEAT_GET(priv->hw, last_heartbeat, vf_num,
				    &last_beat);
		if (beat == last_beat) {
			(priv->plat_ex->vf_loss_hb_cnt[vf_num])++;
			if (priv->plat_ex->vf_loss_hb_cnt[vf_num] >
			    DN200_MAX_VF_LOSS_HB_CNT) {
				netdev_warn(priv->dev,
					    "%s vf %d register but not keep heartbeat beat %d last_beat %d\n",
					    __func__, vf_num, beat, last_beat);
				dn200_reset_vf_rxp(priv, vf_num);
				dn200_stop_vf_dma(priv, vf_num + 1);
				if (dn200_all_rx_queue_fc_act_check(priv)) {
					dn200_global_err(priv, DN200_FC_VF_STOP);
					return;
				}
			}
			continue;
		}
		/*VF has returned to normal, clear loss cnt */
		priv->plat_ex->vf_loss_hb_cnt[vf_num] = 0;
		last_beat = beat;
		DN200_HEARTBEAT_SET(priv->hw, last_heartbeat, vf_num,
				    last_beat);
	}
}

static void dn200_vf_tx_clean_ck(struct dn200_priv *priv)
{
	u32 tx_count = priv->plat->tx_queues_to_use;
	u32 queue;

	if (test_bit(DN200_DOWN, &priv->state) || !netif_running(priv->dev))
		return;
	for (queue = 0; queue < tx_count; queue++) {
		struct dn200_tx_queue *tx_q = &priv->tx_queue[queue];

		if (tx_q->cur_tx != tx_q->dirty_tx && (tx_q->task_need_sch || tx_q->txtimer_need_sch)) {
			tx_q->old_dirty_tx = tx_q->dirty_tx;
			if (tx_q->old_dirty_tx == tx_q->dirty_tx) {
				netdev_dbg(priv->dev,
						"%s queue %d tx_clean need resch:cur_tx %d dirty %d timer %d\n",
						__func__, queue, tx_q->cur_tx, tx_q->dirty_tx,
						atomic_read(&tx_q->txtimer_running));
				netdev_dbg(priv->dev, "task_status %d time_status %d\n",
					tx_q->task_need_sch, tx_q->txtimer_need_sch);
				dn200_tx_timer_arm(priv, queue);
			}
		}
	}
}

static void dn200_vf_heartbeat(struct dn200_priv *priv)
{
	u8 last_beat, reg_info, beat, states;

	DN200_GET_LRAM_MAILBOX_MEMBER(priv->hw, pf_fw_err_states, &states);
	if (states) {
		if (!test_bit(DN200_DEV_ERR_CLOSE, &priv->state))
			dn200_fw_err_dev_close(priv);
		return;
	}

	DN200_HEARTBEAT_GET(priv->hw, registered_vf_state,
			    priv->plat_ex->vf_offset, &reg_info);
	DN200_GET_LRAM_MAILBOX_MEMBER(priv->hw, pf_states, &states);
	if (reg_info == 0 && states != 0) {
		/*vf self has been clear rxp info by PF, reset vf self */
		dn200_normal_reset(priv);
		return;
	}

	DN200_HEARTBEAT_GET(priv->hw, last_heartbeat, priv->plat_ex->vf_offset,
			    &last_beat);

	beat = !last_beat;
	netdev_dbg(priv->dev, "%s vf %d last_beat %d set beat to %d\n",
		   __func__, priv->plat_ex->vf_offset, last_beat, beat);

	DN200_HEARTBEAT_SET(priv->hw, heartbeat, priv->plat_ex->vf_offset,
			    beat);
}

static int dn200_parse_fw_commit_err(struct device *dev, int nic_st, int status)
{
	int i;
	struct fw_cmt_err {
		int nic_st;
		int status;
		char *info;
	} err_info[] = {
		{1, 1, "the NIC fw upgrade fail"},
		{1, 2, "the NIC fw uncompress to mem fail"},
		// {2, 0, "the NIC fw prepare jump"},
		{3, 1, "the NIC vu or cli cmd is running"},
		{3, 2, "the NIC cli cmd unlock fail"},
		{3, 3, "the NIC wait lock timeout"},
		{3, 2, "the NIC jump to new fw fail"},
	};

	for (i = 0; i < ARRAY_SIZE(err_info); i++) {
		if (err_info[i].nic_st == nic_st && err_info[i].status == status) {
			dev_err(dev, "[loading fw]%s [nic_st %d, status %d].\n",
				err_info[i].info, nic_st, status);
			return -EIO;
		}
	}
	return 0;
}

static void dn200_upgrade_timer(struct timer_list *t)
{
	struct dn200_priv *priv = from_timer(priv, t, upgrade_timer);
	u32 flags = 0;
	u32 status = 0;
	int i = 0;

	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return;
	}

	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, nic_st, &flags);
	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, rsv, &status);

	if (dn200_parse_fw_commit_err(priv->device, flags, status)) {
		clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		priv->update_fail = true;
	}
	if (flags == DN200_STOP_FLAG) {
		DN200_SET_LRAM_UPGRADE_PF(priv->hw, 1, priv->plat_ex->pf_id);
		if (!priv->upgrade_time && priv->flag_upgrade)
			netdev_info(priv->dev, "%s: start fw upgrade......\n", __func__);
		if (!priv->upgrade_time && !priv->flag_upgrade)
			netdev_info(priv->dev, "other's port now in upgrade, stop do anything,wait a while......\n");
		if (!test_bit(DN200_DOWN, &priv->state) && PRIV_PHY_INFO(priv)->link_status) {
			for (i = 0; i < priv->plat->rx_queues_to_use; i++)
				dn200_rx_timer_poll(priv, i);
			for (i = 0; i < priv->plat->tx_queues_to_use; i++)
				dn200_tx_timer_poll(priv, i);
		}
		priv->upgrade_time++;
		if (priv->upgrade_time > 2000) {  /*give 60s, it means sufficient*/
			priv->upgrade_time = 0;
			DN200_SET_LRAM_UPGRADE_PF(priv->hw, 1, priv->plat_ex->pf_id);
			clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
			return;
		}
		mod_timer(&priv->upgrade_timer,
			jiffies + msecs_to_jiffies(30));
		return;
	}

	if (flags == DN200_START_FLAG) {
		priv->upgrade_time = 0;
		if (!priv->flag_upgrade)
			netdev_info(priv->dev, "other's port upgrade success,you can do anything now......\n");
		clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		DN200_SET_LRAM_UPGRADE_PF_FINISH(priv->hw, 1, priv->plat_ex->pf_id);
		return;
	}

	if (flags == DN200_UNFINISH_FLAG)  {
		priv->upgrade_time = 0;
		if (priv->flag_upgrade)
			netdev_info(priv->dev, "[loading fw]dn200 load fw fail, for img copy to flash happened err.\n");
		if (!priv->flag_upgrade)
			netdev_info(priv->dev, "[loading fw]dn200 load fw fail, you can start other thing\n");
		clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		priv->update_fail = true;
		return;
	}

	if (flags == DN200_JMP_FAIL_FLAG) {
		priv->upgrade_time = 0;
		if (priv->flag_upgrade)
			netdev_info(priv->dev, "[loading fw]dn200 load fw fail, for jmp new img fail.\n");
		if (!priv->flag_upgrade)
			netdev_info(priv->dev, "[loading fw]dn200 load fw fail, you can start other thing\n");
		clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		priv->update_fail = true;
		return;
	}
	if (test_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state))
		mod_timer(&priv->upgrade_timer,
			jiffies + msecs_to_jiffies(100));

}

static void dn200_heartbeat(struct timer_list *t)
{
	struct dn200_priv *priv = from_timer(priv, t, keepalive_timer);

	if (test_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state)  ||
			test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		return;
	if (priv->dev && priv->dev->reg_state == NETREG_RELEASED)
		return;
	if (!dn200_hwif_id_check(priv->ioaddr)) {
		netdev_err(priv->dev, "%s: %s\n", __func__, DN200_PCIE_BAR_ERR);
		set_bit(ADMIN_QUEUE_CQ_NO_RETURN, &priv->plat_ex->ctrl.admin_state);
		set_bit(DN200_PCIE_UNAVAILD, &priv->state);
		dn200_global_err(priv, DN200_PCIE_UNAVAILD_ERR);
		return;
	}
	/* tx abnormal should call global err processing to reset hw */
	if (!netif_running(priv->dev)) {
		mod_timer(&priv->keepalive_timer,
			  jiffies + msecs_to_jiffies(1000));
		return;
	}

	if (PRIV_SRIOV_SUPPORT(priv)) {
		dn200_check_vf_alive(priv);
		if (!test_bit(DN200_RXP_SETTING, &priv->state) && test_bit(DN200_RXP_NEED_CHECK, &priv->state)) {
			queue_work(priv->wq, &priv->rxp_task);
			clear_bit(DN200_RXP_NEED_CHECK, &priv->state);
		}
	} else if (PRIV_IS_VF(priv)) {
		dn200_vf_heartbeat(priv);
		dn200_vf_tx_clean_ck(priv);
	}

	mod_timer(&priv->keepalive_timer, jiffies + msecs_to_jiffies(1000));
}

static u32 dn200_nextpow2(u32 value)
{
	value--;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	value++;
	return value;
}

int dn200_reinit_ringparam(struct net_device *dev, u32 rx_size, u32 tx_size)
{
	struct dn200_priv *priv = netdev_priv(dev);
	int ret = 0;
	int rx_chan = 0;
	bool need_update;
	bool need_hw_reset = false;
	int rx_state = 0;

	rx_state = dn200_mac_rx_get(priv, priv->ioaddr);
	need_update = netif_running(dev);
	if (test_and_set_bit(DN200_NET_SUSPENDED, &priv->state))
		return -EINVAL;

	netif_trans_update(priv->dev);
	if (need_update) {
		if (!HW_IS_PUREPF(priv->hw)) {
			ret = dn200_sw_resc_close(priv);
			if (ret < 0) {
				need_hw_reset = true;
				dev_close(dev);
			}
		} else {
			need_hw_reset = true;
			dev_close(dev);
		}
	}
	/*for rx_size and tx_size is impossileby 0 */
	priv->dma_rx_size =
	    dn200_nextpow2(rx_size) > DMA_MAX_RX_SIZE ? DMA_MAX_RX_SIZE : dn200_nextpow2(rx_size);
	priv->dma_tx_size =
	    dn200_nextpow2(tx_size) > DMA_MAX_TX_SIZE ? DMA_MAX_TX_SIZE : dn200_nextpow2(tx_size);
	if (priv->dma_rx_size < DMA_MIN_RX_SIZE ||
		 priv->dma_rx_size > DMA_MAX_RX_SIZE ||
		 priv->dma_tx_size < DMA_MIN_TX_SIZE ||
		 priv->dma_tx_size > DMA_MAX_TX_SIZE)
		WARN_ON(1);

	/* The watchdog is enabled after packet collection is complete.
	 * However, if the descriptor queue is small and the packet collection speed is fast,
	 * descriptors may be used up before the watchdog interrupt takes effect,
	 * and packet collection is blocked if no interruption is triggered.
	 * Therefore, it is necessary to use the packet receiving interrupt aggregation
	 * to avoid this situation,
	 * and ensure that rx-frams + 32 is smaller than rx_dma_size
	 */
	for (; rx_chan < priv->plat->rx_queues_to_use; rx_chan++) {
		if ((priv->rx_coal_frames[rx_chan] +
		     dn200_rx_refill_size(priv)) >= priv->dma_rx_size) {
			netdev_warn(dev, "change queue %d rx-frames to %d\n",
				    rx_chan, DN200_RX_MIN_FRAMES);
			priv->rx_coal_frames[rx_chan] = DN200_RX_MIN_FRAMES;
		}
	}
	if (need_update) {
		if (need_hw_reset)
			ret = dev_open(dev, NULL);
		else
			ret = dn200_sw_resc_reinit(priv, false);
	}
	if (rx_state)
		dn200_mac_rx_set(priv, priv->ioaddr, true);
	clear_bit(DN200_NET_SUSPENDED, &priv->state);
	/* base on new desc ring size to update rx interrupt usec */
	dn200_rx_itr_usec_update(priv);
	return ret;
}

static int dn200_hw_phy_info_init(struct dn200_priv *priv,
				  struct net_device *ndev)
{
	int ret = 0;

	dn200_check_pcs_mode(priv);

	if (priv->hw->pcs != DN200_PCS_TBI &&
	    priv->hw->pcs != DN200_PCS_RTBI && !priv->plat_ex->has_xpcs) {
		/* MDIO bus Registration */
		ret = dn200_mdio_register(ndev);
		if (ret < 0) {
			dev_err(priv->device,
				"%s: MDIO bus (id: %d) registration failed",
				__func__, priv->plat->bus_id);
			return ret;
		}
	}
	ret = dn200_phy_info_init(priv->dev, &dn200_phy_mac_ops);
	if (ret) {
		netdev_err(ndev, "failed to setup phy (%d)\n", ret);
		goto error_phy_setup;
	}
	return 0;

error_phy_setup:
	if (priv->hw->pcs != DN200_PCS_TBI && priv->hw->pcs != DN200_PCS_RTBI)
		dn200_mdio_unregister(ndev);
	return ret;
}

static int dn200_sw_phy_info_init(struct dn200_priv *priv,
				  struct net_device *ndev)
{
	dn200_phy_info_init(priv->dev, &dn200_vf_phy_mac_ops);
	return 0;
}

static int dn200_phyinfo_init(struct dn200_priv *priv, struct net_device *ndev)
{
	if (PRIV_IS_VF(priv))
		return dn200_sw_phy_info_init(priv, ndev);
	else
		return dn200_hw_phy_info_init(priv, ndev);
}

static int dn200_ntuple_fdir_init(struct dn200_priv *priv)
{
	struct dma_features *dma_cap = &priv->dma_cap;

	priv->flow_entries_max = dma_cap->l3l4fnum;
	priv->fdir_counts = 0;
	priv->fdir_enties = devm_kcalloc(priv->device,
					 dma_cap->l3l4fnum,
					 sizeof(struct dn200_fdir_filter),
					 GFP_KERNEL);
	if (!priv->fdir_enties)
		return -ENOMEM;
	memset(&priv->fdir_info, 0, sizeof(priv->fdir_info));
	return 0;
}

static void dn200_uc_addr_set(struct dn200_priv *priv, int vf_id, u8 *addr)
{
	int i = 0;

	for (i = 0; i < ETH_ALEN; i++)
		writeb(addr[i], LRAM_PRIV_MAC_VF_OFFSET(priv, vf_id, i));
}

static void dn200_uc_addr_get(struct dn200_priv *priv, int vf_id, u8 *addr)
{
	int i = 0;

	for (i = 0; i < ETH_ALEN; i++)
		addr[i] = readb(LRAM_PRIV_MAC_VF_OFFSET(priv, vf_id, i));
}

void dn200_vf_mac_change(struct dn200_priv *priv)
{
	u8 mac[ETH_ALEN];

	dn200_uc_addr_get(priv, priv->plat_ex->vf_offset, mac);
	eth_hw_addr_set(priv->dev, mac);
	dn200_normal_reset(priv);
}

void dn200_vf_link_set(struct dn200_priv *priv, u8 link_reset)
{
	if (link_reset & LINK_UP_SET) {	/*link_up */
		priv->vf_link_action = LINK_UP_SET;
	} else if (link_reset & LINK_DOWN_SET) {	/*link_down */
		priv->vf_link_action = LINK_DOWN_SET;
	}
	queue_work(priv->wq, &priv->vf_linkset_task);
}

static void dn200_xpcs_all_rst(struct dn200_priv *priv)
{
	u32 reg_val = 0;
	u32 i = 0;
	/*phy_all rst*/
	for (i = 0; i < priv->plat_ex->total_pfs; i++) {
		reg_val = readl(priv->ioaddr + XGE_XGMAC_XPCS_SW_RST(i));
		if (reg_val & BIT(1))
			continue;
		writel(reg_val | BIT(1), priv->ioaddr + XGE_XGMAC_XPCS_SW_RST(i));
	}
	usleep_range(100, 200);

	/*LD_DN*/
	for (i = 0; i < priv->plat_ex->total_pfs; i++) {
		fw_reg_read(&priv->plat_ex->ctrl,
		 XPCS_REG_BASE + XPCS_REG_OFFSET * i + XPCS_VR_XS_PMA_MP_12G_16G_25G_SRAM,
			 &reg_val);
		if (reg_val & BIT(1))
			continue;
		reg_val |= BIT(1);
		fw_reg_write(&priv->plat_ex->ctrl,
			XPCS_REG_BASE + XPCS_REG_OFFSET * i + XPCS_VR_XS_PMA_MP_12G_16G_25G_SRAM,
				 reg_val);
	}
	usleep_range(100000, 200000);
}

static ssize_t temp_show(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	struct dn200_ctrl_resource *ctrl;
	struct dn200_priv *priv;
	struct net_device *ndev = to_net_dev(dev);
	char temp[64] = {};
	int ret;

	priv = netdev_priv(ndev);
	ctrl = &priv->plat_ex->ctrl;
	ret = dn200_dev_temp_get(ctrl, temp, sizeof(temp));
	if (ret)
		return ret;

	return snprintf(buf, sizeof(temp), temp);
}

void dn200_xgmac_clock_ctl(struct dn200_priv *priv)
{
	u32 value = 0;

	value = readl(priv->ioaddr + XGE_TOP_CONFIG_OFFSET +
			 0x1c + priv->plat_ex->pf_id * 0x50);
	value &= ~(BIT(2) | BIT(1));
	writel(value, priv->ioaddr + XGE_TOP_CONFIG_OFFSET +
			 0x1c + priv->plat_ex->pf_id * 0x50);
	usleep_range(10000, 20000);
	if (!PRIV_IS_VF(priv) && !priv->mii && !priv->plat_ex->raid_supported)
		dn200_xpcs_all_rst(priv);
	value = readl(priv->ioaddr + XGE_TOP_CONFIG_OFFSET +
			 0x1c + priv->plat_ex->pf_id * 0x50);
	value |= (BIT(2) | BIT(1));
	writel(value, priv->ioaddr + XGE_TOP_CONFIG_OFFSET +
			 0x1c + priv->plat_ex->pf_id * 0x50);
	usleep_range(10000, 20000);
}

/**
 * dn200_dvr_probe
 * @device: device pointer
 * @plat_dat: platform data pointer
 * @plat_ex: db200 platform data pointer
 * @res: dn200 resource pointer
 * Description: this is the main probe function used to
 * call the alloc_etherdev, allocate the priv structure.
 * Return:
 * returns 0 on success, otherwise errno.
 */
int dn200_dvr_probe(struct device *device,
		    struct plat_dn200enet_data *plat_dat,
		    struct plat_dn200_data *plat_ex,
		    struct dn200_resources *res)
{
	struct net_device *ndev = NULL;
	struct dn200_priv *priv;
	u8 macaddr[ETH_ALEN];
	u32 rxq;
	int i, ret = 0;
	int min_mtu = 0;
	struct dn200_ver dn200_ver_info = {0};

	ndev = devm_alloc_etherdev_mqs(device, sizeof(struct dn200_priv),
				       MTL_MAX_TX_QUEUES, MTL_MAX_RX_QUEUES);
	if (!ndev)
		return -ENOMEM;

	SET_NETDEV_DEV(ndev, device);

	priv = netdev_priv(ndev);
	memset(priv, 0, sizeof(*priv));
	priv->device = device;
	priv->dev = ndev;
	dn200_set_ethtool_ops(ndev);
	priv->dma32_iatu_used = false;
	priv->pause = PAUSE_TIME;
	priv->plat = plat_dat;
	priv->plat_ex = plat_ex;
	priv->ioaddr = res->addr;
	priv->dev->base_addr = (unsigned long)res->addr;
	priv->plat->dma_cfg->multi_msi_en = priv->plat->multi_msi_en;
	priv->dev->irq = res->irq;
	priv->lpi_irq = res->lpi_irq;
	priv->sfty_ce_irq = res->sfty_ce_irq;
	priv->sfty_ue_irq = res->sfty_ue_irq;
	priv->xpcs_irq = res->xpcs_vec;
	priv->plat_ex->pf.ioaddr = res->mail;
	priv->plat_ex->pf.ctrl_addr = res->ctrl_addr;
	priv->plat_ex->priv_back = priv;
	ndev->priv_flags |= IFF_UNICAST_FLT;
	priv->max_usecs = DN200_ITR_MAX_USECS;
	priv->min_usecs = DN200_ITR_MIN_USECS;
	priv->txrx_itr_combined = TXRX_ITR_PROCESS_SELF;	/*tx and rx irq process self */
	priv->speed = 10000;
	priv->speed_cmd = plat_ex->speed_cmd;
	priv->dn200_update_ops = &dn200_itr_update_ops;
	priv->numa_node = dev_to_node(device);
	priv->flag_upgrade = false;
	for (i = 0; i < MTL_MAX_RX_QUEUES; i++)
		priv->rx_irq[i] = res->rx_irq[i];
	for (i = 0; i < MTL_MAX_TX_QUEUES; i++)
		priv->tx_irq[i] = res->tx_irq[i];

	if (!plat_ex->is_vf)
		dn200_get_mac_from_firmware(priv, res);

	if (!is_zero_ether_addr(res->mac)) {
		eth_hw_addr_set(priv->dev, res->mac);
		ndev->addr_assign_type = NET_ADDR_PERM;
		ether_addr_copy(ndev->perm_addr, res->mac);
	}

	dev_set_drvdata(device, priv->dev);
	/* Allocate workqueue */
	priv->wq = alloc_workqueue("%s", WQ_UNBOUND | WQ_MEM_RECLAIM, 1,
				   "dn200_wq");
	if (!priv->wq) {
		dev_err(priv->device, "failed to create workqueue\n");
		return -ENOMEM;
	}
	priv->tx_wq =
	    alloc_workqueue("%s_tx_wq", WQ_MEM_RECLAIM | WQ_CPU_INTENSIVE, 1,
			    dev_name(priv->device));
	if (!priv->tx_wq) {
		dev_err(priv->device, "failed to create txq workqueue\n");
		return -ENOMEM;
	}

	INIT_WORK(&priv->service_task, dn200_service_task);
	INIT_WORK(&priv->reconfig_task, dn200_reconfig_task);
	INIT_WORK(&priv->vf_process_task, dn200_vf_process_task);
	INIT_WORK(&priv->vf_linkset_task, dn200_linkset_task);
	INIT_WORK(&priv->rxp_task, dn200_rxp_task);
	timer_setup(&priv->reset_timer, dn200_tx_reset, 0);
	INIT_WORK(&priv->retask, dn200_retask);
	if (!PRIV_IS_VF(priv))
		timer_setup(&priv->upgrade_timer, dn200_upgrade_timer, 0);
	/* Init MAC and get the capabilities */
	ret = dn200_hw_init(priv);
	if (ret)
		goto error_hw_init;

	if (PRIV_SRIOV_SUPPORT(priv))
		dn200_sriov_ver_set(priv, &dn200_ver_info);
	/* Only DWMAC core version 5.20 onwards supports HW descriptor prefetch.
	 */
	if (priv->chip_id < DWMAC_CORE_5_20)
		priv->plat->dma_cfg->dche = false;

	dn200_check_ether_addr(priv);
	if (PRIV_IS_VF(priv)) {
		ether_addr_copy(macaddr, priv->dev->dev_addr);
		dn200_uc_addr_set(priv, priv->plat_ex->vf_offset, macaddr);
		ndev->addr_assign_type = NET_ADDR_SET;
		ndev->netdev_ops = &dn200_vf_netdev_ops;
	} else {
		ndev->netdev_ops = &dn200_netdev_ops;
	}

	if (!PRIV_IS_VF(priv))
		ndev->dcbnl_ops = dn200_get_dcbnl_ops();
	ndev->hw_features = NETIF_F_SG;
	if (!HW_IS_PUREPF(priv->hw))
		ndev->features |= NETIF_F_RXCSUM;
	else
		ndev->hw_features |= NETIF_F_RXCSUM;
	ndev->hw_features |= NETIF_F_HW_CSUM;
	if (priv->plat->tso_en && priv->dma_cap.tsoen) {
		ndev->hw_features |= NETIF_F_TSO | NETIF_F_TSO6;
		priv->tso = true;
	}

	if (priv->dma_cap.sphen && !priv->plat->sph_disable) {
		ndev->hw_features |= NETIF_F_GRO;
		priv->sph_cap = true;
		priv->sph = priv->sph_cap;
		dev_info(priv->device, "SPH feature enabled\n");
	}

	/* The current IP register MAC_HW_Feature1[ADDR64] only define
	 * 32/40/64 bit width, but some SOC support others like i.MX8MP
	 * support 34 bits but it map to 40 bits width in MAC_HW_Feature1[ADDR64].
	 * So overwrite dma_cap.addr64 according to HW real design.
	 */
	if (priv->plat->addr64)
		priv->dma_cap.addr64 = priv->plat->addr64;

	if (priv->plat->addr64 > 32) {
		/* If more than 32 bits can be addressed, make sure to
		 * enable enhanced addressing mode.
		 */
		if (IS_ENABLED(CONFIG_ARCH_DMA_ADDR_T_64BIT))
			priv->plat->dma_cfg->eame = true;
	} else {
		priv->dma_cap.addr64 = 32;
	}
	if (PRIV_IS_VF(priv))
		ndev->watchdog_timeo = msecs_to_jiffies(120 * 1000);
	else
		ndev->watchdog_timeo = msecs_to_jiffies(TX_TIMEO);
	if (priv->dma_cap.rssen && priv->plat->rss_en) {
		if (!HW_IS_VF(priv->hw))
			ndev->hw_features |= NETIF_F_RXHASH;
	}

	if (!HW_IS_VF(priv->hw))
		ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_FILTER;
	ndev->features |= ndev->hw_features | NETIF_F_HIGHDMA;

	if (!HW_IS_VF(priv->hw)) {
		ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_RX;
		ndev->features |= NETIF_F_HW_VLAN_CTAG_RX;
	}

	if (priv->dma_cap.vlins) {
		ndev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX;
		ndev->features |= NETIF_F_HW_VLAN_CTAG_TX;
	}

	ndev->vlan_features = ndev->features;
	if (!HW_IS_VF(priv->hw)) {
		if (!dn200_ntuple_fdir_init(priv))
			ndev->hw_features |= NETIF_F_NTUPLE;
	}

	ndev->hw_features |= NETIF_F_RXALL;

	if (!PRIV_IS_VF(priv)) {
		/* Add Loopback capability to the device */
		ndev->hw_features |= NETIF_F_LOOPBACK;
	}
	/* Both mac100 and gmac support receive VLAN tag detection */
	priv->msg_enable = default_msg_level;

	/* Initialize RSS */
	rxq = priv->plat->rx_queues_to_use;
	netdev_rss_key_fill(priv->rss.key, sizeof(priv->rss.key));
	for (i = 0; i < ARRAY_SIZE(priv->rss.table); i++)
		priv->rss.table[i] = ethtool_rxfh_indir_default(i, rxq);

	// enable ip 2-tuple,tcp 4-tuple and udp 4-tuple
	priv->rss.rss_flags =
	    DN200_RSS_IP2TE | DN200_RSS_UDP4TE | DN200_RSS_TCP4TE;

	dn200_init_ndev_tunnel(ndev);
	ret = dn200_max_mtu_get(priv, &priv->plat->maxmtu, &min_mtu);
	if (ret < 0) {
		dev_err(priv->device, "max_mtu alloc err!\n");
		goto error_mtu_init;
	}

	/* MTU range: 68 - hw-specific max */
	ndev->min_mtu = min_mtu;
	if (priv->plat->has_xgmac)
		ndev->max_mtu = XGMAC_JUMBO_LEN;
	else if ((priv->plat->enh_desc) || (priv->chip_id >= DWMAC_CORE_4_00))
		ndev->max_mtu = JUMBO_LEN;
	else
		ndev->max_mtu = SKB_MAX_HEAD(NET_SKB_PAD + NET_IP_ALIGN);
	/* Will not overwrite ndev->max_mtu if plat->maxmtu > ndev->max_mtu
	 * as well as plat->maxmtu < ndev->min_mtu which is a invalid range.
	 */
	if (priv->plat->maxmtu < ndev->max_mtu &&
	    priv->plat->maxmtu >= ndev->min_mtu)
		ndev->max_mtu = priv->plat->maxmtu;
	else if (priv->plat->maxmtu < ndev->min_mtu)
		dev_warn(priv->device,
			 "%s: warning: maxmtu having invalid value (%d)\n",
			 __func__, priv->plat->maxmtu);
	priv->flow_ctrl = FLOW_AUTO; /* RX/TX pause on */
	if (priv->plat_ex->max_num_vlan)
		priv->hw->max_vlan_num = priv->plat_ex->max_num_vlan;
	else
		priv->hw->max_vlan_num = 0;

	mutex_init(&priv->lock);

	/* If a specific clk_csr value is passed from the platform
	 * this means that the CSR Clock Range selection cannot be
	 * changed at run-time and it is fixed. Viceversa the driver'll try to
	 * set the MDC clock dynamically according to the csr actual
	 * clock input.
	 */
	if (priv->plat->clk_csr >= 0)
		priv->clk_csr = priv->plat->clk_csr;
	else
		dn200_clk_csr_set(priv);

	ret = dn200_phyinfo_init(priv, ndev);
	if (ret) {
		dev_err(priv->device, "%s: ERROR %i init phy info\n",
			__func__, ret);
		goto error_phy_init;
	}
	netif_set_real_num_rx_queues(ndev, priv->plat_ex->default_rx_queue_num);
	netif_set_real_num_tx_queues(ndev, priv->plat_ex->default_tx_queue_num);
	dn200_axi_init_for_raid(priv);
	ret = register_netdev(ndev);
	if (ret) {
		dev_err(priv->device, "%s: ERROR %i registering the device\n",
			__func__, ret);
		goto error_netdev_register;
	}
	netif_carrier_off(ndev);

	if (priv->plat->dump_debug_regs)
		priv->plat->dump_debug_regs(priv->plat->bsp_priv);

	/* init itr divisor at first, and will be updated when link up */
	dn200_set_itr_divisor(priv, SPEED_10000);

	dn200_init_fs(ndev);
	set_bit(DN200_DCB_DOWN, &priv->state);
	set_bit(DN200_DOWN, &priv->state);

	/* pf/vf shared static info that will not be cleared when dev close/open */
	dn200_sriov_static_init(priv);

	if (!PRIV_IS_VF(priv)) {
		priv->temp_attr = kmalloc(sizeof(struct device_attribute), GFP_KERNEL);
		if (priv->temp_attr) {
			priv->temp_attr->show = temp_show;
			priv->temp_attr->store = NULL;
			priv->temp_attr->attr.name = "temp";
			priv->temp_attr->attr.mode = 00444;
			if (sysfs_create_file(&ndev->dev.kobj, &priv->temp_attr->attr)) {
				dev_info(priv->device, "sysfs_create_file failed.\n");
				kfree(priv->temp_attr);
				priv->temp_attr = NULL;
			}
		}
	}

	if (PRIV_IS_VF(priv))
		DN200_ITR_SYNC_SET(priv->hw, vf_probe, priv->plat_ex->vf_offset, 1);

	if (priv->mii) {
		dn200_xgmac_clock_ctl(priv);
		if (priv->mii) {
			/* set rgmii rx clock from soc */
			dn200_xgmac_rx_ext_clk_set(priv, false);
			/* workaround: set phy loopback for reg timeout */
			mdiobus_write(priv->mii, priv->plat->phy_addr, 0, 0x4140);
		}

		usleep_range(10000, 20000);
		dn200_reset(priv);
		usleep_range(10000, 20000);
		if (priv->mii)
			mdiobus_write(priv->mii, priv->plat->phy_addr, 0, 0x1940);
	} else if (!PRIV_IS_VF(priv)) {
		dn200_xgmac_clock_ctl(priv);
		usleep_range(10000, 20000);
		dn200_reset(priv);
		usleep_range(10000, 20000);
	}

	if (!PRIV_IS_PUREPF(priv))
		dn200_napi_add(ndev);

	if (!PRIV_IS_VF(priv))
		set_bit(DN200_PROBE_FINISHED, &priv->state);
	return ret;

error_netdev_register:
error_phy_init:
error_hw_init:
error_mtu_init:
	destroy_workqueue(priv->wq);
	destroy_workqueue(priv->tx_wq);
	return ret;
}
EXPORT_SYMBOL_GPL(dn200_dvr_probe);

static void dn200_task_stop(struct dn200_priv *priv)
{
	del_timer_sync(&priv->reset_timer);
	if (!PRIV_IS_VF(priv))
		del_timer_sync(&priv->upgrade_timer);
	if (priv->retask.func)
		cancel_work_sync(&priv->retask);
	if (priv->service_task.func)
		cancel_work_sync(&priv->service_task);
	if (priv->vf_process_task.func)
		cancel_work_sync(&priv->vf_process_task);
	if (priv->vf_linkset_task.func)
		cancel_work_sync(&priv->vf_linkset_task);
}

/**
 * dn200_dvr_remove
 * @dev: device pointer
 * Description: this function resets the TX/RX processes, disables the MAC RX/TX
 * changes the link status, releases the DMA descriptor rings.
 */
int dn200_dvr_remove(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct dn200_priv *priv = netdev_priv(ndev);

	if (PRIV_IS_VF(priv))
		DN200_ITR_SYNC_SET(priv->hw, vf_probe, priv->plat_ex->vf_offset, 0);
	else {
		if (priv->temp_attr) {
			sysfs_remove_file(&dev->kobj, &priv->temp_attr->attr);
			kfree(priv->temp_attr);
			priv->temp_attr = NULL;
		}
	}

	if (ndev->reg_state == NETREG_REGISTERED)
		unregister_netdev(ndev);
	if (!test_bit(DN200_PCIE_UNAVAILD, &priv->state))
		dn200_phy_info_remove(ndev);
	if (!PRIV_IS_PUREPF(priv))
		dn200_napi_del(priv->dev);
	dn200_axi_uninit_for_raid(priv);
	if (!priv->fdir_enties)
		kfree(priv->fdir_enties);
	dn200_exit_fs(ndev);
	if (priv->hw->pcs != DN200_PCS_TBI && priv->hw->pcs != DN200_PCS_RTBI)
		dn200_mdio_unregister(ndev);
	dn200_task_stop(priv);
	destroy_workqueue(priv->wq);
	destroy_workqueue(priv->tx_wq);
	mutex_destroy(&priv->lock);
	return 0;
}
EXPORT_SYMBOL_GPL(dn200_dvr_remove);

/**
 * dn200_suspend - suspend callback
 * @dev: device pointer
 * Description: this is the function to suspend the device and it is called
 * by the platform driver to stop the network queue, release the resources,
 * program the PMT register (for WoL), clean and release driver resources.
 */
int dn200_suspend(struct device *dev)
{
	struct net_device *ndev = dev_get_drvdata(dev);
	struct dn200_priv *priv = netdev_priv(ndev);
	struct plat_dn200_data *plat_ex = priv->plat_ex;
	struct dn200_ctrl_resource *ctrl = &plat_ex->ctrl;
	int retry = 0;

	if (test_bit(DN200_PCIE_UNAVAILD, &priv->state) || !dn200_hwif_id_check(priv->ioaddr))
		return -EIO;

	if (test_and_set_bit(DN200_NET_SUSPENDED, &priv->state))
		return 0;

	netif_trans_update(priv->dev);
	while (test_and_set_bit(DN200_RESETING, &priv->state)) {
		usleep_range(1000, 2000);
		if (retry++ >= 3)
			return 0;
	}

	if (ndev && netif_running(ndev)) {
		rtnl_lock();
		if (test_bit(DN200_SYS_SUSPENDED, &priv->state))
			priv->dev->netdev_ops->ndo_stop(priv->dev);
		else
			dev_close(priv->dev);
		rtnl_unlock();
	}

	if (!test_bit(DN200_SYS_SUSPENDED, &priv->state))
		return 0;

	if (!plat_ex->use_msi) {
		synchronize_irq(ctrl->msix_entries[plat_ex->total_irq - 2].vector);
		devm_free_irq(ctrl->dev,
			      ctrl->msix_entries[plat_ex->total_irq - 2].vector,
			      plat_ex);
		synchronize_irq(ctrl->msix_entries[plat_ex->total_irq - 1].vector);
		devm_free_irq(ctrl->dev,
			      ctrl->msix_entries[plat_ex->total_irq - 1].vector,
			      plat_ex);
		synchronize_irq(ctrl->msix_entries[0].vector);
		devm_free_irq(ctrl->dev, ctrl->msix_entries[0].vector, ctrl);
		pci_disable_msix(to_pci_dev(dev));
	} else {
		pci_free_irq_vectors(to_pci_dev(dev));
	}
	return 0;
}
EXPORT_SYMBOL_GPL(dn200_suspend);

/**
 * dn200_resume - resume callback
 * @dev: device pointer
 * Description: when resume this function is invoked to setup the DMA and CORE
 * in a usable state.
 */
int dn200_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct net_device *ndev = dev_get_drvdata(dev);
	struct dn200_priv *priv = netdev_priv(ndev);
	struct dn200_resources res;
	int i;

	if (test_bit(DN200_PCIE_UNAVAILD, &priv->state) || !dn200_hwif_id_check(priv->ioaddr))
		return -EIO;

	if (test_bit(DN200_SYS_SUSPENDED, &priv->state)) {
		memset(&res, 0, sizeof(res));

		res.addr = priv->ioaddr;

		irq_info_pfvf_release(pdev, &priv->plat_ex->ctrl, true);
		dn200_config_interrupt(pdev, priv->plat, priv->plat_ex, &res, !priv->plat_ex->use_msi);

		priv->dev->irq = res.irq;
		priv->lpi_irq = res.lpi_irq;
		priv->sfty_ce_irq = res.sfty_ce_irq;
		priv->sfty_ue_irq = res.sfty_ue_irq;
		priv->xpcs_irq = res.xpcs_vec;
		for (i = 0; i < MTL_MAX_RX_QUEUES; i++)
			priv->rx_irq[i] = res.rx_irq[i];
		for (i = 0; i < MTL_MAX_TX_QUEUES; i++)
			priv->tx_irq[i] = res.tx_irq[i];
		dn200_axi_init_for_raid(priv);
	}

	if (!test_and_clear_bit(DN200_NET_SUSPENDED, &priv->state))
		return 0;

	dn200_hw_sideband_init(PRIV_PHY_INFO(priv));
	rtnl_lock();
	if (test_bit(DN200_SYS_SUSPENDED, &priv->state)) {
		clear_bit(DN200_SYS_SUSPENDED, &priv->state);
		priv->dev->netdev_ops->ndo_open(priv->dev);
	} else {
		dev_open(priv->dev, NULL);
	}
	rtnl_unlock();
	clear_bit(DN200_DOWN, &priv->state);
	clear_bit(DN200_RESETING, &priv->state);
	clear_bit(DN200_SYS_SUSPENDED, &priv->state);

	return 0;
}
EXPORT_SYMBOL_GPL(dn200_resume);
