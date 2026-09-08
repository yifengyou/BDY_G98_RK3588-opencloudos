// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_edma_priv.h"

struct yk3_rxcb {
	u16 lro_valid:1;
	u16 lro_id;
};

#ifndef SKB_HEAD_ALIGN
#define SKB_HEAD_ALIGN(X) (SKB_DATA_ALIGN(X) + \
	SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
#endif

static int yk3_rxq_fill_rxd(struct yk3_rxq *rxq);
static int yk3_rxcq_handler(struct napi_struct *napi, int napi_budget);

static int rxq_debugfs_show(struct seq_file *seq, void *v)
{
	u16 i;

	struct yk3_rxq *rxq = seq->private;
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;

	if (v != SEQ_START_TOKEN)
		return 0;
	/* 1. rxq */
	/* 1.1. name */
	seq_printf(seq, "%-16s :\n", "rx queue");
	seq_printf(seq, "\t%-16s : %-16s\n", "netdev", ndev_priv->ndev->name);
	seq_printf(seq, "\t%-16s : %-4d\n", "l_id", rxq->qid.l_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "f_id", rxq->qid.f_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "p_id", rxq->qid.p_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "g_id", rxq->qid.g_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "qsetid", ndev_priv->qsetid);
	seq_printf(seq, "\t%-16s : %-4d\n", "active", rxq->active);
	/* 1.2. config param */
	seq_printf(seq, "\t%-16s : %-4d\n", "qdepth", rxq->qdepth);
	seq_printf(seq, "\t%-16s : %-4d\n", "qfragsize", rxq->qfragsize);
	/* 1.3. property */
	seq_printf(seq, "\t%-16s : %-4u\n", "qdepth_max", rxq->qdepth_max);
	seq_printf(seq, "\t%-16s : %-4u\n", "qfragsize_max", rxq->qfragsize_max);
	/* 1.4. stats */
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_alloc_page", rxq->stats_sw.err_alloc_page);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_map_page", rxq->stats_sw.err_map_page);
	/* 1.5. hw */
	/* 1.6 ring */
	seq_printf(seq, "\t%-16s : %-6d\n", "head", yk3_ringb_head_orig(&rxq->rxdrb));
	seq_printf(seq, "\t%-16s : %-6d\n", "tail", yk3_ringb_tail_orig(&rxq->rxdrb));

	/* 2. rxcq */
	/* 2.1. name */
	seq_printf(seq, "\n%-16s :\n", "rx cpl queue");
	/* 1.2. config param */
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_vector", rxq->rxcq->irq_vector);
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_period", rxq->rxcq->irq_period);
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_coal", rxq->rxcq->irq_coal);
	seq_printf(seq, "\t%-16s : %-6d\n", "period", rxq->rxcq->period);
	seq_printf(seq, "\t%-16s : %-6d\n", "coal", rxq->rxcq->coal);
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_disable", rxq->rxcq->irq_disable);
	/* 1.3 stats */
	seq_printf(seq, "\t%-16s : %-16llu\n", "packets", rxq->rxcq->stats_base.packets);
	seq_printf(seq, "\t%-16s : %-16llu\n", "bytes", rxq->rxcq->stats_base.bytes);
	seq_printf(seq, "\t%-16s : %-16llu\n", "errors", rxq->rxcq->stats_base.errors);
	seq_printf(seq, "\t%-16s : %-16llu\n", "drops", rxq->rxcq->stats_base.drops);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_interrupt",
		   rxq->rxcq->stats_sw.num_interrupt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_schedule",
		   rxq->rxcq->stats_sw.num_schedule);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_handler",
		   rxq->rxcq->stats_sw.num_handler);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_unicast_desc",
		   rxq->rxcq->stats_sw.num_unicast_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_unicast_pkt",
		   rxq->rxcq->stats_sw.num_unicast_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_multicast_desc",
		   rxq->rxcq->stats_sw.num_multicast_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_multicast_pkt",
		   rxq->rxcq->stats_sw.num_multicast_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_broadcast_desc",
		   rxq->rxcq->stats_sw.num_broadcast_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_broadcast_pkt",
		   rxq->rxcq->stats_sw.num_broadcast_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_vlan_8021ad",
		   rxq->rxcq->stats_sw.num_vlan_8021ad);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_vlan_8021q",
		   rxq->rxcq->stats_sw.num_vlan_8021q);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_vlan_remove",
		   rxq->rxcq->stats_sw.num_vlan_remove);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_lro_desc",
		   rxq->rxcq->stats_sw.num_lro_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_lro_pkt",
		   rxq->rxcq->stats_sw.num_lro_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_chkcpl_desc",
		   rxq->rxcq->stats_sw.num_chkcpl_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_chkcpl_pkt",
		   rxq->rxcq->stats_sw.num_chkcpl_pkt);
	for (i = 0; i < ARRAY_SIZE(rxq->rxcq->stats_sw.num_csum_unchk); i++) {
		if (rxq->rxcq->stats_sw.num_csum_unchk[i] == 0)
			continue;

		seq_printf(seq, "\t%sO%c%cI%c%c%-6s : %-16llu\n", "num_",
			   (i & 1) ? '3' : '.', (i & 2) ? '4' : '.',
			   (i & 4) ? '3' : '.', (i & 8) ? '4' : '.',
			   "unchk", rxq->rxcq->stats_sw.num_csum_unchk[i]);
	}
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_ptp_pkt",
		   rxq->rxcq->stats_sw.num_ptp_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_nopage",
		   rxq->rxcq->stats_sw.err_nopage);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_rcvsize",
		   rxq->rxcq->stats_sw.err_rcvsize);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_fcs_desc",
		   rxq->rxcq->stats_sw.err_fcs_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_fcs_pkt",
		   rxq->rxcq->stats_sw.err_fcs_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_mtu_desc",
		   rxq->rxcq->stats_sw.err_mtu_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_mtu_pkt",
		   rxq->rxcq->stats_sw.err_mtu_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_edma_desc",
		   rxq->rxcq->stats_sw.err_edma_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_edma_pkt",
		   rxq->rxcq->stats_sw.err_edma_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_ol3_csum_desc",
		   rxq->rxcq->stats_sw.err_ol3_csum_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_ol3_csum_pkt",
		   rxq->rxcq->stats_sw.err_ol3_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_ol4_csum_desc",
		   rxq->rxcq->stats_sw.err_ol4_csum_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_ol4_csum_pkt",
		   rxq->rxcq->stats_sw.err_ol4_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_il3_csum_desc",
		   rxq->rxcq->stats_sw.err_il3_csum_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_il3_csum_pkt",
		   rxq->rxcq->stats_sw.err_il3_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_il4_csum_desc",
		   rxq->rxcq->stats_sw.err_il4_csum_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_il4_csum_pkt",
		   rxq->rxcq->stats_sw.err_il4_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_pktcutoff_desc",
		   rxq->rxcq->stats_sw.err_pktcutoff_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_pktcutoff_pkt",
		   rxq->rxcq->stats_sw.err_pktcutoff_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_pkttimeo_desc",
		   rxq->rxcq->stats_sw.err_pkttimeo_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_pkttimeo_pkt",
		   rxq->rxcq->stats_sw.err_pkttimeo_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_unknown_desc",
		   rxq->rxcq->stats_sw.err_unknown_desc);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_alloc_skb",
		   rxq->rxcq->stats_sw.err_alloc_skb);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_build_skb",
		   rxq->rxcq->stats_sw.err_build_skb);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_gather",
		   rxq->rxcq->stats_sw.err_gather);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_csum_pkt",
		   rxq->rxcq->stats_sw.err_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_csum_pkt",
		   rxq->rxcq->stats_sw.num_csum_pkt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_rxcd_head",
		   rxq->rxcq->stats_sw.err_rxcd_head);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_rxcd_descid",
		   rxq->rxcq->stats_sw.err_rxcd_descid);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_watchdog",
		   rxq->rxcq->stats_sw.num_watchdog);
	/* 1.4. ring */
	seq_printf(seq, "\t%-16s : %-6d\n", "head_dma", le16_to_cpu(*rxq->rxcq->rxcd_head));
	seq_printf(seq, "\t%-16s : %-6d\n", "head", yk3_ringb_head_orig(&rxq->rxcq->rxcdrb));
	seq_printf(seq, "\t%-16s : %-6d\n", "tail", yk3_ringb_tail_orig(&rxq->rxcq->rxcdrb));

	/* 1.5 page pool */
	if (rxq->pp)
		yk3_pp_debugfs_show(seq, rxq->pp);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(rxq_debugfs);

