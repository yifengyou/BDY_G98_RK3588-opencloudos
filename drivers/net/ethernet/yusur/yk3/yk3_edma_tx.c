// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_edma_priv.h"

struct yk3_aggr_cache_mgr {
	struct kmem_cache *cache;
	atomic_t ref;
};

static struct yk3_aggr_cache_mgr cache_mgr = { 0 };

static int yk3_txcq_handler(struct napi_struct *napi, int napi_budget);
static void yk3_destroy_txcq(struct yk3_txq *txq);

static int txq_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_txq *txq = seq->private;
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;

	if (v != SEQ_START_TOKEN)
		return 0;
	/* 1. txq */
	/* 1.1. name */
	seq_printf(seq, "%-16s :\n", "tx queue");
	seq_printf(seq, "\t%-16s : %-16s\n", "netdev", ndev_priv->name);
	seq_printf(seq, "\t%-16s : %-4d\n", "l_id", txq->qid.l_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "f_id", txq->qid.f_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "p_id", txq->qid.p_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "g_id", txq->qid.g_id);
	seq_printf(seq, "\t%-16s : %-4d\n", "qsetid", ndev_priv->qsetid);
	seq_printf(seq, "\t%-16s : %-4d\n", "active", txq->active);
	/* 1.2. config param */
	seq_printf(seq, "\t%-16s : %-4d\n", "qgroup", txq->qgroup);
	seq_printf(seq, "\t%-16s : %-4d\n", "qdepth", txq->qdepth);
	seq_printf(seq, "\t%-16s : %-4d\n", "qfragsize", txq->qfragsize);
	/* 1.3. property */
	seq_printf(seq, "\t%-16s : %-4u\n", "qdepth_max", txq->qdepth_max);
	seq_printf(seq, "\t%-16s : %-4u\n", "qfragsize_max", txq->qfragsize_max);
	seq_printf(seq, "\t%-16s : %-4u\n", "qpktsize_max", txq->qpktsize_max);
	/* 1.4. stats */
	seq_printf(seq, "\t%-16s : %-16llu\n", "packets", txq->stats_base.packets);
	seq_printf(seq, "\t%-16s : %-16llu\n", "bytes", txq->stats_base.bytes);
	seq_printf(seq, "\t%-16s : %-16llu\n", "errors", txq->stats_base.errors);
	seq_printf(seq, "\t%-16s : %-16llu\n", "drops", txq->stats_base.drops);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_smalltso", txq->stats_sw.num_smalltso);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_bigtso", txq->stats_sw.num_bigtso);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_txd", txq->stats_sw.num_txd);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_txdfd", txq->stats_sw.num_txdfd);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_txdld", txq->stats_sw.num_txdld);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_txdummy", txq->stats_sw.num_txdummy);
	seq_printf(seq, "\t%-16s : %-16llu\n", "over_fragsize", txq->stats_sw.over_fragsize);
	seq_printf(seq, "\t%-16s : %-16llu\n", "over_pktsize", txq->stats_sw.over_pktsize);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_dmasg", txq->stats_sw.err_dmasg);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_linearize", txq->stats_sw.err_linearize);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_scatter", txq->stats_sw.err_scatter);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_notxd", txq->stats_sw.err_notxd);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_qstop", txq->stats_sw.num_qstop);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_qwakeup", txq->stats_sw.num_qwakeup);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_aggr_pkt", txq->stats_sw.num_aggr_pkts);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_aggr_bytes", txq->stats_sw.num_aggr_bytes);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_aggr_xmit", txq->stats_sw.num_aggr_xmit);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_aggr_pkt", txq->stats_sw.err_aggr_pkts);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_aggr_bytes", txq->stats_sw.err_aggr_bytes);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_aggr_xmit", txq->stats_sw.err_aggr_xmit);
	/* 1.5. hw */
	/* 1.6 ring */
	seq_printf(seq, "\t%-16s : %-6d\n", "head", yk3_ringb_head_orig(&txq->txdrb));
	seq_printf(seq, "\t%-16s : %-6d\n", "tail", yk3_ringb_tail_orig(&txq->txdrb));

	/* 2. txcq */
	/* 2.1. name */
	seq_printf(seq, "\n%-16s :\n", "tx cpl queue");
	/* 1.2. config param */
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_vector", txq->txcq->irq_vector);
	seq_printf(seq, "\t%-16s : %-6d\n", "coal", txq->txcq->coal);
	seq_printf(seq, "\t%-16s : %-6d\n", "period", txq->txcq->period);
	seq_printf(seq, "\t%-16s : %-6d\n", "irq_disable", txq->txcq->irq_disable);
	seq_printf(seq, "\t%-16s : %-6d\n", "cpllen", txq->txcq->qcpllen);
	/* 1.3 stats sw */
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_freeskb", txq->txcq->stats_sw.num_freeskb);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_interrupt", txq->txcq->stats_sw.num_interrupt);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_schedule", txq->txcq->stats_sw.num_schedule);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_handler", txq->txcq->stats_sw.num_handler);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_packets", txq->txcq->stats_sw.num_packets);
	seq_printf(seq, "\t%-16s : %-16llu\n", "num_bytes", txq->txcq->stats_sw.num_bytes);
	seq_printf(seq, "\t%-16s : %-16llu\n", "err_txcd_head", txq->txcq->stats_sw.err_txcd_head);
	/* 1.4. ring */
	seq_printf(seq, "\t%-16s : %-6d\n", "head_dma", le16_to_cpu(*txq->txcq->txcd_head));
	seq_printf(seq, "\t%-16s : %-6d\n", "head", yk3_ringb_head_orig(&txq->txcq->txcdrb));
	seq_printf(seq, "\t%-16s : %-6d\n", "tail", yk3_ringb_tail_orig(&txq->txcq->txcdrb));

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(txq_debugfs);

