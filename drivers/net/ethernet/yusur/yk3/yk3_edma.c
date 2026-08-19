// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_edma_priv.h"
#include "yk3_qset_priv.h"

static int yk3_edma_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_edma_priv *edma_priv = pdev_priv->edma_priv;
	struct yk3_queuebase qbase;
	int i;

	if (!pdev_priv->edma_priv) {
		seq_printf(seq, "\t    %-16s : no edma info\n", pdev_priv->name);
		return 0;
	}

	qbase = edma_priv->qbase[YK3_QUEUE_T_LOCAL];
	seq_printf(seq, "\t    %-16s : %-16d\n", "local qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "local qnum", qbase.num);
	qbase = edma_priv->qbase[YK3_QUEUE_T_FUNC];
	seq_printf(seq, "\t    %-16s : %-16d\n", "func qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "func qnum", qbase.num);
	qbase = edma_priv->qbase[YK3_QUEUE_T_PF];
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf qnum", qbase.num);
	qbase = edma_priv->qbase[YK3_QUEUE_T_GLOBAL];
	seq_printf(seq, "\t    %-16s : %-16d\n", "global qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "glocal qnum", qbase.num);

	if (!yk3_pdev_is_pf(pdev_priv))
		return 0;

	seq_printf(seq, "\t    %-16s : %-16d\n", "func top", edma_priv->pf->fbase.top);
	seq_printf(seq, "\t    %-16s : %-16d\n", "func base", edma_priv->pf->fbase.base);
	qbase = edma_priv->pf->g_qbase;
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf global qstart", qbase.start);
	seq_printf(seq, "\t    %-16s : %-16d\n", "pf glocal qnum", qbase.num);

	for (i = 0; i < YK3_N_PF_MAX_FUNC; i++) {
		qbase = edma_priv->pf->fx_p_qbase[i];
		if (qbase.start == 0 && qbase.num == 0)
			continue;
		seq_printf(seq, "\tfunc%d_%-9s : %-16d\n", i, "p_qstart", qbase.start);
		seq_printf(seq, "\tfunc%d_%-9s : %-16d\n", i, "p_qnum", qbase.num);
	}

	for (i = 0; i < YK3_N_PF_MAX_FUNC; i++) {
		qbase = edma_priv->pf->fx_g_qbase[i];
		if (qbase.start == 0 && qbase.num == 0)
			continue;
		seq_printf(seq, "\tfunc%d_%-9s : %-16d\n", i, "g_qstart", qbase.start);
		seq_printf(seq, "\tfunc%d_%-9s : %-16d\n", i, "g_qnum", qbase.num);
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_edma_debugfs);

static int yk3_debug_edma_init(struct yk3_pdev_priv *pdev_priv)
{
	struct dentry *entry;

	if (!pdev_priv->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("edma", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_edma_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs edma file for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	pdev_priv->edma_priv->dbgfs_info_file = entry;

	return 0;
}

static void yk3_debug_edma_exit(struct yk3_pdev_priv *pdev_priv)
{
	debugfs_remove(pdev_priv->edma_priv->dbgfs_info_file);
	pdev_priv->edma_priv->dbgfs_info_file = NULL;
}

static int yk3_edma_card_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_card *card = seq->private;
	struct yk3_edma_info *edma_info = card->edma_info;

	if (!edma_info) {
		seq_printf(seq, "\t    %-16s : no edma info\n", card->name);
		return 0;
	}

	seq_printf(seq, "\t%-16s : %-16d\n", "dma_id", edma_info->dma_id);
	seq_printf(seq, "\t%-16s : %-16d\n", "dma_inst", edma_info->dma_inst);
	seq_printf(seq, "\t%-16s : %-16d\n", "dma_qmaxnum", edma_info->dma_qmaxnum);
	seq_printf(seq, "\t%-16s : %-16d\n", "dma_max_qsetnum", edma_info->dma_max_qsetnum);
	seq_printf(seq, "\t%-16s : %-16d\n", "dma_qset_offset", edma_info->dma_qset_offset);
	seq_printf(seq, "\t%-16s : %-16d\n", "dma_qset_qmaxnum", edma_info->dma_qset_qmaxnum);

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_edma_card_debugfs);

static int yk3_debug_edma_card_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_card *card;
	struct dentry *entry;

	card = pdev_priv->card;

	if (!pdev_priv->card || !pdev_priv->card->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("edma", 0444, card->dbgfs_dir, card,
				    &yk3_edma_card_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs edma info file for card %s", card->name);
		return -ENOMEM;
	}
	card->edma_info->dbgfs_info_file = entry;

	return 0;
}

static void yk3_debug_edma_card_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (!pdev_priv->card || !pdev_priv->card->edma_info)
		return;

	debugfs_remove(pdev_priv->card->edma_info->dbgfs_info_file);
	pdev_priv->card->edma_info->dbgfs_info_file = NULL;
}

int yk3_edma_set_queue_group(struct yk3_ndev_priv *ndev_priv, u16 queue, u8 qgroup)
{
	if (queue > ndev_priv->ndev->real_num_tx_queues)
		return -EINVAL;

	ndev_priv->qpair[queue].txq->qgroup = qgroup;

	return 0;
}

int yk3_edma_create_queues(struct yk3_ndev_priv *ndev_priv)
{
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	int ret, i;
	u32 depth;

	ndev_priv->qpair = kcalloc(total_qnum, sizeof(struct yk3_edma_qpair), GFP_KERNEL);
	if (!ndev_priv->qpair)
		return -ENOMEM;

	for (i = 0; i < total_qnum; i++) {
		if (i < txqnum) {
			depth = ndev_priv->txq_depth;
			depth = roundup_pow_of_two(depth);
			ret = yk3_create_txq(ndev_priv, i, depth);
			if (ret) {
				yk3_net_err("create txq %d failed", i);
				goto failed;
			}
		}

		if (i < rxqnum) {
			depth = ndev_priv->rxq_depth;
			depth = roundup_pow_of_two(depth);
			ret = yk3_create_rxq(ndev_priv, i, depth);
			if (ret) {
				yk3_net_err("create rxq %d failed", i);
				goto failed;
			}
		}
	}

	return 0;

failed:
	yk3_edma_destroy_queues(ndev_priv);
	return ret;
}

void yk3_edma_destroy_queues(struct yk3_ndev_priv *ndev_priv)
{
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	int i;

	for (i = 0; i < total_qnum; i++) {
		if (ndev_priv->qpair[i].txq)
			yk3_destroy_txq(ndev_priv->qpair[i].txq);
		if (ndev_priv->qpair[i].rxq)
			yk3_destroy_rxq(ndev_priv->qpair[i].rxq);
	}

	kfree(ndev_priv->qpair);
	ndev_priv->qpair = NULL;
}

int yk3_edma_start(struct yk3_ndev_priv *ndev_priv, u16 txqnum, u16 rxqnum)
{
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	int i;
	int ret;

	for (i = 0; i < total_qnum; i++) {
		if (i < rxqnum)
			yk3_activate_rxq(ndev_priv->qpair[i].rxq);

		if (i < txqnum)
			yk3_activate_txq(ndev_priv->qpair[i].txq);
	}

	ret = yk3_qset_start(ndev_priv, txqnum, rxqnum);
	if (ret)
		yk3_net_err("start qset failed");

	return ret;
}

void yk3_edma_stop(struct yk3_ndev_priv *ndev_priv)
{
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	int i;

	yk3_qset_stop(ndev_priv);

	for (i = 0; i < total_qnum; i++) {
		if (i < txqnum)
			yk3_deactivate_txq(ndev_priv->qpair[i].txq);

		if (i < rxqnum)
			yk3_deactivate_rxq(ndev_priv->qpair[i].rxq);
	}

	for (i = 0; i < total_qnum; i++) {
		if (i < txqnum)
			yk3_clean_txq(ndev_priv->qpair[i].txq);

		if (i < rxqnum)
			yk3_clean_rxq(ndev_priv->qpair[i].rxq);
	}
}

void yk3_edma_set_coal(struct yk3_ndev_priv *ndev_priv)
{
	u16 txqnum = ndev_priv->ndev->real_num_tx_queues;
	u16 rxqnum = ndev_priv->ndev->real_num_rx_queues;
	u16 total_qnum = max_t(u16, txqnum, rxqnum);
	int i;

	for (i = 0; i < total_qnum; i++) {
		if (i < txqnum)
			yk3_txcq_set_coal(ndev_priv->qpair[i].txcq,
					  ndev_priv->tx_max_coalesced_frames,
					  ndev_priv->tx_coalesce_usecs);

		if (i < rxqnum)
			yk3_rxcq_set_coal(ndev_priv->qpair[i].rxcq,
					  ndev_priv->rx_max_coalesced_frames,
					  ndev_priv->rx_coalesce_usecs);
	}
}

int yk3_edma_alloc_p_qbase(struct yk3_pdev_priv *pdev_priv, u16 qnum, struct yk3_queuebase *qbase)
{
	int i;
	u64 first;
	struct yk3_edma_priv *priv = pdev_priv->edma_priv;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	first = find_first_zero_bit(priv->pf->qbitmap, priv->pf->g_qbase.num);
	if (first >= priv->pf->g_qbase.num)
		return -ENOSPC;

	qbase->start = (u16)first;

	for (i = 0; i < qnum; i++) {
		if (test_bit(qbase->start + i, priv->pf->qbitmap))
			break;
		set_bit(qbase->start + i, priv->pf->qbitmap);
	}

	qbase->num = (u16)i;
	atomic_sub(i, &priv->pf->qfree);

	return 0;
}

void yk3_edma_free_p_qbase(struct yk3_pdev_priv *pdev_priv, struct yk3_queuebase qbase)
{
	int i;
	struct yk3_edma_priv *priv = pdev_priv->edma_priv;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	for (i = 0; i < qbase.num; i++)
		clear_bit(qbase.start + i, priv->pf->qbitmap);
	atomic_add(i, &priv->pf->qfree);
}

int yk3_edma_remain_p_qbase(struct yk3_pdev_priv *pdev_priv)
{
	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	return atomic_read(&pdev_priv->edma_priv->pf->qfree);
}

struct yk3_queuebase
yk3_edma_p_qbase_cast(struct yk3_pdev_priv *pdev_priv, enum yk3_queue_type type,
		      struct yk3_queuebase p_qbase)
{
	struct yk3_edma_priv *priv = pdev_priv->edma_priv;
	struct yk3_queuebase qbase = {0};

	switch (type) {
	case YK3_QUEUE_T_LOCAL:
	case YK3_QUEUE_T_FUNC:
	case YK3_QUEUE_T_GLOBAL:
		qbase = priv->qbase[type];
		qbase.start += p_qbase.start - priv->qbase[YK3_QUEUE_T_PF].start;
		qbase.num = p_qbase.num;
		break;
	case YK3_QUEUE_T_PF:
		qbase = p_qbase;
		break;
	default:
		yk3_dev_err("invalid queue type %d", type);
		break;
	}

	return qbase;
}

int yk3_edma_get_qbase(struct yk3_pdev_priv *pdev_priv, enum yk3_queue_type type,
		       struct yk3_queuebase *qbase)
{
	struct yk3_edma_priv *priv = pdev_priv->edma_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg mbox_ack_msg = {0};
	struct yk3_edma_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret;

	if (yk3_pdev_is_pf(pdev_priv)) {
		switch (type) {
		case YK3_QUEUE_T_LOCAL:
		case YK3_QUEUE_T_FUNC:
		case YK3_QUEUE_T_PF:
		case YK3_QUEUE_T_GLOBAL:
			*qbase = priv->qbase[type];
			break;
		default:
			return -EINVAL;
		}
	} else {
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_msg.opcode = YK3_MBOX_OPCODE_EDMA;
		msg = (struct yk3_edma_msg *)mbox_msg.data;
		msg->msgid = MSG_QBASE_GET;
		msg->qbase_get.req.type = type;
		msg->qbase_get.req.vf_id = pdev_priv->vf_id;

		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;
		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
		if (ret) {
			yk3_dev_err("edma send msg failed");
			return ret;
		}

		msg = (struct yk3_edma_msg *)mbox_ack_msg.data;
		*qbase = msg->qbase_get.rsp.qbase;
	}

	return 0;
}

void yk3_edma_set_fx_qbase(struct yk3_pdev_priv *pdev_priv, u16 vf_id,
			   struct yk3_queuebase p_qbase)
{
	struct yk3_edma_priv *priv;
	struct yk3_queuebase qbase;
	u32 val;

	if (!yk3_pdev_is_pf(pdev_priv))
		return;

	priv = pdev_priv->edma_priv;
	priv->pf->fx_p_qbase[vf_id] = p_qbase;

	qbase = priv->pf->g_qbase;
	qbase.start += p_qbase.start;
	qbase.num = p_qbase.num;
	priv->pf->fx_g_qbase[vf_id] = qbase;

	val = FIELD_PREP(YK3_RE_DMA_FUNC_QSTART_GMASK, p_qbase.start);
	val |= FIELD_PREP(YK3_RE_DMA_FUNC_QNUM_GMASK, p_qbase.num);

	yk3_wr32(priv->hw_addr, YK3_RE_DMA_FUNCX_QBASE(vf_id), val);
}

void yk3_edma_update_stat(struct yk3_ndev_priv *ndev_priv)
{
	struct net_device *ndev = ndev_priv->ndev;
	struct yk3_txq *txq;
	struct yk3_rxcq *rxcq;

	u64 packets, bytes, errors, drops;
	int i;

	packets = 0;
	bytes = 0;
	errors = 0;
	drops = 0;
	for (i = 0; i < ndev->real_num_tx_queues; i++) {
		txq = ndev_priv->qpair[i].txq;

		if (txq) {
			packets += READ_ONCE(txq->stats_base.packets);
			bytes += READ_ONCE(txq->stats_base.bytes);
			errors += READ_ONCE(txq->stats_base.errors);
			drops += READ_ONCE(txq->stats_base.drops);
		}
	}
	ndev->stats.tx_packets = packets;
	ndev->stats.tx_bytes = bytes;
	ndev->stats.tx_errors = errors;
	ndev->stats.tx_dropped = drops;

	packets = 0;
	bytes = 0;
	errors = 0;
	drops = 0;
	for (i = 0; i < ndev->real_num_rx_queues; i++) {
		rxcq = ndev_priv->qpair[i].rxcq;

		if (rxcq) {
			packets += READ_ONCE(rxcq->stats_base.packets);
			bytes += READ_ONCE(rxcq->stats_base.bytes);
			errors += READ_ONCE(rxcq->stats_base.errors);
			drops += READ_ONCE(rxcq->stats_base.drops);
		}
	}

	ndev->stats.rx_packets = packets;
	ndev->stats.rx_bytes = bytes;
	ndev->stats.rx_errors = errors;
	ndev->stats.rx_dropped = drops;
}

static const struct yk3_statbase yk3_rxq_stats[] = {
	YK3_STAT("packets", 0, offsetof(struct yk3_stats_base, packets), 0),
	YK3_STAT("bytes", 0, offsetof(struct yk3_stats_base, bytes), 0),
	YK3_STAT("errors", 0, offsetof(struct yk3_stats_base, errors), 0),
	YK3_STAT("drops", 0, offsetof(struct yk3_stats_base, drops), 0),
	YK3_STAT("csum_offload_errors", 0, offsetof(struct yk3_rxc_stats_sw, err_csum_pkt), 1),
	YK3_STAT("csum_offload_good", 0, offsetof(struct yk3_rxc_stats_sw, num_csum_pkt), 1),
	YK3_STAT("vlanremove", 0, offsetof(struct yk3_rxc_stats_sw, num_vlan_remove), 1),
	YK3_STAT("lro_packets", 0, offsetof(struct yk3_rxc_stats_sw, num_lro_pkt), 1),
};

static const struct yk3_statbase yk3_txq_stats[] = {
	YK3_STAT("packets", 0, offsetof(struct yk3_stats_base, packets), 0),
	YK3_STAT("bytes", 0, offsetof(struct yk3_stats_base, bytes), 0),
	YK3_STAT("errors", 0, offsetof(struct yk3_stats_base, errors), 0),
	YK3_STAT("drops", 0, offsetof(struct yk3_stats_base, drops), 0),
	YK3_STAT("vlaninsert", 1, offsetof(struct yk3_tx_stats_sw, num_vlaninsert), 1),
};

static const struct yk3_statbase yk3_rx_hw_total_stats[] = {
	YK3_STAT("hw_packets", 0x20000, 0xCB0, 8),
};

static const struct yk3_statbase yk3_rx_hw_q0_stats[] = {
	YK3_STAT("hw_packets", YK3_RE_HWQ_RXCNT(0), 0x0, 8),
};

static const struct yk3_statbase yk3_rxdrop_hw_q0_stats[] = {
	YK3_STAT("hw_drops", YK3_RE_HWQ_RXDROPCNT(0), 0x0, 8),
};

static const struct yk3_statbase yk3_rxother_hw_stats[] = {
	YK3_STAT("hw_rx_err_norxd", 0x30000, 0x210, 8),
	YK3_STAT("hw_rx_err_only_md", 0x30000, 0x580, 8),
	YK3_STAT("hw_rx_err_oversize", 0x30000, 0x5b0, 8),
	YK3_STAT("hw_rx_err_noeop_0", 0x0, 0x78, 4),
	YK3_STAT("hw_rx_err_nosop_0", 0x0, 0x7c, 4),
	YK3_STAT("hw_rx_pfc_status", 0x0, 0x7e8, 4),
	YK3_STAT("hw_st_apb_timeout", 0x30000, 0x218, 8),
};

static const struct yk3_statbase yk3_tx_hw_total_stats[] = {
	YK3_STAT("hw_packets", 0x20000, 0xC80, 8),
};

static const struct yk3_statbase yk3_tx_hw_q0_stats[] = {
	YK3_STAT("hw_packets", YK3_RE_HWQ_TXCNT(0), 0x0, 8),
};

static const struct yk3_statbase yk3_tx_hqos_schedule_stats[] = {
	YK3_STAT("hw_tx_hqos_schedule", 0x30000, 0x7b8, 8),
};

/* Per-TC statistics names for QoS monitoring */
static const char yk3_tc_stats_tx_packets[] = "tc%d_tx_packets";
static const char yk3_tc_stats_tx_bytes[] = "tc%d_tx_bytes";

#define YK3_STAT_READ64(ptr, stat, i)	\
	READ_ONCE(*(u64 *)((u8 *)(ptr) + (stat)[(i)].offset))

static inline u64
yk3_stat_rdreg(struct yk3_pdev_priv *pdev_priv, const struct yk3_statbase *stat, int i)
{
	if (stat->resv == 8)
		return yk3_rd64(pdev_priv->bar_addr[0], stat->base + stat->offset + i * 8);
	else
		return yk3_rd32(pdev_priv->bar_addr[0], stat->base + stat->offset + i * 4);
}

int yk3_edma_et_get_sset_count(struct yk3_ndev_priv *ndev_priv)
{
	int rxq_count;
	int txq_count;
	int tx_hw_count = 0;
	int rx_hw_count = 0;
	int tc_count = 0;
	int num_tc;

	rxq_count = (ndev_priv->ndev->real_num_rx_queues + 1);
	rxq_count *= ARRAY_SIZE(yk3_rxq_stats);
	/* rss */
	rxq_count += 4 * ndev_priv->ndev->real_num_rx_queues;

	txq_count = (ndev_priv->ndev->real_num_tx_queues + 1);
	txq_count *= ARRAY_SIZE(yk3_txq_stats);

	if (yk3_pdev_is_pf(ndev_priv->pdev_priv) &&
	    ndev_priv->type == YK3_NDEV_T_PF) {
		tx_hw_count = 1 + YK3_N_TOTAL_QNUM + 1;
		rx_hw_count = 1 + YK3_N_TOTAL_QNUM * 2 + 6 + 1;
	}

	/* Per-TC statistics when MQPRIO is configured */
	num_tc = netdev_get_num_tc(ndev_priv->ndev);
	if (num_tc > 1)
		tc_count = num_tc * 2;  /* tx_packets, tx_bytes per TC */

	return rxq_count + txq_count + rx_hw_count + tx_hw_count + tc_count;
}

void yk3_edma_et_get_strings(struct yk3_ndev_priv *ndev_priv, u8 *data, u8 **tail)
{
	u8 *cur;
	int i, j;

	cur = data;

	/* rx sum */
	for (i = 0; i < ARRAY_SIZE(yk3_rxq_stats); i++) {
		snprintf(cur, ETH_GSTRING_LEN, "rx_%s", yk3_rxq_stats[i].name);
		cur += ETH_GSTRING_LEN;
	}

	/* tx sum */
	for (i = 0; i < ARRAY_SIZE(yk3_txq_stats); i++) {
		snprintf(cur, ETH_GSTRING_LEN, "tx_%s", yk3_txq_stats[i].name);
		cur += ETH_GSTRING_LEN;
	}

	/* rx queues */
	for (i = 0; i < ARRAY_SIZE(yk3_rxq_stats); i++) {
		for (j = 0; j < ndev_priv->ndev->real_num_rx_queues; j++) {
			snprintf(cur, ETH_GSTRING_LEN, "rx%d_%s", j, yk3_rxq_stats[i].name);
			cur += ETH_GSTRING_LEN;
		}
	}
	/* rss */
	for (i = 0; i < 4 * ndev_priv->ndev->real_num_rx_queues; i++) {
		snprintf(cur, ETH_GSTRING_LEN, "rss_indir_%d", i);
		cur += ETH_GSTRING_LEN;
	}

	/* tx queues */
	for (i = 0; i < ARRAY_SIZE(yk3_txq_stats); i++) {
		for (j = 0; j < ndev_priv->ndev->real_num_tx_queues; j++) {
			snprintf(cur, ETH_GSTRING_LEN, "tx%d_%s", j, yk3_txq_stats[i].name);
			cur += ETH_GSTRING_LEN;
		}
	}

	/* Per-TC statistics when MQPRIO is configured */
	{
		int num_tc = netdev_get_num_tc(ndev_priv->ndev);

		if (num_tc > 1) {
			for (i = 0; i < num_tc; i++) {
				snprintf(cur, ETH_GSTRING_LEN, yk3_tc_stats_tx_packets, i);
				cur += ETH_GSTRING_LEN;
			}
			for (i = 0; i < num_tc; i++) {
				snprintf(cur, ETH_GSTRING_LEN, yk3_tc_stats_tx_bytes, i);
				cur += ETH_GSTRING_LEN;
			}
		}
	}

	if (!yk3_pdev_is_pf(ndev_priv->pdev_priv)) {
		*tail = cur;
		return;
	}

	/* rx hw total */
	snprintf(cur, ETH_GSTRING_LEN, "rx_hw_total_packets");
	cur += ETH_GSTRING_LEN;

	/* rx hw qx */
	for (i = 0; i < YK3_N_TOTAL_QNUM; i++) {
		snprintf(cur, ETH_GSTRING_LEN, "rx%d_hw_packets", i);
		cur += ETH_GSTRING_LEN;
	}

	for (i = 0; i < YK3_N_TOTAL_QNUM; i++) {
		snprintf(cur, ETH_GSTRING_LEN, "rx%d_drop_hw_packets", i);
		cur += ETH_GSTRING_LEN;
	}

	/* rx hw other */
	for (i = 0; i < ARRAY_SIZE(yk3_rxother_hw_stats); i++) {
		snprintf(cur, ETH_GSTRING_LEN, "%s", yk3_rxother_hw_stats[i].name);
		cur += ETH_GSTRING_LEN;
	}

	/* tx hw total */
	snprintf(cur, ETH_GSTRING_LEN, "tx_hw_total_packets");
	cur += ETH_GSTRING_LEN;

	/* tx hw qx */
	for (i = 0; i < YK3_N_TOTAL_QNUM; i++) {
		snprintf(cur, ETH_GSTRING_LEN, "tx%d_hw_packets", i);
		cur += ETH_GSTRING_LEN;
	}

	/* tx hw other */
	snprintf(cur, ETH_GSTRING_LEN, "%s", yk3_tx_hqos_schedule_stats[0].name);
	cur += ETH_GSTRING_LEN;
	*tail = cur;
}

int yk3_edma_et_get_stats(struct yk3_ndev_priv *ndev_priv, u64 *data)
{
	struct yk3_rxcq *rxcq;
	struct yk3_txq *txq;
	int i, j;
	u64 *cur, *tmp, val;
	u64 rx_sum[ARRAY_SIZE(yk3_rxq_stats)] = {0};
	u64 tx_sum[ARRAY_SIZE(yk3_txq_stats)] = {0};
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	cur = data + ARRAY_SIZE(yk3_rxq_stats) + ARRAY_SIZE(yk3_txq_stats);

	/* rx queues */
	for (i = 0; i < ARRAY_SIZE(yk3_rxq_stats); i++) {
		for (j = 0; j < ndev_priv->ndev->real_num_rx_queues; j++) {
			rxcq = ndev_priv->qpair[j].rxcq;
			if (!yk3_rxq_stats[i].resv)
				*cur = YK3_STAT_READ64(&rxcq->stats_base, yk3_rxq_stats, i);
			else
				*cur = YK3_STAT_READ64(&rxcq->stats_sw, yk3_rxq_stats, i);
			rx_sum[i] += *cur;
			cur++;
		}
	}

	/* rss */
	for (i = 0; i < 4 * ndev_priv->ndev->real_num_rx_queues; i++) {
		val = 0;
		for (j = 0; j < ndev_priv->ndev->real_num_rx_queues; j++)
			val += READ_ONCE(ndev_priv->qpair[j].rxcq->stats_rss_indir
							.num_rss_indir_idx[i]);
		*cur = val;
		cur++;
	}

	/* tx queues */
	for (i = 0; i < ARRAY_SIZE(yk3_txq_stats); i++) {
		for (j = 0; j < ndev_priv->ndev->real_num_tx_queues; j++) {
			txq = ndev_priv->qpair[j].txq;
			if (!yk3_txq_stats[i].resv)
				*cur = YK3_STAT_READ64(&txq->stats_base, yk3_txq_stats, i);
			else
				*cur = YK3_STAT_READ64(&txq->stats_sw, yk3_txq_stats, i);
			tx_sum[i] += *cur;
			cur++;
		}
	}

	/* Per-TC statistics: aggregate per-queue stats based on TC mapping */
	{
		struct net_device *ndev = ndev_priv->ndev;
		int num_tc = netdev_get_num_tc(ndev);

		if (num_tc > 1) {
			u64 tc_tx_packets[YK3_MAX_TC] = {0};
			u64 tc_tx_bytes[YK3_MAX_TC] = {0};
			int tc, q;
			u16 tc_offset, tc_count;

			/* Aggregate TX stats per TC using netdev TC queue mapping */
			for (tc = 0; tc < num_tc; tc++) {
				tc_count = ndev->tc_to_txq[tc].count;
				tc_offset = ndev->tc_to_txq[tc].offset;
				for (q = tc_offset; q < tc_offset + tc_count &&
				     q < ndev->real_num_tx_queues; q++) {
					txq = ndev_priv->qpair[q].txq;
					tc_tx_packets[tc] += READ_ONCE(txq->stats_base.packets);
					tc_tx_bytes[tc] += READ_ONCE(txq->stats_base.bytes);
				}
			}

			/* Output TC stats */
			for (i = 0; i < num_tc; i++)
				*cur++ = tc_tx_packets[i];
			for (i = 0; i < num_tc; i++)
				*cur++ = tc_tx_bytes[i];
		}
	}

	tmp = cur;
	cur = data;
	for (i = 0; i < ARRAY_SIZE(yk3_rxq_stats); i++)
		*cur++ = rx_sum[i];

	for (i = 0; i < ARRAY_SIZE(yk3_txq_stats); i++)
		*cur++ = tx_sum[i];

	if (!yk3_pdev_is_pf(ndev_priv->pdev_priv))
		return 0;

	/* rx hw total */
	cur = tmp;
	for (i = 0, val = 0; i < 6; i++)
		val += yk3_stat_rdreg(pdev_priv, yk3_rx_hw_total_stats, i);
	*cur++ = val;

	/* rx hw qx */
	for (i = 0; i < YK3_N_TOTAL_QNUM; i++)
		*cur++ = yk3_stat_rdreg(pdev_priv, yk3_rx_hw_q0_stats, i);

	for (i = 0; i < YK3_N_TOTAL_QNUM; i++)
		*cur++ = yk3_stat_rdreg(pdev_priv, yk3_rxdrop_hw_q0_stats, i);

	/* rx hw other */
	for (i = 0; i < ARRAY_SIZE(yk3_rxother_hw_stats); i++)
		*cur++ = yk3_stat_rdreg(pdev_priv, yk3_rxother_hw_stats + i, 0);

	/* tx hw total */
	for (i = 0, val = 0; i < 6; i++)
		val += yk3_stat_rdreg(pdev_priv, yk3_tx_hw_total_stats, i);
	*cur++ = val;

	/* tx hw qx */
	for (i = 0; i < YK3_N_TOTAL_QNUM; i++)
		*cur++ = yk3_stat_rdreg(pdev_priv, yk3_tx_hw_q0_stats, i);

	/* tx hw other */
	*cur++ = yk3_stat_rdreg(pdev_priv, yk3_tx_hqos_schedule_stats, 0);

	return 0;
}

static int yk3_get_cir_cbs(u32 speed, u32 rate, u32 *cir, u32 *cbs)
{
	switch (speed) {
	case SPEED_10G:
		if (cir)
			*cir = rate;
		if (cbs)
			*cbs = 10 * 1000 * 1000 / 8 / 4;
		break;
	case SPEED_25G:
		if (cir)
			*cir = rate;
		if (cbs)
			*cbs = 25 * 1000 * 1000 / 8 / 4;
		break;
	case SPEED_100G:
		if (cir)
			*cir = rate;
		if (cbs)
			*cbs = 100 * 1000 * 1000 / 8 / 4;
		break;
	case SPEED_10M:
	case SPEED_100M:
	case SPEED_1G:
	case SPEED_40G:
	case SPEED_50G:
	case SPEED_AUTO:
	default:
		return -EINVAL;
	}

	return 0;
}

static int yk3_mdrop_set_rate(struct yk3_pdev_priv *pdev_priv, u16 qsetid, u32 speed, u32 rate)
{
	u32 cir_value, cbs_value, val;
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RE_BASE;

	if (yk3_get_cir_cbs(speed, rate, &cir_value, &cbs_value) < 0)
		return -EINVAL;

	if (!rate) {
		cir_value = YK3_N_RM_CIR_MAX;
		cbs_value = YK3_RM_CBS_GMASK;
	} else {
		if (rate > speed || rate > YK3_N_RM_CIR_MAX)
			return -EINVAL;
		cbs_value -= cir_value * 125;

		if (!cbs_value)
			cbs_value = 9600;
	}

	yk3_wr32(hw_addr, YK3_RM_CIR(qsetid), cir_value);
	yk3_wr32(hw_addr, YK3_RM_CBS(qsetid), cbs_value);

	val = yk3_rd32(hw_addr, YK3_RM_STATUS);
	if (FIELD_GET(YK3_RM_STATUS_BYPASS_GMASK, val) ||
	    FIELD_GET(YK3_RM_STATUS_ACTIVE_GMASK, val)) {
		val |= FIELD_PREP(YK3_RM_STATUS_ACTIVE_GMASK, 1);
		val &= ~FIELD_PREP(YK3_RM_STATUS_BYPASS_GMASK, 1);
		yk3_wr32(hw_addr, YK3_RM_STATUS, val);
	}

	return 0;
}

int yk3_edma_set_rx_rate(struct yk3_ndev_priv *ndev_priv, u32 rate)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	u32 link_speed = ndev_priv->link_speed;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg mbox_ack_msg = {0};
	struct yk3_edma_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret;

	if (!link_speed)
		link_speed = SPEED_100G;

	if (rate > link_speed)
		return -EINVAL;

	if (yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	if (yk3_pdev_is_pf(pdev_priv)) {
		ret = yk3_mdrop_set_rate(pdev_priv, ndev_priv->qsetid, link_speed, rate);
		if (ret) {
			yk3_net_err("edma set rate failed, ret %d", ret);
			goto err_out;
		}
		goto ok_out;
	}

	mbox_msg.dst_id = yk3_mbox_pf_id(0);
	mbox_msg.opcode = YK3_MBOX_OPCODE_EDMA;
	msg = (struct yk3_edma_msg *)mbox_msg.data;
	msg->msgid = MSG_MDROP_SET_RATE;
	msg->mdrop_rate.req.qsetid = ndev_priv->qsetid;
	msg->mdrop_rate.req.rate = rate;
	msg->mdrop_rate.req.speed = link_speed;

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;
	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
	if (ret) {
		yk3_net_err("edma send msg failed, ret %d", ret);
		goto err_out;
	}

	msg = (struct yk3_edma_msg *)mbox_ack_msg.data;
	ret = msg->mdrop_rate.rsp.ret;
	if (ret) {
		yk3_net_err("edma set rate failed, ret %d", ret);
		goto err_out;
	}
ok_out:
	ndev_priv->mdrop_rx_rate = rate;
	return 0;

err_out:
	return ret;
}

static void yk3_edma_mbox_cb(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = param;
	struct yk3_edma_priv *priv;
	struct yk3_edma_msg *req, *rsp;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt;
	struct yk3_queuebase qbase;
	int ret;

	if (msg->opcode != YK3_MBOX_OPCODE_EDMA) {
		yk3_dev_err("edma mbox cb, opcode mismatch");
		return;
	}

	req = (struct yk3_edma_msg *)msg->data;
	rsp = (struct yk3_edma_msg *)ack_msg.data;

	rsp->msgid = req->msgid;

	switch (req->msgid) {
	case MSG_QBASE_GET:
		/* should mbox but no */
		priv = pdev_priv->edma_priv;

		switch (req->qbase_get.req.type) {
		case YK3_QUEUE_T_LOCAL:
			qbase = priv->pf->fx_g_qbase[req->qbase_get.req.vf_id];
			qbase.start = 0;
			break;
		case YK3_QUEUE_T_FUNC:
			qbase = priv->pf->fx_g_qbase[req->qbase_get.req.vf_id];
			qbase.start = 0;
			break;
		case YK3_QUEUE_T_PF:
			qbase = priv->pf->fx_p_qbase[req->qbase_get.req.vf_id];
			break;
		case YK3_QUEUE_T_GLOBAL:
			qbase = priv->pf->fx_g_qbase[req->qbase_get.req.vf_id];
			break;
		default:
			return;
		}
		rsp->qbase_get.rsp.qbase = qbase;
		break;
	case MSG_MDROP_SET_RATE:
		ret = yk3_mdrop_set_rate(pdev_priv, req->mdrop_rate.req.qsetid,
					 req->mdrop_rate.req.speed, req->mdrop_rate.req.rate);
		rsp->mdrop_rate.rsp.ret = ret;
		break;
	default:
		yk3_dev_err("edma msgid not support");
		return;
	}

	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
}

int yk3_edma_early_init(struct yk3_pdev_priv *pdev_priv)
{
	void __iomem *hw_addr;
	u32 val, rdval;
	int i;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	hw_addr = pdev_priv->bar_addr[0] + YK3_RB_BASE;
	val = FIELD_PREP(YK3_RB_EDMA_GMASK, YK3_V_EDMA_BASE);
	yk3_wr32(hw_addr, YK3_RB_EDMA_BASE, val);
	val = FIELD_PREP(YK3_RB_EDMA_GMASK, YK3_V_EDMA_TOP);
	yk3_wr32(hw_addr, YK3_RB_EDMA_TOP, val);

	dma_wmb(); /* guarantee sequence */

	for (i = 0; i < 10; i++) {
		rdval = yk3_rd32(hw_addr, YK3_RB_EDMA_TOP);
		if (rdval == val)
			break;
		usleep_range(1000, 2000);
	}

	if (i >= 10) {
		yk3_dev_err("edma host filter reg write failed");
		return -EIO;
	}

	return 0;
}

int yk3_edma_init(struct yk3_pdev_priv *pdev_priv)
{
	size_t size;
	struct yk3_edma_priv *priv;
	struct yk3_edma_info *info;
	u32 val;
	int i, ret;
	u16 qnum;
	struct yk3_queuebase *qbase_ptr, *qbase_ptr2;
	struct yk3_queuebase qbase;
	void __iomem *hw_addr;
	char name[64];

	if (yk3_pdev_is_mgr(pdev_priv)) {
		info = kzalloc(sizeof(*info), GFP_KERNEL);
		if (!info)
			return -ENOMEM;

		hw_addr = pdev_priv->bar_addr[0] + YK3_RE_BASE;

		/* clear */
		for (i = 0; i < pdev_priv->card->qsetid_num; i++)
			yk3_wr32(hw_addr, YK3_RE_QSET2Q(i), 0);
		yk3_wr32(hw_addr, YK3_RE_DATAHDL_CTRL, 0);
		yk3_wr32(hw_addr, YK3_RE_DATAHDL_CTRL, 1);
		for (i = 0; i < pdev_priv->card->qnum; i++) {
			yk3_wr32(hw_addr + YK3_RE_QX_BASE(i), YK3_RE_TXQ_CTRL, 0);
			yk3_wr32(hw_addr + YK3_RE_QX_BASE(i), YK3_RE_RXQ_CTRL, YK3_V_RXQ_RXCLR);
		}
		/* clk and count */
		yk3_wr32(hw_addr, YK3_RE_CLK_EN, 3);
		yk3_wr32(hw_addr, 0x10014, 2);
		yk3_wr32(hw_addr, 0x10014, 0);
		yk3_wr32(hw_addr, 0x1001c, 2);
		yk3_wr32(hw_addr, 0x1001c, 0);
		for (i = 0; i < YK3_N_TOTAL_QNUM; i++) {
			yk3_wr64(hw_addr, YK3_RE_HWQ_RXCNT(i), 0);
			yk3_wr64(hw_addr, 0xc2000 + 0x8 * i, 0);
			yk3_wr64(hw_addr, YK3_RE_HWQ_TXCNT(i), 0);
			yk3_wr64(hw_addr, 0xc6000 + 0x8 * i, 0);
			yk3_wr64(hw_addr, 0xc8000 + 0x8 * i, 0);
			yk3_wr64(hw_addr, YK3_RE_HWQ_RXDROPCNT(i), 0);
		}

		qnum = pdev_priv->card->qnum / pdev_priv->card->pf_num;
		for (i = 0; i < pdev_priv->card->pf_num; i++) {
			val = FIELD_PREP(YK3_RE_DMA_PF_QSTART_GMASK, qnum * i);
			val |= FIELD_PREP(YK3_RE_DMA_PF_QNUM_GMASK, qnum);
			yk3_wr32(hw_addr, YK3_RE_DMA_PFX_QBASE(i), val);
		}
		val = FIELD_PREP(YK3_RE_DMA_MDROP_BYPASS_GMASK, 1);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_HEADKEEP_GMASK, 1);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_LOSECHK_GMASK, 1);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_LOSECLR_GMASK, 0);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_QIDSEL_GMASK, 0);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_MACTIVE_GMASK, 0);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_SWSCALE_GMASK, 2);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_SWBIAS_GMASK, 0);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_SWRSSFR_MASK, 0);
		val |= FIELD_PREP(YK3_RE_DMA_MDROP_MAXPKTLEN_GMASK, 9700);
		yk3_wr32(hw_addr, YK3_RE_DMA_MDROP_STATUS, val);
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_CTRL, 1);

		/* edma cfg */
		/* buffer */
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_TC_THR, 0x170009a4);
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_QUE_THR, 0x170009a4);
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_AF_THR, 0x0c001769);
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_QUE_PKT_THR, 0x0c0002e0);
		for (i = 0; i < 31; i++) {
			yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_DATAPFC_THRX(i), 0x1800);
			yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_PKTPFC_THRX(i), 0xd00);
		}

		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_DATAPFC_DIFF, 0x97);
		yk3_wr32(hw_addr, YK3_RE_DMA_DATAHDL_PKTPFC_DIFF, 0x14);
		yk3_wr32(hw_addr, YK3_RE_DMA_TXDF_AXI4_ARID, 0x5);
		yk3_wr32(hw_addr, YK3_RE_DMA_RXDF_AXI4_ARID, 0x5);

		info->dma_id = yk3_rd32(hw_addr, YK3_RE_DMA_ID);
		info->dma_inst = yk3_rd32(hw_addr, YK3_RE_DMA_INST);
		info->dma_qmaxnum = yk3_rd32(hw_addr, YK3_RE_DMA_QNUM);
		info->dma_max_qsetnum = yk3_rd32(hw_addr, YK3_RE_DMA_QSETNUM);

		val = yk3_rd32(hw_addr, YK3_RE_DMA_QSET_OFFSET);
		info->dma_qset_offset = FIELD_GET(YK3_RE_DMA_QSET_OFFSET_GMASK, val);
		info->dma_qset_qmaxnum = FIELD_GET(YK3_RE_DMA_QSET_QMAXNUM_GMASK, val);
		info->dma_qset_qmaxnum = (1 << info->dma_qset_qmaxnum) - 1;

		pdev_priv->card->edma_info = info;

		ret = yk3_debug_edma_card_init(pdev_priv);
		if (ret) {
			yk3_dev_err("debug edma card init failed\n");
			pdev_priv->card->edma_info = NULL;
			kfree(info);
			return ret;
		}

		return 0;
	}

	if (yk3_pdev_is_pf(pdev_priv))
		size = sizeof(struct yk3_edma_priv) + sizeof(struct yk3_edma_pf);
	else
		size = sizeof(struct yk3_edma_priv);

	priv = kzalloc(size, GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	snprintf(name, sizeof(name), "pf%dvf%d_cd_head_pool", pdev_priv->pf_id, pdev_priv->vf_id);
	priv->cd_head_pool = dma_pool_create(name, pdev_priv->dev, sizeof(__le16),
					     dma_get_cache_alignment(), 0);
	if (!priv->cd_head_pool) {
		kfree(priv);
		return -ENOMEM;
	}

	priv->hw_addr = pdev_priv->bar_addr[0] + YK3_RE_BASE;
	pdev_priv->edma_priv = priv;

	if (yk3_pdev_is_pf(pdev_priv)) {
		val = yk3_rd32(priv->hw_addr, YK3_RE_DMA_PFX_FNUM(pdev_priv->pf_id));
		priv->pf->fbase.top = FIELD_GET(YK3_RE_DMA_PF_FTOP_GMASK, val);
		priv->pf->fbase.base = FIELD_GET(YK3_RE_DMA_PF_FBASE_GMASK, val);

		val = yk3_rd32(priv->hw_addr, YK3_RE_DMA_PFX_QBASE(pdev_priv->pf_id));
		priv->pf->g_qbase.start = FIELD_GET(YK3_RE_DMA_PF_QSTART_GMASK, val);
		priv->pf->g_qbase.num = FIELD_GET(YK3_RE_DMA_PF_QNUM_GMASK, val);

		for (i = 0; i < YK3_N_PF_MAX_FUNC; i++) {
			val = yk3_rd32(priv->hw_addr, YK3_RE_DMA_FUNCX_QBASE(i));
			if (yk3_reg_err(val))
				continue;
			qbase_ptr = &priv->pf->fx_p_qbase[i];
			qbase_ptr->start = FIELD_GET(YK3_RE_DMA_FUNC_QSTART_GMASK, val);
			qbase_ptr->num = FIELD_GET(YK3_RE_DMA_FUNC_QNUM_GMASK, val);

			if (qbase_ptr->start || qbase_ptr->num) {
				qbase_ptr2 = &priv->pf->fx_g_qbase[i];
				qbase_ptr2->start = qbase_ptr->start;
				qbase_ptr2->start += priv->qbase[YK3_QUEUE_T_GLOBAL].start;
				qbase_ptr2->num = qbase_ptr->num;
			}
		}

		qbase = priv->pf->g_qbase;
		priv->qbase[YK3_QUEUE_T_GLOBAL] = qbase;

		qbase.start = 0;
		priv->qbase[YK3_QUEUE_T_FUNC] = qbase;
		priv->qbase[YK3_QUEUE_T_PF] = qbase;
		priv->qbase[YK3_QUEUE_T_LOCAL] = qbase;

		yk3_edma_set_fx_qbase(pdev_priv, 0, priv->qbase[YK3_QUEUE_T_PF]);
		bitmap_zero(priv->pf->qbitmap, YK3_N_EDMA_QNUM);
		atomic_set(&priv->pf->qfree, priv->pf->g_qbase.num);

		ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_EDMA,
						 yk3_edma_mbox_cb, pdev_priv);
		if (ret) {
			yk3_dev_err("register edma mbox callback failed");
			goto failed;
		}
	} else {
		ret = yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_LOCAL, &qbase);
		if (ret) {
			yk3_dev_err("get queuebase local failed\n");
			goto failed;
		}
		priv->qbase[YK3_QUEUE_T_LOCAL] = qbase;

		ret = yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_FUNC, &qbase);
		if (ret) {
			yk3_dev_err("get queuebase func failed\n");
			goto failed;
		}
		priv->qbase[YK3_QUEUE_T_FUNC] = qbase;

		ret = yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_PF, &qbase);
		if (ret) {
			yk3_dev_err("get queuebase pf failed\n");
			goto failed;
		}
		priv->qbase[YK3_QUEUE_T_PF] = qbase;

		ret = yk3_edma_get_qbase(pdev_priv, YK3_QUEUE_T_GLOBAL, &qbase);
		if (ret) {
			yk3_dev_err("get queuebase global failed\n");
			goto failed;
		}
		priv->qbase[YK3_QUEUE_T_GLOBAL] = qbase;
	}

	ret = yk3_debug_edma_init(pdev_priv);
	if (ret) {
		yk3_dev_err("debug edma init failed\n");
		goto failed;
	}

	return 0;
failed:
	pdev_priv->edma_priv = NULL;
	kfree(priv);
	return ret;
}

void yk3_edma_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_edma_priv *priv;
	struct yk3_edma_info *info;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		yk3_debug_edma_card_exit(pdev_priv);
		info = pdev_priv->card->edma_info;
		pdev_priv->card->edma_info = NULL;
		kfree(info);
		return;
	}

	yk3_debug_edma_exit(pdev_priv);
	if (yk3_pdev_is_pf(pdev_priv))
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_EDMA);
	priv = pdev_priv->edma_priv;
	dma_pool_destroy(priv->cd_head_pool);
	pdev_priv->edma_priv = NULL;
	kfree(priv);
}