static void *rxd_debugfs_start(struct seq_file *seq, loff_t *pos)
{
	struct yk3_rxq *rxq = seq->private;

	if (*pos >= rxq->qdepth)
		return NULL;

	return (*pos >= rxq->qdepth) ? NULL : (rxq->rxd + (*pos));
}

static void *rxd_debugfs_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	return rxd_debugfs_start(seq, pos);
}

static void rxd_debugfs_stop(struct seq_file *seq, void *v)
{
}

static int rxd_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_rxq *rxq = seq->private;
	struct yk3_rxd *rxd = v;
	long rxd_idx;

	if (!v)
		return 0;

	rxd_idx = rxd - rxq->rxd;
	seq_printf(seq, "%-6ld : %-16.16llx\n", rxd_idx, rxd->addr);

	return 0;
}

static const struct seq_operations rxd_debugfs_sops = {
	.start = rxd_debugfs_start,
	.next = rxd_debugfs_next,
	.stop = rxd_debugfs_stop,
	.show = rxd_debugfs_show,
};

DEFINE_SEQ_ATTRIBUTE(rxd_debugfs);

/* debug */
static void *rxcd_debugfs_start(struct seq_file *seq, loff_t *pos)
{
	struct yk3_rxq *rxq = seq->private;

	if (*pos >= rxq->qdepth)
		return NULL;

	return (*pos >= rxq->qdepth) ? NULL : (rxq->rxcq->rxcd + (*pos));
}

static void *rxcd_debugfs_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	return rxcd_debugfs_start(seq, pos);
}

static void rxcd_debugfs_stop(struct seq_file *seq, void *v)
{
}

static int rxcd_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_rxq *rxq = seq->private;
	struct yk3_rxcd *rxcd = v;
	long rxcd_idx;

	if (!v)
		return 0;

	rxcd_idx = rxcd - rxq->rxcq->rxcd;
	seq_printf(seq, "%-6ld : %-16.16llx, %-16.16llx\n",
		   rxcd_idx, rxcd->value1, rxcd->value2);

	return 0;
}

static const struct seq_operations rxcd_debugfs_sops = {
	.start = rxcd_debugfs_start,
	.next = rxcd_debugfs_next,
	.stop = rxcd_debugfs_stop,
	.show = rxcd_debugfs_show,
};

DEFINE_SEQ_ATTRIBUTE(rxcd_debugfs);

static irqreturn_t yk3_rxcq_int(int vector, void *data)
{
	struct yk3_rxcq *rxcq = data;

	rxcq->stats_sw.num_interrupt++;

	if (unlikely(!(rxcq->rxq->active)))
		return IRQ_HANDLED;

	if (likely(napi_schedule_prep(&rxcq->napi))) {
		rxcq->stats_sw.num_schedule++;
		yk3_rxcq_irq_disable(rxcq);
		__napi_schedule_irqoff(&rxcq->napi);
	}

	return IRQ_HANDLED;
}