static void *txd_debugfs_start(struct seq_file *seq, loff_t *pos)
{
	struct yk3_txq *txq = seq->private;

	if (*pos >= txq->qdepth)
		return NULL;

	return (*pos >= txq->qdepth) ? NULL : (txq->txd + (*pos));
}

static void *txd_debugfs_next(struct seq_file *seq, void *v, loff_t *pos)
{
	(*pos)++;
	return txd_debugfs_start(seq, pos);
}

static void txd_debugfs_stop(struct seq_file *seq, void *v)
{
}

static int txd_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_txq *txq = seq->private;
	struct yk3_txd *txd = v;
	long txd_idx;

	if (!v)
		return 0;

	txd_idx = txd - txq->txd;
	seq_printf(seq, "%-6ld : %-16.16llx, %-16.16llx\n", txd_idx, txd->addr, txd->value);

	return 0;
}

static const struct seq_operations txd_debugfs_sops = {
	.start = txd_debugfs_start,
	.next = txd_debugfs_next,
	.stop = txd_debugfs_stop,
	.show = txd_debugfs_show,
};

DEFINE_SEQ_ATTRIBUTE(txd_debugfs);

static irqreturn_t yk3_txcq_int(int vector, void *data)
{
	struct yk3_txcq *txcq = data;

	txcq->stats_sw.num_interrupt++;

	if (unlikely(!(txcq->txq->active)))
		return IRQ_HANDLED;

	if (likely(napi_schedule_prep(&txcq->napi))) {
		txcq->stats_sw.num_schedule++;
		yk3_txcq_irq_disable(txcq);
		__napi_schedule_irqoff(&txcq->napi);
	}

	return IRQ_HANDLED;
}

void yk3_txcq_set_coal(struct yk3_txcq *txcq, u16 packets, u16 usecs)
{
	txcq->coal = (packets < YK3_N_COAL_MAX) ? packets : YK3_N_COAL_MAX;
	txcq->period = yk3_edma_usec_to_cycles(txcq->txq->ndev_priv->pdev_priv, usecs);
	txcq->period = (txcq->period < YK3_N_PERIOD_MAX) ? txcq->period : YK3_N_PERIOD_MAX;

	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_COAL, txcq->coal);
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_PERIOD, txcq->period);
}

/*
 * Two-gear adaptive ITR: pick latency vs throughput gear from the byte rate
 * measured over a fixed window. Only touches the hardware on a gear change.
 */
static void yk3_txcq_itr_update(struct yk3_txcq *txcq)
{
	u64 now = ktime_get_ns();
	u64 dt = now - txcq->itr_ts;
	u64 dbytes, scaled;
	u8 gear = txcq->itr_gear;

	if (dt < YK3_ITR_EVAL_NS)
		return;

	dbytes = txcq->stats_sw.num_bytes - txcq->itr_bytes;
	scaled = dbytes * 1000;	/* dbytes*1000/dt(ns) == MB/s */

	if (scaled >= (u64)YK3_ITR_MBPS_HI * dt)
		gear = YK3_ITR_GEAR_THROUGHPUT;
	else if (scaled < (u64)YK3_ITR_MBPS_LO * dt)
		gear = YK3_ITR_GEAR_LATENCY;

	if (gear != txcq->itr_gear) {
		txcq->itr_gear = gear;
		if (gear == YK3_ITR_GEAR_THROUGHPUT)
			yk3_txcq_set_coal(txcq, YK3_ITR_TP_FRAMES, YK3_ITR_TP_USECS);
		else
			yk3_txcq_set_coal(txcq, YK3_ITR_LAT_FRAMES, YK3_ITR_LAT_USECS);
	}

	txcq->itr_bytes = txcq->stats_sw.num_bytes;
	txcq->itr_ts = now;
}

static int yk3_aggr_init(struct yk3_txq *txq)
{
	struct yk3_aggr *aggr = &txq->aggr;
	struct kmem_cache *cache;
	size_t size;

	if (atomic_read(&cache_mgr.ref)) {
		aggr->cache = cache_mgr.cache;
		atomic_inc(&cache_mgr.ref);
		return 0;
	}

	size = sizeof(struct sk_buff *) * YK3_N_AGGR_PKT_MAXNUM;
	cache = kmem_cache_create("yk3_aggr_skb", size, 0, SLAB_HWCACHE_ALIGN, NULL);
	if (!cache)
		return -ENOMEM;

	cache_mgr.cache = cache;
	aggr->cache = cache;
	atomic_inc(&cache_mgr.ref);
	return 0;
}

static void yk3_aggr_exit(struct yk3_txq *txq)
{
	struct yk3_aggr *aggr = &txq->aggr;

	aggr->cache = NULL;
	if (!atomic_dec_and_test(&cache_mgr.ref))
		return;

	kmem_cache_destroy(cache_mgr.cache);
}