static void yk3_rxcq_watchdog(struct timer_list *timer)
{
	struct yk3_rxcq *rxcq = container_of(timer, struct yk3_rxcq, watchdog);

	rxcq->stats_sw.num_watchdog++;

	if (!rxcq->rxq->active)
		return;

	napi_schedule(&rxcq->napi);
}

void yk3_rxcq_set_coal(struct yk3_rxcq *rxcq, u16 packets, u16 usecs)
{
	rxcq->coal = (packets < YK3_N_COAL_MAX) ? packets : YK3_N_COAL_MAX;
	rxcq->period = yk3_edma_usec_to_cycles(rxcq->rxq->ndev_priv->pdev_priv, usecs);
	rxcq->period = (rxcq->period < YK3_N_PERIOD_MAX) ? rxcq->period : YK3_N_PERIOD_MAX;
	rxcq->irq_period = rxcq->period;
	rxcq->irq_coal = rxcq->coal;

	yk3_wr32(rxcq->rxq->hw_addr, YK3_RE_RXCQ_IRQ_COAL, rxcq->irq_coal);
	yk3_wr32(rxcq->rxq->hw_addr, YK3_RE_RXCQ_IRQ_PERIOD, rxcq->irq_period);
	yk3_wr32(rxcq->rxq->hw_addr, YK3_RE_RXCQ_PERIOD, rxcq->period);
	yk3_wr32(rxcq->rxq->hw_addr, YK3_RE_RXCQ_COAL, rxcq->coal);
}

/*
 * Two-gear adaptive ITR: pick latency vs throughput gear from the byte rate
 * measured over a fixed window. Only touches the hardware on a gear change.
 */
static void yk3_rxcq_itr_update(struct yk3_rxcq *rxcq)
{
	u64 now = ktime_get_ns();
	u64 dt = now - rxcq->itr_ts;
	u64 dbytes, scaled;
	u8 gear = rxcq->itr_gear;

	if (dt < YK3_ITR_EVAL_NS)
		return;

	dbytes = rxcq->stats_base.bytes - rxcq->itr_bytes;
	scaled = dbytes * 1000;	/* dbytes*1000/dt(ns) == MB/s */

	if (scaled >= (u64)YK3_ITR_MBPS_HI * dt)
		gear = YK3_ITR_GEAR_THROUGHPUT;
	else if (scaled < (u64)YK3_ITR_MBPS_LO * dt)
		gear = YK3_ITR_GEAR_LATENCY;

	if (gear != rxcq->itr_gear) {
		rxcq->itr_gear = gear;
		if (gear == YK3_ITR_GEAR_THROUGHPUT)
			yk3_rxcq_set_coal(rxcq, YK3_ITR_TP_FRAMES, YK3_ITR_TP_USECS);
		else
			yk3_rxcq_set_coal(rxcq, YK3_ITR_LAT_FRAMES, YK3_ITR_LAT_USECS);
	}

	rxcq->itr_bytes = rxcq->stats_base.bytes;
	rxcq->itr_ts = now;
}

static int yk3_create_rxcq(struct yk3_rxq *rxq)
{
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_rxcq *rxcq;
	u64 size;
	int ret = 0;
	struct yk3_irq_param irq_param;
	struct dma_pool *cd_head_pool = pdev_priv->edma_priv->cd_head_pool;

	rxcq = kzalloc(sizeof(*rxcq), GFP_KERNEL);
	if (!rxcq)
		return -ENOMEM;

	yk3_ringb_init(&rxcq->rxcdrb, rxq->qdepth);

	/* hw addr */
	rxcq->hw_addr = rxq->hw_addr;

	size = sizeof(struct yk3_rxcd) * rxq->qdepth;
	rxcq->rxcd = dma_alloc_coherent(rxq->dev, size, &rxcq->rxcd_dma_addr, GFP_KERNEL);
	if (!rxcq->rxcd) {
		yk3_net_err("rxcq %d rxcdma dma alloc failed", rxq->qid.l_id);
		ret = -ENOMEM;
		goto rxcd_dma_failed;
	}

	rxcq->rxq = rxq;

	/* dma */
	rxcq->rxcd_head = dma_pool_zalloc(cd_head_pool, GFP_KERNEL, &rxcq->rxc_head_dma_addr);
	if (!rxcq->rxcd_head) {
		yk3_net_err("rxcq %d rxcd head dma alloc failed", rxq->qid.l_id);
		ret = -ENOMEM;
		goto rxc_head_dma_failed;
	}

	/* config params */
	yk3_rxcq_irq_disable(rxcq);

	snprintf(irq_param.name, sizeof(irq_param.name), "%s-rxcq%d",
		 ndev_priv->name, rxcq->rxq->qid.l_id);
	irq_param.vector = -1;
	irq_param.handler = yk3_rxcq_int;
	irq_param.data = rxcq;
	if (pdev_priv->card->mode == YK3_MODE_TCARD && yk3_ndev_is_pf(ndev_priv))
		irq_param.flags = YK3_F_IRQ_AFFINITY;
	else
		irq_param.flags = 0;
	ret = yk3_irq_request(pdev_priv, &irq_param);
	if (ret < 0) {
		yk3_net_err("rxcq %d register irq failed", rxq->qid.l_id);
		goto config_failed;
	}
	rxcq->irq_vector = ret;
	rxcq->irq_disable = 0;

	rxq->rxcq = rxcq;
	ndev_priv->qpair[rxq->qid.l_id].rxcq = rxcq;

	timer_setup(&rxcq->watchdog, yk3_rxcq_watchdog, 0);

	return 0;

config_failed:
	dma_pool_free(cd_head_pool, rxcq->rxcd_head, rxcq->rxc_head_dma_addr);
rxc_head_dma_failed:
	size = sizeof(struct yk3_rxcd) * rxq->qdepth;
	dma_free_coherent(rxq->dev, size, rxcq->rxcd, rxcq->rxcd_dma_addr);
rxcd_dma_failed:
	kfree(rxcq);
	return ret;
}

static void yk3_destroy_rxcq(struct yk3_rxq *rxq)
{
	struct yk3_rxcq *rxcq = rxq->rxcq;
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	u64 size;
	struct yk3_irq_param irq_param;
	struct dma_pool *cd_head_pool = pdev_priv->edma_priv->cd_head_pool;

	ndev_priv->qpair[rxq->qid.l_id].rxcq = NULL;

	irq_param.vector = rxcq->irq_vector;
	irq_param.handler = yk3_rxcq_int;
	irq_param.data = rxcq;
	yk3_irq_free(pdev_priv, &irq_param);

	dma_pool_free(cd_head_pool, rxcq->rxcd_head, rxcq->rxc_head_dma_addr);

	size = sizeof(struct yk3_rxcd) * rxq->qdepth;
	dma_free_coherent(rxq->dev, size, rxcq->rxcd, rxcq->rxcd_dma_addr);
	kfree(rxcq);
}

int yk3_create_rxq(struct yk3_ndev_priv *ndev_priv, u16 idx, u32 depth)
{
	struct yk3_rxq *rxq;
	size_t size;
	int ret;
	char name[32];

	if (!is_power_of_2(depth)) {
		yk3_net_err("rxq %d depth %d not power of 2", idx, depth);
		return -EINVAL;
	}

	size = sizeof(struct yk3_rxq);
	rxq = kzalloc(size, GFP_KERNEL);
	if (!rxq) {
		yk3_net_err("rxq alloc mem failed, size = %ld", size);
		return -ENOMEM;
	}

	size = sizeof(struct yk3_rxi) * depth;
	rxq->rxi = kzalloc(size, GFP_KERNEL);
	if (!rxq->rxi) {
		yk3_net_err("rxi alloc failed, size = %ld", size);
		ret = -ENOMEM;
		goto rxi_failed;
	}

	yk3_ringb_init(&rxq->rxdrb, depth);

	size = sizeof(struct yk3_rxd) * depth;
	rxq->rxd = dma_alloc_coherent(&ndev_priv->pdev->dev, size, &rxq->rxd_dma_addr, GFP_KERNEL);
	if (!rxq->rxd) {
		yk3_net_err("rxq %d rxd dma alloc failed", idx);
		ret = -ENOMEM;
		goto rxd_dma_failed;
	}

	rxq->dev = &ndev_priv->pdev->dev;

	rxq->qid.l_id = ndev_priv->qbase[YK3_QUEUE_T_LOCAL].start + idx;
	rxq->qid.f_id = ndev_priv->qbase[YK3_QUEUE_T_FUNC].start + idx;
	rxq->qid.p_id = ndev_priv->qbase[YK3_QUEUE_T_PF].start + idx;
	rxq->qid.g_id = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start + idx;

	/* hw addr */
	rxq->hw_addr = yk3_edma_hwaddr(ndev_priv->pdev_priv) +
		       YK3_RE_QX_BASE(rxq->qid.f_id);

	/* config params */
	rxq->qdepth = depth;

	rxq->active = 0;
	rxq->qdepth_max_power = yk3_rd32(rxq->hw_addr, YK3_RE_RXQ_DEPTH_MAX);
	/* should be this */
	//rxq->qdepth_max = 1 << rxq->qdepth_max_power;
	rxq->qdepth_max = YK3_N_MAX_QDEPTH;
	if (rxq->qdepth > rxq->qdepth_max) {
		yk3_net_err("rxq %d depth %d > max %d", idx, rxq->qdepth, rxq->qdepth_max);
		ret = -EINVAL;
		goto property_failed;
	}

	rxq->qfragsize_max = yk3_rd32(rxq->hw_addr, YK3_RE_RXQ_FRAGSIZE_MAX);

	rxq->ndev_priv = ndev_priv;

	ret = yk3_create_rxcq(rxq);
	if (ret) {
		yk3_net_err("rxq %d create rxcq failed", idx);
		goto rxcq_failed;
	}

	if (ndev_priv->dbgfs_dir) {
		snprintf(name, sizeof(name), "rxq_%d_info", rxq->qid.l_id);
		rxq->debugfs_info_file = debugfs_create_file(name, 0400, ndev_priv->dbgfs_dir, rxq,
							     &rxq_debugfs_fops);
		if (IS_ERR(rxq->debugfs_info_file))
			yk3_net_err("rxq %d create debugfs info file failed", idx);

		snprintf(name, sizeof(name), "rxq_%d_rxd", rxq->qid.l_id);
		rxq->debugfs_rxd_file = debugfs_create_file(name, 0400, ndev_priv->dbgfs_dir, rxq,
							    &rxd_debugfs_fops);
		if (IS_ERR(rxq->debugfs_rxd_file))
			yk3_net_err("rxq %d create debugfs rxd file failed", idx);

		snprintf(name, sizeof(name), "rxq_%d_rxcd", rxq->qid.l_id);
		rxq->debugfs_rxcd_file = debugfs_create_file(name, 0400, ndev_priv->dbgfs_dir, rxq,
							     &rxcd_debugfs_fops);
		if (IS_ERR(rxq->debugfs_rxcd_file))
			yk3_net_err("rxq %d create debugfs rxcd file failed", idx);
	}

	ndev_priv->qpair[rxq->qid.l_id].rxq = rxq;

	return 0;

rxcq_failed:
property_failed:
	size = sizeof(struct yk3_rxd) * depth;
	dma_free_coherent(rxq->dev, size, rxq->rxd, rxq->rxd_dma_addr);
rxd_dma_failed:
	kfree(rxq->rxi);
rxi_failed:
	kfree(rxq);
	return ret;
}