static int yk3_create_txcq(struct yk3_txq *txq)
{
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_txcq *txcq;
	int ret = 0;
	struct yk3_irq_param irq_param;
	struct dma_pool *cd_head_pool = pdev_priv->edma_priv->cd_head_pool;

	txcq = kzalloc(sizeof(*txcq), GFP_KERNEL);
	if (!txcq)
		return -ENOMEM;

	yk3_ringb_init(&txcq->txcdrb, txq->qdepth);
	txcq->txq = txq;

	/* hw addr */
	txcq->hw_addr = txq->hw_addr;

	/* dma */
	txcq->txcd_head = dma_pool_zalloc(cd_head_pool, GFP_KERNEL, &txcq->txc_head_dma_addr);
	if (!txcq->txcd_head) {
		yk3_net_err("txcq %d txcd head dma alloc failed", txq->qid.l_id);
		ret = -ENOMEM;
		goto txcq_failed;
	}
	/* config param */
	txcq->irq_disable = 0;
	txcq->qcpllen = CPLLEN_64K;

	yk3_txcq_irq_disable(txcq);

	snprintf(irq_param.name, sizeof(irq_param.name), "%s-txcq%d",
		 ndev_priv->name, txcq->txq->qid.l_id);
	irq_param.vector = -1;
	irq_param.handler = yk3_txcq_int;
	irq_param.data = txcq;
	if (pdev_priv->card->mode == YK3_MODE_TCARD && yk3_ndev_is_pf(ndev_priv))
		irq_param.flags = YK3_F_IRQ_AFFINITY;
	else
		irq_param.flags = 0;
	ret = yk3_irq_request(pdev_priv, &irq_param);
	if (ret < 0) {
		yk3_net_err("txcq %d register irq failed", txq->qid.l_id);
		goto config_failed;
	}
	txcq->irq_vector = ret;

	txq->txcq = txcq;
	ndev_priv->qpair[txq->qid.l_id].txcq = txcq;

	return 0;

config_failed:
	dma_pool_free(cd_head_pool, txcq->txcd_head, txcq->txc_head_dma_addr);
txcq_failed:
	kfree(txcq);
	return ret;
}

int yk3_create_txq(struct yk3_ndev_priv *ndev_priv, u16 idx, u32 depth)
{
	struct yk3_txq *txq;
	size_t size;
	int ret;
	char name[32];

	if (!is_power_of_2(depth)) {
		yk3_net_err("txq %d depth %d not power of 2", idx, depth);
		return -EINVAL;
	}

	size = sizeof(*txq);
	txq = kzalloc(size, GFP_KERNEL);
	if (!txq) {
		yk3_net_err("txq alloc failed, size %lu", size);
		return -ENOMEM;
	}

	size = sizeof(struct yk3_txi) * depth;
	txq->txi = kzalloc(size, GFP_KERNEL);
	if (!txq->txi) {
		yk3_net_err("txi alloc failed, size %lu, depth %u, struct size %lu",
			    size, depth, sizeof(struct yk3_txi));
		ret = -ENOMEM;
		goto txi_failed;
	}

	/* txd & txi */
	yk3_ringb_init(&txq->txdrb, depth);
	size = sizeof(struct yk3_txd) * depth;
	txq->txd = dma_alloc_coherent(&ndev_priv->pdev->dev, size,
				      &txq->txd_dma_addr, GFP_KERNEL);
	if (!txq->txd) {
		ret = -ENOMEM;
		goto txq_failed;
	}

	txq->dev = &ndev_priv->pdev->dev;
	txq->ndev_priv = ndev_priv;
	txq->qid.l_id = ndev_priv->qbase[YK3_QUEUE_T_LOCAL].start + idx;
	txq->qid.f_id = ndev_priv->qbase[YK3_QUEUE_T_FUNC].start + idx;
	txq->qid.p_id = ndev_priv->qbase[YK3_QUEUE_T_PF].start + idx;
	txq->qid.g_id = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start + idx;

	/* hw addr */
	txq->hw_addr = yk3_edma_hwaddr(ndev_priv->pdev_priv) +
		       YK3_RE_QX_BASE(txq->qid.f_id);

	/* config params */
	txq->qdepth = depth;
	txq->qfragsize = YK3_N_MAX_TXFRAGSIZE;

	/* property */
	txq->active = 0;
	txq->qdepth_max_power = yk3_rd32(txq->hw_addr, YK3_RE_TXQ_DEPTH_MAX);
	/* should be this */
	//txq->qdepth_max = 1 << txq->qdepth_max_power;
	txq->qdepth_max = YK3_N_MAX_QDEPTH;
	if (txq->qdepth > txq->qdepth_max) {
		yk3_net_err("txq %d depth %d > max %d", idx, txq->qdepth, txq->qdepth_max);
		ret = -EINVAL;
		goto property_failed;
	}

	txq->qfragsize_max = yk3_rd32(txq->hw_addr, YK3_RE_TXQ_FRAGSIZE_MAX);
//	if (txq->qfragsize > txq->qfragsize_max) {
//		yk3_net_err("txq %d fragsize %d > max %d", idx, txq->qfragsize, txq->qfragsize_max);
//		ret = -EINVAL;
//		goto property_failed;
//	}
	txq->qpktsize_max = YK3_N_MAX_TXPKTLEN;

	ret = yk3_create_txcq(txq);
	if (ret) {
		yk3_net_err("txq %d create txcq failed", idx);
		goto txcq_failed;
	}

	ret = yk3_aggr_init(txq);
	if (ret) {
		yk3_net_err("txq %d aggr init failed", idx);
		goto aggr_failed;
	}

	if (ndev_priv->dbgfs_dir) {
		snprintf(name, sizeof(name), "txq_%d_info", txq->qid.l_id);
		txq->debugfs_info_file = debugfs_create_file(name, 0400, ndev_priv->dbgfs_dir, txq,
							     &txq_debugfs_fops);
		if (IS_ERR(txq->debugfs_info_file))
			yk3_net_err("txq %d create debugfs info file failed", idx);

		snprintf(name, sizeof(name), "txq_%d_txd", txq->qid.l_id);
		txq->debugfs_txd_file = debugfs_create_file(name, 0400, ndev_priv->dbgfs_dir, txq,
							    &txd_debugfs_fops);
		if (IS_ERR(txq->debugfs_txd_file))
			yk3_net_err("txq %d create debugfs txd file failed", idx);
	}

	txq->qgroup = ndev_priv->pf_id << 3;
	if (ndev_priv->type == YK3_NDEV_T_PF)
		ndev_priv->tc_cfg.tc2txq[txq->qid.l_id % 8][txq->qid.l_id / 8] = txq->qid.l_id;

	ndev_priv->qpair[idx].txq = txq;

	return 0;

aggr_failed:
	yk3_destroy_txcq(txq);
txcq_failed:
property_failed:
	size = sizeof(struct yk3_txd) * depth;
	dma_free_coherent(&ndev_priv->pdev->dev, size, txq->txd, txq->txd_dma_addr);
txq_failed:
	kfree(txq->txi);
txi_failed:
	kfree(txq);
	return ret;
}

static void yk3_destroy_txcq(struct yk3_txq *txq)
{
	struct yk3_txcq *txcq = txq->txcq;
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_irq_param irq_param;
	struct dma_pool *cd_head_pool = pdev_priv->edma_priv->cd_head_pool;

	ndev_priv->qpair[txq->qid.l_id].txcq = NULL;

	irq_param.vector = txcq->irq_vector;
	irq_param.handler = yk3_txcq_int;
	irq_param.data = txcq;
	yk3_irq_free(pdev_priv, &irq_param);

	dma_pool_free(cd_head_pool, txcq->txcd_head, txcq->txc_head_dma_addr);
	kfree(txcq);
}

void yk3_destroy_txq(struct yk3_txq *txq)
{
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	size_t size;

	ndev_priv->qpair[txq->qid.l_id].txq = NULL;

	debugfs_remove(txq->debugfs_info_file);
	debugfs_remove(txq->debugfs_txd_file);

	yk3_aggr_exit(txq);
	yk3_destroy_txcq(txq);

	size = sizeof(struct yk3_txd) * txq->qdepth;
	dma_free_coherent(&ndev_priv->pdev->dev, size, txq->txd, txq->txd_dma_addr);

	kfree(txq->txi);
	kfree(txq);
}

int yk3_activate_txq(struct yk3_txq *txq)
{
	struct yk3_txcq *txcq = txq->txcq;
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	struct napi_struct *napi = &txq->txcq->napi;

	yk3_ringb_init(&txq->txdrb, txq->qdepth);
	yk3_ringb_init(&txcq->txcdrb, txq->qdepth);
	*txcq->txcd_head = 0;

	/* txcq */
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_HEAD_ADDR_L, (u32)(txcq->txc_head_dma_addr));
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_HEAD_ADDR_H, (u32)(txcq->txc_head_dma_addr >> 32));

	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_IRQ_VECTOR, txcq->irq_vector);

	yk3_txcq_set_coal(txcq, ndev_priv->tx_max_coalesced_frames, ndev_priv->tx_coalesce_usecs);
	txcq->itr_gear = YK3_ITR_GEAR_THROUGHPUT;
	txcq->itr_bytes = txcq->stats_sw.num_bytes;
	txcq->itr_ts = ktime_get_ns();

	yk3_txcq_irq_enable(txcq);
	yk3_wr32(txcq->hw_addr, YK3_RE_TXCQ_CPLLEN, txcq->qcpllen);

	/* napi */
#ifdef YK3_HAVE_NETIF_NAPI_ADD
	netif_napi_add(ndev_priv->ndev, napi, yk3_txcq_handler, NAPI_POLL_WEIGHT);
#else
	netif_napi_add(ndev_priv->ndev, napi, yk3_txcq_handler);
#endif
	napi_enable(napi);
	txq->tx_queue = netdev_get_tx_queue(ndev_priv->ndev, txq->qid.l_id);

	/* txq */
	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_CTRL, 0);

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_ADDR_L, (u32)(txq->txd_dma_addr));
	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_ADDR_H, (u32)(txq->txd_dma_addr >> 32));

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_TAIL, yk3_ringb_tail_orig(&txq->txdrb));
	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_HEAD, yk3_ringb_head_orig(&txq->txdrb));

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_DEPTH, ilog2(txq->qdepth));

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_FRAGSIZE, txq->qfragsize);

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_CTRL, 1);

	txq->active = 1;

	return 0;
}

void yk3_deactivate_txq(struct yk3_txq *txq)
{
	int i;
	u32 txq_head, txq_tail, txcq_head;
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;

	txq->active = 0;
	for (i = 0; i < 10; i++) {
		txq_head = yk3_rd32(txq->hw_addr, YK3_RE_TXQ_HEAD);
		txq_tail = yk3_rd32(txq->hw_addr, YK3_RE_TXQ_TAIL);
		txcq_head = yk3_rd32(txq->hw_addr, YK3_RE_TXCQ_HEAD);

		if ((!txq_head && !txq_tail) ||
		    (txq_head == txq_tail && txcq_head == txq_tail))
			break;

		msleep(20);
	}

	if (i >= 10)
		yk3_net_err("txq %d clean txq timeout", txq->qid.l_id);

	yk3_wr32(txq->hw_addr, YK3_RE_TXQ_CTRL, 0);

	usleep_range(100, 200);

	yk3_txcq_irq_disable(txq->txcq);

	napi_disable(&txq->txcq->napi);
	netif_napi_del(&txq->txcq->napi);
}