void yk3_destroy_rxq(struct yk3_rxq *rxq)
{
	u64 size;

	rxq->ndev_priv->qpair[rxq->qid.l_id].rxq = NULL;

	debugfs_remove(rxq->debugfs_rxcd_file);
	debugfs_remove(rxq->debugfs_rxd_file);
	debugfs_remove(rxq->debugfs_info_file);

	yk3_destroy_rxcq(rxq);

	size = sizeof(struct yk3_rxd) * rxq->qdepth;
	dma_free_coherent(rxq->dev, size, rxq->rxd, rxq->rxd_dma_addr);

	kfree(rxq->rxi);
	kfree(rxq);
}

int yk3_activate_rxq(struct yk3_rxq *rxq)
{
	struct yk3_rxcq *rxcq = rxq->rxcq;
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;
	struct napi_struct *napi = &rxcq->napi;
	u16 data_size;

	data_size = SKB_HEAD_ALIGN(ndev_priv->ndev->mtu + 18);
	rxq->pp = yk3_pp_create(ndev_priv->pdev_priv, data_size);
	if (!rxq->pp) {
		yk3_net_err("rxq %d create page pool failed", rxq->qid.l_id);
		return -ENOMEM;
	}

	yk3_ringb_init(&rxq->rxdrb, rxq->qdepth);
	yk3_ringb_init(&rxq->rxcq->rxcdrb, rxq->qdepth);
	*rxcq->rxcd_head = 0;

	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_CTRL, YK3_V_RXQ_RXCLR);

	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_ADDR_L, rxcq->rxcd_dma_addr);
	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_ADDR_H, rxcq->rxcd_dma_addr >> 32);

	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_HEAD_ADDR_L, rxcq->rxc_head_dma_addr);
	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_HEAD_ADDR_H, rxcq->rxc_head_dma_addr >> 32);

	yk3_wr32(rxcq->hw_addr, YK3_RE_RXCQ_IRQ_VECTOR, rxcq->irq_vector);

	yk3_rxcq_set_coal(rxcq, ndev_priv->rx_max_coalesced_frames, ndev_priv->rx_coalesce_usecs);
	rxcq->itr_gear = YK3_ITR_GEAR_THROUGHPUT;
	rxcq->itr_bytes = rxcq->stats_base.bytes;
	rxcq->itr_ts = ktime_get_ns();

	yk3_rxcq_irq_enable(rxcq);

#ifdef YK3_HAVE_NETIF_NAPI_ADD
	netif_napi_add(ndev_priv->ndev, napi, yk3_rxcq_handler, NAPI_POLL_WEIGHT);
#else
	netif_napi_add(ndev_priv->ndev, napi, yk3_rxcq_handler);
#endif
	napi_enable(napi);

	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_ADDR_L, rxq->rxd_dma_addr);
	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_ADDR_H, rxq->rxd_dma_addr >> 32);

	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_TAIL, yk3_ringb_tail_orig(&rxq->rxdrb));
	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_HEAD, yk3_ringb_head_orig(&rxq->rxdrb));

	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_DEPTH, ilog2(rxq->qdepth));

	rxq->qfragsize = yk3_pp_size(rxq->pp);
	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_FRAGSIZE, rxq->qfragsize);

	/* fill rxd & txi */
	yk3_rxq_fill_rxd(rxq);

	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_CTRL, YK3_V_RXQ_PROTECTOFF);
	rxq->active = 1;

	return 0;
}

void yk3_deactivate_rxq(struct yk3_rxq *rxq)
{
	rxq->active = 0;

	del_timer_sync(&rxq->rxcq->watchdog);

	/* clear rxd */
	yk3_wr32(rxq->hw_addr, YK3_RE_RXQ_CTRL, YK3_V_RXQ_RXCLR);

	yk3_rxcq_irq_disable(rxq->rxcq);
	napi_disable(&rxq->rxcq->napi);
	netif_napi_del(&rxq->rxcq->napi);
}

void yk3_clean_rxq(struct yk3_rxq *rxq)
{
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;
	struct yk3_rxi *rxi;
	int count = 0;
	u32 val;

	count = 10;
	while (count--) {
		val = yk3_rd32(rxq->hw_addr, YK3_RE_RXQ_CTRL);
		if (val & YK3_V_RXQ_RXEMPTY)
			break;
		usleep_range(1000, 2000);
	}
	if (!(val & YK3_V_RXQ_RXEMPTY))
		yk3_net_err("rxq %d clear rxd timeout", rxq->qid.l_id);

	/* clean rxi */
	count = 0;
	while (!yk3_ringb_empty(&rxq->rxdrb)) {
		rxi = rxq->rxi + yk3_ringb_tail(&rxq->rxdrb);

		if (rxi->skb)
			dev_kfree_skb_any(rxi->skb);

		if (rxi->page && !rxi->skb) {
			yk3_pp_free(rxq->pp, rxi->page, rxi->fragidx);
			rxi->page = NULL;
		}

		yk3_ringb_pop(&rxq->rxdrb);
		yk3_ringb_pop(&rxq->rxcq->rxcdrb);

		if (count++ > rxq->qdepth) {
			yk3_net_err("rxq clean rxi error, count %d > qdepth %d\n",
				    count, rxq->qdepth);
			break;
		}
	}

	/* free page again */
	for (count = 0; count < rxq->qdepth; count++) {
		rxi = rxq->rxi + count;
		if (rxi->page) {
			yk3_pp_free(rxq->pp, rxi->page, rxi->fragidx);
			rxi->page = NULL;
		}
	}

	yk3_pp_destroy(rxq->pp);
	rxq->pp = NULL;
}