void yk3_clean_txq(struct yk3_txq *txq)
{
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	struct yk3_txi *txi;
	u32 count = 0;
	u16 head = yk3_ringb_head_orig(&txq->txdrb);
	u16 tail = yk3_ringb_tail_orig(&txq->txdrb);

	/* clean txi */
	while (!yk3_ringb_empty(&txq->txdrb)) {
		txi = txq->txi + yk3_ringb_tail(&txq->txdrb);

		if ((txi->flags & YK3_N_FLAG_NORMAL) && txi->n.addr) {
			dma_unmap_single(txq->dev, dma_unmap_addr(txi, n.addr),
					 dma_unmap_len(txi, n.len), DMA_TO_DEVICE);
			txi->n.addr = 0;
			txi->n.len = 0;
		}
		if ((txi->flags & YK3_N_FLAG_NORMAL) && txi->n.skb) {
			dev_kfree_skb_any(txi->n.skb);
			txi->n.skb = NULL;
		}

		yk3_ringb_pop(&txq->txdrb);
		yk3_ringb_pop(&txq->txcq->txcdrb);

		if (count++ > txq->qdepth) {
			yk3_net_err("txq %d(head %d tail %d)clean txi error, count %d > qdepth %d\n",
				    txq->qid.l_id, head, tail, count, txq->qdepth);
			break;
		}
	}
}

static inline bool yk3_txcq_update_head(struct yk3_txcq *txcq)
{
	u16 ori_head = yk3_ringb_head_orig(&txcq->txcdrb);
	struct yk3_txq *txq = txcq->txq;
	struct yk3_ndev_priv *ndev_priv = txq->ndev_priv;
	bool ret;

	dma_rmb();
	ret = yk3_ringb_update_head(&txcq->txcdrb, le16_to_cpu(*txcq->txcd_head));
	if (yk3_ringb_used(&txcq->txcdrb) > yk3_ringb_used(&txq->txdrb)) {
		yk3_net_debug("txq %-5d txd %-5d %-5d txcd %-5d %-5d ori_head %-5d",
			      txq->qid.l_id, txq->txdrb.head, txq->txdrb.tail,
			      txcq->txcdrb.head, txcq->txcdrb.tail, ori_head);
		yk3_ringb_update_head(&txcq->txcdrb, ori_head);

		txcq->stats_sw.err_txcd_head++;

		return false;
	}

	return ret;
}

static int yk3_txcq_handler(struct napi_struct *napi, int napi_budget)
{
	struct yk3_txcq *txcq = container_of(napi, struct yk3_txcq, napi);
	struct yk3_ndev_priv *ndev_priv = txcq->txq->ndev_priv;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(ndev_priv->pdev);
	struct yk3_txq *txq = txcq->txq;
	int done = 0;
	struct yk3_txi *txi;
	int i;

	txcq->stats_sw.num_handler++;

	yk3_txcq_update_head(txcq);
again:
	while (!yk3_ringb_empty(&txcq->txcdrb) && done < napi_budget) {
		txi = txq->txi + yk3_ringb_tail(&txcq->txcdrb);

		if (unlikely(txi->flags & YK3_N_FLAG_PTP) && txi->n.skb) {
			yk3_ptp_set_tx_ts(pdev_priv, txi->n.skb);
			txi->flags &= ~YK3_N_FLAG_PTP;
		}

		if ((txi->flags & YK3_N_FLAG_NORMAL) && txi->n.addr) {
			dma_unmap_single(txq->dev, dma_unmap_addr(txi, n.addr),
					 dma_unmap_len(txi, n.len), DMA_TO_DEVICE);
			txi->n.addr = 0;
			txi->n.len = 0;
		}
		if ((txi->flags & YK3_N_FLAG_NORMAL) && txi->n.skb) {
			txcq->stats_sw.num_packets++;
			txcq->stats_sw.num_bytes += txi->n.skb->len;
			txcq->stats_sw.num_freeskb++;
			dev_consume_skb_any(txi->n.skb);
			txi->n.skb = NULL;
		}

		if (txi->flags & YK3_N_FLAG_AGGR) {
			dma_unmap_page(txq->dev, txi->a.addr, txi->a.len, DMA_TO_DEVICE);
			__free_pages(txi->a.page, YK3_N_AGGR_PAGEPARAM);
			for (i = 0; i < txi->a.num; i++)
				dev_kfree_skb_any(txi->a.skb[i]);
			kmem_cache_free(txq->aggr.cache, txi->a.skb);
			txi->a.num = 0;
			txi->a.skb = NULL;
			txi->a.page = NULL;
			txi->a.addr = 0;
			txi->a.len = 0;
		}

		txi->flags = 0;

		yk3_ringb_pop(&txcq->txcdrb);
		yk3_ringb_pop(&txq->txdrb);
		done++;
	}

	if (done < napi_budget && yk3_txcq_update_head(txcq))
		goto again;

	/* wake queue if it is stopped */
	if (netif_tx_queue_stopped(txq->tx_queue) &&
	    !yk3_txq_is_full(txq)) {
		txq->stats_sw.num_qwakeup++;
		netif_tx_wake_queue(txq->tx_queue);
	}

	if (done == napi_budget)
		return done;

	if (napi_complete_done(napi, done)) {
		if (ndev_priv->itr_tx_enable)
			yk3_txcq_itr_update(txcq);
		yk3_txcq_irq_enable(txq->txcq);
	}

	return done;
}

static inline bool yk3_check_size(struct yk3_txq *txq, struct sk_buff *skb)
{
	int hdrlen = 0;

	if (!skb_is_gso(skb))
		goto not_gso_check;
	else
		goto gso_check;

gso_check:
	if (skb->len <= txq->qpktsize_max) {
		txq->stats_sw.num_smalltso++;
	} else {
		txq->stats_sw.num_bigtso++;
		if (skb_shinfo(skb)->gso_type & (SKB_GSO_TCPV6 | SKB_GSO_TCPV4))
			hdrlen = skb->encapsulation ? (skb_inner_transport_offset(skb) +
				inner_tcp_hdrlen(skb)) : (skb_transport_offset(skb) +
				tcp_hdrlen(skb));
#ifdef YK3_HAVE_NETIF_F_GSO_UDP_L4
		else if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_L4)
			hdrlen = skb->encapsulation ? (skb_inner_transport_offset(skb) +
				 sizeof(struct udphdr)) : (skb_transport_offset(skb) +
				 sizeof(struct udphdr));
#endif
		if ((skb_headlen(skb) < hdrlen) && (skb_linearize(skb) < 0)) {
			txq->stats_sw.err_linearize++;
			return false;
		}
	}

	return true;

not_gso_check:
	if (unlikely(skb_headlen(skb) > txq->qfragsize || skb->data_len > txq->qfragsize)) {
		txq->stats_sw.over_fragsize++;
		return false;
	}

	if (unlikely(skb->len > txq->qpktsize_max)) {
		txq->stats_sw.over_pktsize++;
		return false;
	}

	return true;
}

static int
yk3_tx_check_scatter(struct yk3_txq *txq, struct yk3_scatter *scatter)
{
	int i;
	struct yk3_sctfrag *frag;
	u16 will_size;
	bool rollback = false;
	u16 ctrld_num;
	struct yk3_ringbase txdrb = txq->txdrb;
	bool in_bottom = yk3_ringb_in_bottom(&txdrb);
	u16 bottom_left = yk3_ringb_bottom_left(&txdrb);

	for (i = 0, will_size = 0; i < ARRAY_SIZE(scatter->frags); i++) {
		frag = &scatter->frags[i];

		if (!frag->seg_num)
			break;
		ctrld_num = frag->tso_valid ? 1 : 0;
		if ((will_size + frag->seg_num + ctrld_num) > bottom_left &&
		    in_bottom && !rollback) {
			will_size = bottom_left;
			rollback = true;
		}
		will_size += frag->seg_num + ctrld_num;
	}

	if (will_size > yk3_ringb_left(&txdrb)) {
		txq->stats_sw.err_notxd++;
		return -ENOMEM;
	}

	return 0;
}

static void yk3_txq_bottom_fill_dummy(struct yk3_txq *txq)
{
	u16 i;
	u16 bottom = yk3_ringb_bottom_left(&txq->txdrb);
	struct yk3_txd *txd;
	struct yk3_txi *txi;

	for (i = 0; i < bottom; i++) {
		txd = txq->txd + yk3_ringb_head(&txq->txdrb);
		txi = txq->txi + yk3_ringb_head(&txq->txdrb);

		txd->value = 0;
		txd->fd = 1;
		txd->ld = 1;
		txi->flags = 0;
		yk3_ringb_push(&txq->txdrb);
	}

	txq->stats_sw.num_txdummy += bottom;
}

static int yk3_aggr_xmit(struct yk3_txq *txq)
{
	struct yk3_aggr *aggr = &txq->aggr;
	struct device *dev = txq->ndev_priv->pdev_priv->dev;
	int ret;
	struct yk3_ad *ad;
	void *data;
	int i;
	struct sk_buff *skb;
	u16 vlan_pri;
	bool is_8021ad;
	struct yk3_atxd *atxd;
	struct yk3_txi *txi;
	size_t size;

	if (!aggr->num)
		return 0;

	aggr->page = dev_alloc_pages(YK3_N_AGGR_PAGEPARAM);
	if (unlikely(!aggr->page)) {
		ret = -ENOMEM;
		goto err_alloc_pages;
	}

	size = aggr->data_size;
	size += YK3_M_AGGR_ADSIZE(aggr->num);
	aggr->dma_addr = dma_map_page(dev, aggr->page, 0, size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(dev, aggr->dma_addr))) {
		ret = -ENOMEM;
		goto err_map_page;
	}

	ad = (struct yk3_ad *)page_address(aggr->page);
	data = page_address(aggr->page) + YK3_M_AGGR_ADSIZE(aggr->num);

	for (i = 0; i < aggr->num; i++) {
		skb = aggr->skb[i];
		if (skb_copy_bits(skb, 0, data, skb->len)) {
			ret = -EFAULT;
			goto err_copy_bits;
		}

		ad->value = 0;
		yk3_ad_set_size(ad, skb->len);
		yk3_ad_set_qgroup(ad, txq->qgroup);
		ad->local_priority = txq->qgroup & 0x7;

		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			if (skb->encapsulation) {
				ad->outer_l3_csum_en = 1;
				if (skb_is_gso(skb) && (skb_shinfo(skb)->gso_type &
				    (SKB_GSO_UDP_TUNNEL_CSUM | SKB_GSO_GRE_CSUM)))
					ad->outer_l4_csum_en = 1;
				ad->inner_l3_csum_en = 1;
				ad->inner_l4_csum_en = 1;
			} else {
				ad->outer_l3_csum_en = 1;
				ad->outer_l4_csum_en = 1;
			}
		}

		if (skb_vlan_tag_present(skb)) {
			txq->stats_sw.num_vlaninsert++;
			vlan_pri = (skb_vlan_tag_get(skb) & VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
			is_8021ad =  (skb->vlan_proto == htons(ETH_P_8021AD)) ? true : false;
			yk3_ad_set_vlan(ad, skb_vlan_tag_get(skb), vlan_pri, is_8021ad);
		}

		data += YK3_M_AGGR_ALIGN(skb->len);
		ad++;
	}

	atxd = txq->atxd + yk3_ringb_head(&txq->txdrb);
	txi = txq->txi + yk3_ringb_head(&txq->txdrb);

	atxd->addr = cpu_to_le64(aggr->dma_addr);
	atxd->value = 0;

	atxd->pktaggr = 1;
	atxd->pkt_num = aggr->num - 1;

	yk3_atxd_set_size(atxd, size);
	yk3_atxd_set_payloadsize(atxd, aggr->data_bytes);

	atxd->interrupt = 1;
	atxd->fd = 1;
	atxd->ld = 1;

	txi->flags = YK3_N_FLAG_AGGR;
	txi->a.skb = aggr->skb;
	txi->a.page = aggr->page;
	txi->a.num = aggr->num;
	txi->a.addr = aggr->dma_addr;
	txi->a.len = size;

	yk3_ringb_push(&txq->txdrb);

	txq->stats_sw.num_txd++;
	txq->stats_sw.num_txdfd++;
	txq->stats_sw.num_txdld++;

	txq->stats_sw.num_aggr_pkts += aggr->num;
	txq->stats_sw.num_aggr_bytes += aggr->data_bytes;
	txq->stats_sw.num_aggr_xmit++;

	return 0;