static int yk3_rxq_fill_rxd(struct yk3_rxq *rxq)
{
	int ret = 0;
	struct yk3_rxd *rxd;
	struct yk3_rxi *rxi;
	int count = 0;
	dma_addr_t addr;
	bool nobuf = false;
	bool empty = false;

	if (yk3_ringb_left(&rxq->rxdrb) < 16)
		return 0;

	if (yk3_ringb_empty(&rxq->rxdrb))
		empty = true;

	while (yk3_ringb_left(&rxq->rxdrb)) {
		rxd = rxq->rxd + yk3_ringb_head(&rxq->rxdrb);
		rxi = rxq->rxi + yk3_ringb_head(&rxq->rxdrb);

		if (rxi->page)
			goto next;

		ret = yk3_pp_alloc(rxq->pp, &rxi->page);
		if (unlikely(ret < 0)) {
			rxq->stats_sw.err_alloc_page++;
			ret = -ENOMEM;
			nobuf = true;
			break;
		}
		rxi->fragidx = (u16)ret;

		addr = yk3_page_dma_addr(rxi->page, rxi->fragidx);

		dma_sync_single_for_device(rxq->dev, addr,
					   yk3_page_size(rxi->page),
					   DMA_FROM_DEVICE);
		rxd->addr = cpu_to_le64(addr);
next:
		count++;
		yk3_ringb_push(&rxq->rxdrb);
	}

	if (count)
		yk3_rxq_doorbell(rxq);

	if ((nobuf || empty) && yk3_ringb_empty(&rxq->rxdrb))
		mod_timer(&rxq->rxcq->watchdog, jiffies + msecs_to_jiffies(250));

	return ret;
}

static inline bool
yk3_rxcd_err(struct yk3_rxcq *rxcq, struct yk3_rxcd *rxcd,
	     struct yk3_rxi *rxi)
{
	u16 rcvsize;

	if (unlikely(!rxi->page)) {
		rxcq->stats_sw.err_nopage++;
		goto err_must;
	}

	rcvsize = le16_to_cpu((__force __le16)rxcd->size);
	if (unlikely(!rcvsize || rcvsize > rxcq->rxq->qfragsize)) {
		rxcq->stats_sw.err_rcvsize++;
		goto err_must;
	}

	if (unlikely(rxcd->edma_error)) {
		rxcq->stats_sw.err_edma_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_edma_pkt++;
		goto err_out;
	}

	if (unlikely(rxcd->ptp_rec))
		return false;

	if (unlikely(rxcd->fcs_error)) {
		rxcq->stats_sw.err_fcs_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_fcs_pkt++;
		goto err_out;
	}

	if (unlikely(rxcd->mtu_error && !rxcd->lro_valid)) {
		rxcq->stats_sw.err_mtu_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_mtu_pkt++;
		goto err_out;
	}

	if (unlikely(rxcd->outer_l3_csum_error)) {
		rxcq->stats_sw.err_ol3_csum_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_ol3_csum_pkt++;
		goto csum_err;
	}

	if (unlikely(rxcd->inner_l3_csum_error)) {
		rxcq->stats_sw.err_il3_csum_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_il3_csum_pkt++;
		goto csum_err;
	}

	if (unlikely(rxcd->outer_l4_csum_error && !rxcd->lro_valid)) {
		rxcq->stats_sw.err_ol4_csum_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_ol4_csum_pkt++;
		goto csum_err;
	}

	if (unlikely(rxcd->inner_l4_csum_error && !rxcd->inner_l4_csum_unchk)) {
		rxcq->stats_sw.err_il4_csum_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_il4_csum_pkt++;
		goto csum_err;
	}

	if (unlikely(rxcd->pkt_cutoff)) {
		rxcq->stats_sw.err_pktcutoff_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_pktcutoff_pkt++;
		goto err_out;
	}

	if (unlikely(rxcd->pkt_timeout)) {
		rxcq->stats_sw.err_pkttimeo_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.err_pkttimeo_pkt++;
		goto err_out;
	}

	return false;

csum_err:
	if (!(rxcq->rxq->ndev_priv->ndev->features & NETIF_F_RXCSUM))
		return false;
	rxcq->stats_sw.err_csum_pkt++;
err_out:
	if (rxcq->rxq->ndev_priv->ndev->features & NETIF_F_RXALL)
		return false;
err_must:
	if (rxcd->fd)
		rxcq->stats_base.errors++;
	return true;
}

static inline bool
yk3_rxcd_casttype(struct yk3_rxcq *rxcq, struct yk3_rxcd *rxcd)
{
	if (rxcd->ptp_rec)
		return false;

	if (rxcd->cast_type == 0) {
		rxcq->stats_sw.num_unicast_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.num_unicast_pkt++;
	} else if (rxcd->cast_type == 1) {
		rxcq->stats_sw.num_multicast_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.num_multicast_pkt++;
	} else if (rxcd->cast_type == 2) {
		rxcq->stats_sw.num_broadcast_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.num_broadcast_pkt++;
	} else {
		rxcq->stats_sw.err_unknown_desc++;
		return true;
	}

	return false;
}

static inline void yk3_rxcd_other(struct yk3_rxcq *rxcq, struct yk3_rxcd *rxcd)
{
	u16 unchk_idx;
	bool csum_err;
	bool csum_unchk;

	if (rxcd->ptp_rec) {
		rxcq->stats_sw.num_ptp_pkt++;
		return;
	}

	if (rxcd->csum_complete_valid) {
		rxcq->stats_sw.num_chkcpl_desc++;
		if (rxcd->fd)
			rxcq->stats_sw.num_chkcpl_pkt++;
	}

	unchk_idx = rxcd->outer_l3_csum_unchk | (rxcd->outer_l4_csum_unchk << 1) |
		    (rxcd->inner_l3_csum_unchk << 2) | (rxcd->inner_l4_csum_unchk << 3);

	rxcq->stats_sw.num_csum_unchk[unchk_idx]++;

	csum_err = !!rxcd->outer_l3_csum_error || !!rxcd->outer_l4_csum_error ||
		   !!rxcd->inner_l3_csum_error || !!rxcd->inner_l4_csum_error;
	csum_unchk = !!rxcd->outer_l3_csum_unchk && !!rxcd->outer_l4_csum_unchk &&
		     !!rxcd->inner_l3_csum_unchk && !!rxcd->inner_l4_csum_unchk;

	if (!rxcd->lro_valid && !csum_err && !csum_unchk)
		rxcq->stats_sw.num_csum_pkt++;
}