err_copy_bits:
	size = aggr->data_size;
	size += YK3_M_AGGR_ADSIZE(aggr->num);
	dma_unmap_page(dev, aggr->dma_addr, size, DMA_TO_DEVICE);
err_map_page:
	__free_pages(aggr->page, YK3_N_AGGR_PAGEPARAM);
err_alloc_pages:
	return ret;
}

static struct sk_buff *yk3_aggr_start_xmit(struct yk3_txq *txq, struct sk_buff *skb)
{
	struct yk3_aggr *aggr = &txq->aggr;
	bool xmit_more = netdev_xmit_more();
	int i;
	struct sk_buff *skb_ret = skb;

	/* 1. check */
	if (!aggr->num && !xmit_more)
		return skb;

	if (skb_is_gso(skb) || skb->len > YK3_N_AGGR_PKT_MAXSIZE ||
	    aggr->num >= YK3_N_AGGR_PKT_MAXNUM)
		goto start_xmit;

	if ((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP))
		goto start_xmit;

	if (!aggr->num) {
		aggr->skb = kmem_cache_alloc(aggr->cache, GFP_ATOMIC);
		if (!aggr->skb)
			goto start_xmit;
	}

	aggr->data_size += YK3_M_AGGR_ALIGN(skb->len);
	aggr->data_bytes += skb->len;
	aggr->skb[aggr->num] = skb;
	aggr->num++;

	if (aggr->num < YK3_N_AGGR_PKT_MAXNUM && xmit_more)
		return NULL;

	skb_ret = NULL;

start_xmit:
	if (!aggr->num)
		return skb_ret;

	if (yk3_aggr_xmit(txq)) {
		txq->stats_base.drops += aggr->num;
		txq->stats_sw.err_aggr_xmit++;
		txq->stats_sw.err_aggr_pkts += aggr->num;
		txq->stats_sw.err_aggr_bytes += aggr->data_bytes;

		for (i = 0; i < aggr->num; i++) {
			skb = aggr->skb[i];
			dev_kfree_skb_any(skb);
		}
		kmem_cache_free(aggr->cache, aggr->skb);
	}

	aggr->skb = NULL;
	aggr->page = NULL;
	aggr->dma_addr = 0;
	aggr->num = 0;
	aggr->data_bytes = 0;
	aggr->data_size = 0;

	return skb_ret;
}