static inline void yk3_fixup_csum(struct sk_buff *skb)
{
	__be16 proto = ((struct ethhdr *)skb->data)->h_proto;
	int depth = 0;

	proto = __vlan_get_protocol(skb, proto, &depth);
	if (depth > ETH_HLEN)
		skb->csum = csum_partial(skb->data + ETH_HLEN, depth - ETH_HLEN, skb->csum);
}

static struct sk_buff *yk3_lro_build(struct yk3_rxcq *rxcq, struct sk_buff *skb)
{
	struct yk3_rxcb *rxcb = (struct yk3_rxcb *)(skb->cb);

	if (rxcb->lro_valid) {
		rxcq->stats_sw.num_lro_pkt++;
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}

	return skb;
}

static inline bool yk3_rxcq_update_head(struct yk3_rxcq *rxcq)
{
	u16 ori_head = yk3_ringb_head_orig(&rxcq->rxcdrb);
	struct yk3_rxq *rxq = rxcq->rxq;
	struct yk3_ndev_priv *ndev_priv = rxq->ndev_priv;
	bool ret;

	dma_rmb();
	ret = yk3_ringb_update_head(&rxcq->rxcdrb, le16_to_cpu(*rxcq->rxcd_head));
	if (yk3_ringb_used(&rxcq->rxcdrb) > yk3_ringb_used(&rxq->rxdrb)) {
		yk3_net_debug("rxq %-5d rxd %-5d %-5d rxcd %-5d %-5d ori_head %-5d",
			      rxq->qid.l_id, rxq->rxdrb.head, rxq->rxdrb.tail,
			      rxcq->rxcdrb.head, rxcq->rxcdrb.tail, ori_head);
		yk3_ringb_update_head(&rxcq->rxcdrb, ori_head);

		rxcq->stats_sw.err_rxcd_head++;

		return false;
	}

	return ret;
}