netdev_tx_t yk3_edma_start_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	int ret;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_txq *txq = NULL;
	struct yk3_scatter *scatter = NULL;
	struct yk3_scatter_iter iter_value;
	struct yk3_scatter_iter *iter = &iter_value;
	struct yk3_ctld *ctld;
	struct yk3_txd *txd = NULL;
	struct yk3_txi *txi = NULL;
	bool ptp_sync = false;
	bool xmit_more;
	u16 vlan_pri;
	bool is_8021ad;
	bool mangleid;
	u16 tmp_gso_size;
	struct sk_buff *skb2;
	u16 segs;

	/* 1. check */
	if (unlikely(!(ndev->flags & IFF_UP)))
		goto tx_drop;

	txq = ndev_priv->qpair[skb_get_queue_mapping(skb)].txq;
	if (!txq->active)
		goto tx_drop;

	/* 1. check */
	if (unlikely(yk3_txq_is_full(txq))) {
		if (!netif_tx_queue_stopped(txq->tx_queue)) {
			txq->stats_sw.num_qstop++;
			netif_tx_stop_queue(txq->tx_queue);
		}
		goto tx_busy;
	}

	if (unlikely(!yk3_check_size(txq, skb)))
		goto tx_drop;

	/* 2. aggr */
	skb2 = yk3_aggr_start_xmit(txq, skb);
	if (skb2) {
		skb = skb2;
	} else {
		txq->stats_base.packets++;
		txq->stats_base.bytes += skb->len;
		goto tx_out;
	}

	if ((skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP)) {
		skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
		yk3_ptp_tod_refl_clear(pdev_priv);
		ptp_sync = true;
	}

	/* 3. scatter skb */
	scatter = &txq->scatter;
	ret = yk3_scatter_construct(scatter, txq->dev, skb, txq->qpktsize_max);
	if (ret < 0) {
		if (ret == -ENOMEM)
			txq->stats_sw.err_dmasg++;
		else
			txq->stats_sw.err_scatter++;
		goto tx_drop;
	}

	/* 4. can send ? */
	ret = yk3_tx_check_scatter(txq, scatter);
	if (ret < 0) {
		yk3_scatter_destruct(scatter, txq->dev);
		goto tx_busy;
	}

	mangleid = !!(ndev->features & NETIF_F_TSO_MANGLEID);
	/* 5. for each scatter */
	for (yk3_scatter_iter_start(scatter, iter);
	     yk3_scatter_iter_cond(scatter, iter);
	     yk3_scatter_iter_next(scatter, iter)) {
	/* 6. fill tx desc */
		/* 6.0 fill dummy ? */
		if (iter->seg->fp &&
		    (yk3_ringb_bottom_left(&txq->txdrb) <
		    (iter->frag->seg_num + (iter->frag->tso_valid ? 1 : 0))) &&
		    yk3_ringb_in_bottom(&txq->txdrb))
			yk3_txq_bottom_fill_dummy(txq);

		/* 6.1 tso and seg fp, so add ctrld*/
		if (iter->frag->tso_valid && iter->seg->fp) {
			ctld = txq->ctld + yk3_ringb_head(&txq->txdrb);
			txi = txq->txi + yk3_ringb_head(&txq->txdrb);

			ctld->value1 = 0;
			ctld->value2 = 0;

			ctld->tso_en = 1;
			ctld->tso_first = iter->frag->tso_fp;
			ctld->tso_last = iter->frag->tso_lp;
			ctld->mss_num = iter->frag->mss_num;

			tmp_gso_size = (__force u16)cpu_to_le16(skb_shinfo(skb)->gso_size);
			yk3_ctld_set_mss(ctld, tmp_gso_size);
			ctld->last_offset = scatter->hdrlen;

			ctld->control = 1;
			ctld->fd = 1;
			txi->n.skb = NULL;
			yk3_ringb_push(&txq->txdrb);
			txq->stats_sw.num_txd++;
			txq->stats_sw.num_txdfd++;
		}

		/* 6.2 fill txd */
		txd = txq->txd + yk3_ringb_head(&txq->txdrb);
		txi = txq->txi + yk3_ringb_head(&txq->txdrb);

		txi->flags = YK3_N_FLAG_NORMAL;

		txd->value = 0;
		txd->addr = (__force dma_addr_t)cpu_to_le64(iter->seg->addr);
		txd->size = (__force u16)cpu_to_le16(iter->seg->len);
		txd->ppp_sysm_disable = 0;	/* ppp_sysm_disable set 1 when ptp */
		txd->tso_fixed_id = (iter->frag->tso_valid && mangleid) ? 0 : 1;
		yk3_txd_set_qgroup(txd, txq->qgroup);
		txd->local_priority = txq->qgroup & 0x7;

		if (ptp_sync) {
			txd->ptp_sync = 1;
			txd->local_priority = 5;
		}

		if (skb->ip_summed == CHECKSUM_PARTIAL) {
			if (skb->encapsulation) {
				txd->outer_l3_csum_en = 1;
				if (skb_is_gso(skb) && (skb_shinfo(skb)->gso_type &
				    (SKB_GSO_UDP_TUNNEL_CSUM | SKB_GSO_GRE_CSUM)))
					txd->outer_l4_csum_en = 1;
				txd->inner_l3_csum_en = 1;
				txd->inner_l4_csum_en = 1;
			} else {
				txd->outer_l3_csum_en = 1;
				txd->outer_l4_csum_en = 1;
			}
		}

		if (skb_vlan_tag_present(skb)) {
			if ((iter->frag->tso_valid && iter->frag->tso_fp && iter->seg->fp) ||
			    (!iter->frag->tso_valid && iter->seg->fp))
				txq->stats_sw.num_vlaninsert++;

			vlan_pri = (skb_vlan_tag_get(skb) & VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
			is_8021ad =  (skb->vlan_proto == htons(ETH_P_8021AD)) ? true : false;
			yk3_txd_set_vlan(txd, skb_vlan_tag_get(skb), vlan_pri, is_8021ad);
		}

		txd->fd = (iter->seg->fp && (!iter->frag->tso_valid)) ? 1 : 0;
		txd->ld = iter->seg->lp;

		if (txd->fd)
			txq->stats_sw.num_txdfd++;
		if (txd->ld)
			txq->stats_sw.num_txdld++;

		if (iter->unmap && iter->sctl) {
			dma_unmap_addr_set(txi, n.addr, dma_unmap_addr(iter->sctl, addr));
			dma_unmap_len_set(txi, n.len, dma_unmap_len(iter->sctl, len));
		}

		yk3_ringb_push(&txq->txdrb);
		txq->stats_sw.num_txd++;
	}

	txd->interrupt = 1;
	if (txi) {
		txi->n.skb = skb;
		if (ptp_sync)
			txi->flags |= YK3_N_FLAG_PTP;
	}
	/* 7. other */
	if (skb_is_gso(skb)) {
		segs = skb_shinfo(skb)->gso_segs;
		txq->stats_base.packets += segs;
		txq->stats_base.bytes += skb->len + (segs - 1) * scatter->hdrlen;
	} else {
		txq->stats_base.packets++;
		txq->stats_base.bytes += skb->len;
	}
	skb_tx_timestamp(skb);

tx_out:
	/* 8. doorbell */
	xmit_more = netdev_xmit_more();
	if (!xmit_more)
		yk3_txq_doorbell(txq);

	return NETDEV_TX_OK;

tx_drop:
	if (txq)
		txq->stats_base.drops++;
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
tx_busy:
	yk3_txq_doorbell(txq);
	return NETDEV_TX_BUSY;
}