static int yk3_rxcq_handler(struct napi_struct *napi, int napi_budget)
{
	struct yk3_rxcq *rxcq = container_of(napi, struct yk3_rxcq, napi);
	struct yk3_ndev_priv *ndev_priv = rxcq->rxq->ndev_priv;
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_rxq *rxq = rxcq->rxq;
	struct yk3_rxi *rxi;
	struct yk3_rxcd *rxcd;
	struct sk_buff *skb;
	int done = 0;
	u16 rcvsize;
	u16 vlan_tci;
	u32 headlen;
	struct yk3_rxcb *rxcb;
	__sum16 tmp_csum;
	__le16 tmp_csum_complete;
	u32 rss_indir_qcount = 4 * ndev->real_num_rx_queues;
	u32 rss_indir_idx = 0;
	u32 offset;
	u16 desc_id;

	rxcq->stats_sw.num_handler++;
	if (unlikely(!(rxcq->rxq->active) || !(ndev->flags & IFF_UP))) {
		yk3_rxcq_update_head(rxcq);
		while (!yk3_ringb_empty(&rxcq->rxcdrb) && done < napi_budget) {
			yk3_ringb_pop(&rxq->rxdrb);
			yk3_ringb_pop(&rxcq->rxcdrb);
			done++;
		}
		goto out;
	}

	yk3_rxcq_update_head(rxcq);
again:
	while (!yk3_ringb_empty(&rxcq->rxcdrb) && done < napi_budget) {
		rxcd = rxcq->rxcd + yk3_ringb_tail(&rxcq->rxcdrb);
		rxi = rxcq->rxq->rxi + yk3_ringb_tail(&rxcq->rxcdrb);

		desc_id = rxcd->desc_id;
		if (desc_id != yk3_ringb_tail_orig(&rxcq->rxcdrb)) {
			rxcq->stats_sw.err_rxcd_descid++;
			yk3_net_debug("rxq %-5d rxd %-5d tail %-5d rxcd %-5d %-5d descid %-5d",
				      rxq->qid.l_id, rxq->rxdrb.head, rxq->rxdrb.tail,
				      rxcq->rxcdrb.head, rxcq->rxcdrb.tail, desc_id);
			done = napi_budget;
			goto out;
		}

		if (yk3_rxcd_err(rxcq, rxcd, rxi) ||
		    yk3_rxcd_casttype(rxcq, rxcd)) {
			yk3_ringb_pop(&rxq->rxdrb);
			yk3_ringb_pop(&rxcq->rxcdrb);
			if (rxi->skb)
				dev_kfree_skb_any(rxi->skb);
			rxi->skb = NULL;
			continue;
		}

		yk3_rxcd_other(rxcq, rxcd);

		rcvsize = le16_to_cpu((__force __le16)rxcd->size);

		dma_sync_single_for_cpu(rxq->dev, yk3_page_dma_addr(rxi->page, rxi->fragidx),
					yk3_page_size(rxi->page), DMA_FROM_DEVICE);

		skb = rxi->skb;
		if (rxcd->fd && !skb) {
			if (rxcd->ld && (SKB_HEAD_ALIGN(rcvsize) <= rxq->qfragsize)) {
				skb = build_skb(yk3_page_addr(rxi->page, rxi->fragidx),
						yk3_page_size(rxi->page));
				if (unlikely(!skb)) {
					rxcq->stats_sw.err_build_skb++;
					break;
				}
			} else {
				skb = napi_alloc_skb(napi, YK3_N_RX_MINDATA);
				if (unlikely(!skb)) {
					rxcq->stats_sw.err_alloc_skb++;
					break;
				}
			}

			if (rxcd->vlan_valid) {
				if (rxcd->vlan_protocol_type)
					rxcq->stats_sw.num_vlan_8021ad++;
				else
					rxcq->stats_sw.num_vlan_8021q++;

				vlan_tci = le16_to_cpu((__force __le16)yk3_rxcd_get_vlanid(rxcd));
				vlan_tci |= rxcd->vlan_pri << VLAN_PRIO_SHIFT;
				if (rxcd->vlan_protocol_type)
					__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021AD), vlan_tci);
				else
					__vlan_hwaccel_put_tag(skb, htons(ETH_P_8021Q), vlan_tci);
			}

			/* notice : it need lan configure hash elements */
			/* todo : hashtype should be PKT_HASH_TYPE_L4 or PKT_HASH_TYPE_L3
			 *        according to lan configuration(ethtool ?)
			 */
			if (rxcd->hash_result && (ndev->features & NETIF_F_RXHASH))
				skb_set_hash(skb, be16_to_cpu((__force __be16)rxcd->hash_result),
					     PKT_HASH_TYPE_L4);

			if (rxcd->vlan_tag_remove && rxcd->fd)
				rxcq->stats_sw.num_vlan_remove++;

			if (!rxcd->ptp_rec && rxcd->lro_valid) {
				rxcq->stats_sw.num_lro_desc++;
				rxcb = (struct yk3_rxcb *)(skb->cb);
				rxcb->lro_valid = 1;
				rxcb->lro_id = le16_to_cpu((__force __le16)rxcd->lro_id);
			}

			if (rxcd->ld && (SKB_HEAD_ALIGN(rcvsize) <= rxq->qfragsize)) {
				skb_put(skb, rcvsize);
				rxi->page = NULL;
			} else {
				headlen = min_t(u32, rcvsize, YK3_N_RX_MINDATA);
				memcpy(__skb_put(skb, headlen),
				       yk3_page_addr(rxi->page, rxi->fragidx), headlen);

				if (rcvsize - headlen) {
					offset = yk3_page_offset(rxi->page, rxi->fragidx) + headlen;
					skb_add_rx_frag(skb, 0, yk3_page_page(rxi->page),
							offset, rcvsize - headlen,
							yk3_page_size(rxi->page) - headlen);
					rxi->page = NULL;
				}
			}

			if (!rxcd->ptp_rec &&
			    (rxcd->csum_complete_valid && (ndev->features & NETIF_F_RXCSUM))) {
				skb->ip_summed = CHECKSUM_COMPLETE;
				tmp_csum_complete = (__force __le16)rxcd->csum_complete;
				tmp_csum = (__force __sum16)le16_to_cpu(tmp_csum_complete);
				skb->csum = csum_unfold(tmp_csum);
				yk3_fixup_csum(skb);
			}
		} else if (likely(skb) && !rxcd->fd) {
			offset = yk3_page_offset(rxi->page, rxi->fragidx);
			skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, yk3_page_page(rxi->page),
					offset, rcvsize, rxq->qfragsize);
			rxi->page = NULL;
		} else {
			/* it should not be there */
			rxcq->stats_sw.err_gather++;
			yk3_ringb_pop(&rxq->rxdrb);
			yk3_ringb_pop(&rxcq->rxcdrb);
			if (rxi->skb)
				dev_kfree_skb_any(rxi->skb);
			rxi->skb = NULL;
			continue;
		}

		if (rxcd->fd && rxcd->ptp_rec)
			yk3_ptp_set_rx_ts(pdev_priv, skb, rxcd->ts);

		rxi->skb = NULL;
		yk3_ringb_pop(&rxq->rxdrb);
		yk3_ringb_pop(&rxcq->rxcdrb);
		done++;

		if (!rxcd->ld) {
			rxi = rxq->rxi + yk3_ringb_tail(&rxcq->rxcdrb);
			rxi->skb = skb;
			continue;
		}

		skb = yk3_lro_build(rxcq, skb);
		if (unlikely(!skb))
			continue;

		skb_record_rx_queue(skb, rxq->qid.l_id);

		rxcq->stats_base.packets++;
		rxcq->stats_base.bytes += skb->len;
		if (is_power_of_2(rss_indir_qcount))
			rss_indir_idx = skb->hash & (rss_indir_qcount - 1);
		else
			rss_indir_idx = skb->hash - (skb->hash / rss_indir_qcount)
							 * rss_indir_qcount;
		rxcq->stats_rss_indir.num_rss_indir_idx[rss_indir_idx]++;

		skb->protocol = eth_type_trans(skb, ndev_priv->ndev);
		if (ndev_priv->ndev->features & NETIF_F_LRO &&
		    !(ndev_priv->ndev->features & NETIF_F_GRO)) {
			ndev_priv->ndev->features |= NETIF_F_GRO;
			napi_gro_receive(napi, skb);
			ndev_priv->ndev->features &= ~NETIF_F_GRO;
		} else {
			napi_gro_receive(napi, skb);
		}
	}

	if (done < napi_budget && yk3_rxcq_update_head(rxcq))
		goto again;

out:
	/* fill rxd */
	yk3_pp_recycle(rxq->pp);
	yk3_rxq_fill_rxd(rxq);

	if (done == napi_budget)
		return done;

	if (napi_complete_done(napi, done)) {
		if (ndev_priv->itr_rx_enable)
			yk3_rxcq_itr_update(rxcq);
		yk3_rxcq_irq_enable(rxcq);
	}

	return done;
}
