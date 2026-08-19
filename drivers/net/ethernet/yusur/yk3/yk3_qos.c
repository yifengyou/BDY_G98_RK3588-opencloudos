// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_qos_regs.h"
#include "yk3_emp_priv.h"

static int _yk3_dcbnl_setets(struct net_device *ndev);

static inline void set_field(u64 *word, u64 mask, u32 shift, u64 val)
{
	*word &= ~mask;
	*word |= (val << shift) & mask;
}

static inline u64 get_field(u64 word, u64 mask, u32 shift)
{
	return (word & mask) >> shift;
}

static inline u16 get_parent_id(const struct qos_node_entry *e)
{
	u64 parent_id_h, parent_id_l;

	parent_id_l = get_field(e->data[0], NEXT_CLS_ID_W0_MASK,
				NEXT_CLS_ID_W0_SHIFT);

	parent_id_h = get_field(e->data[1], NEXT_CLS_ID_W1_MASK,
				NEXT_CLS_ID_W0_SHIFT) << 4;

	return parent_id_h | parent_id_l;
}

static inline u32 get_cir_factor(const struct qos_node_entry *e)
{
	return get_field(e->data[0], CIR_FACTOR_MASK, CIR_FACTOR_SHIFT);
}

static inline u8 get_yellow_factor(const struct qos_node_entry *e)
{
	return get_field(e->data[0], YFACTOR_MASK, YFACTOR_SHIFT);
}

static inline u8 get_sched_mode(const struct qos_node_entry *e)
{
	return get_field(e->data[2], SCHED_MODE_MASK, SCHED_MODE_SHIFT);
}

static inline void qos_dump_entry(struct yk3_qos *qos, struct qos_node_entry *e)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(qos->pdev);

	yk3_dev_debug("cir_factor:%d yellow_factor:%d sched_mode:%d parent:%d\n",
		      get_cir_factor(e), get_yellow_factor(e),
		      get_sched_mode(e), get_parent_id(e));
}

static void qos_entry_read(struct yk3_qos *qos, struct yk3_qos_node *node)
{
	u32 i, value;
	void __iomem *hw_addr = qos->hw_base;

	value = ((node->layer_id + 7) << 24) | (node->node_id);
	yk3_wr32(hw_addr, QOS_ADDR_OFF, value);

	value = 0x2;
	yk3_wr32(hw_addr, QOS_VALID_OFF, value);

	//while(!(0x2 & yk3_rd32(hw_addr, QOS_RDWR_STATUS_OFF)));

	for (i = 0; i < 4; i++)
		node->entry.data[i] = yk3_rd64(hw_addr, QOS_READ_DATA + i * 8);
}

/* config HW */
static void qos_entry_write(struct yk3_qos *qos, struct qos_node_entry *entry)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(qos->pdev);
	void __iomem *hw_addr = qos->hw_base;
	u32 i;

	WARN_ON_ONCE(in_irq());
	spin_lock_bh(&qos->lock);

	for (i = 0; i < 4; i++)
		yk3_wr64(hw_addr, QOS_DATA_OFF(i), entry->data[i]);

	for (i = 0; i < 8; i++)
		yk3_wr32(hw_addr, QOS_MASK_OFF(i), entry->mask[i]);

	yk3_wr32(hw_addr, QOS_ADDR_OFF, entry->addr);

	dma_wmb();  /* guarantee sequence */
	yk3_wr32(hw_addr, QOS_VALID_OFF, 1);

	yk3_dev_debug("ADDR:0x%08x VALUE:0x%016llx 0x%016llx 0x%016llx 0x%016llx\n",
		      entry->addr, entry->data[0], entry->data[1], entry->data[2], entry->data[3]);

	//while(yk3_rd32(hw_addr, QOS_VALID_OFF));  //spin for writing over

	spin_unlock_bh(&qos->lock);
}

/*rate unit:Mbps*/
static void qos_rate_calc(u32 min_rate, u32 max_rate, struct yk3_qos *qos,
			  u32 *cir_factor, u32 *eir_factor, u32 *cbs)
{
	u32 eir_base = 0;

	if (!min_rate && !max_rate) {
		*cir_factor = ((u64)qos->link_speed << 17) / (qos->qos_clk / 1000000);
		*cbs = (100000 * 1500) >> 11;
		*eir_factor = 0;
		return;
	}

	/* cir_factor use B */
	*cir_factor = ((u64)min_rate << 17) / (qos->qos_clk / 1000000);

	if (min_rate > 0 && min_rate <= 100)
		*cbs = (min_rate * 1000 * 1500) >> 11;
	else
		*cbs = (100000 * 1500) >> 11;

	eir_base = 0x7FFFF;  //yk3_rd32(QOS_EIR_BASE_OFF(level));  //default value is 0x7FFFF
	*eir_factor = ((max_rate - min_rate) * (1000000 / 8)) / (eir_base * (qos->qos_clk >> 20));
}

static void qos_update_entry_rate(struct yk3_qos_node *node)
{
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;
	u32 cir_factor, eir_factor, cbs;

	qos_rate_calc(node->min_rate, node->max_rate, node->qos,
		      &cir_factor, &eir_factor, &cbs);

	set_field(&node->entry.data[0], CIR_FACTOR_MASK, CIR_FACTOR_SHIFT, cir_factor);

	set_field(&node->entry.data[0], YFACTOR_MASK, YFACTOR_SHIFT, eir_factor);

	set_field(&node->entry.data[0], CBS_MASK, CBS_SHIFT, cbs);

	node->entry.addr = ((node->layer_id + BUCKET_LEVEL_BASE) << 24) | (node->node_id);

	memcpy(node->entry.mask, mask, sizeof(u32) * 8);
}

static void qos_update_entry_topo(struct yk3_qos_node *node)
{
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;

	set_field(&node->entry.data[0], BUCKET_EN_MASK, BUCKET_EN_SHIFT,
		  node->in_use);

	set_field(&node->entry.data[0], NEXT_BUF_ID_MASK, NEXT_BUF_ID_SHIFT,
		  node->prio_slot);

	set_field(&node->entry.data[0], NEXT_CLS_ID_W0_MASK, NEXT_CLS_ID_W0_SHIFT,
		  node->parent->node_id);

	set_field(&node->entry.data[1], NEXT_CLS_ID_W1_MASK, NEXT_CLS_ID_W1_SHIFT,
		  node->parent->node_id >> 4);

	set_field(&node->entry.data[2], SCHED_MODE_MASK, SCHED_MODE_SHIFT,
		  node->sched_mode);

	node->entry.addr = ((node->layer_id + BUCKET_LEVEL_BASE) << 24) | (node->node_id);

	memcpy(node->entry.mask, mask, sizeof(u32) * 8);
}

/* Write scheduler entry to hardware */
static void qos_update_entry_sched(struct yk3_qos_node *node, struct qos_sched_entry *sched)
{
	u32 mask[8] = QOS_SCHED_ENTRY_MASK;

	set_field(&node->entry.data[0], SCHED_WEIGHT0_MASK, SCHED_WEIGHT0_SHIFT, sched->weight0);
	set_field(&node->entry.data[0], SCHED_WEIGHT1_MASK, SCHED_WEIGHT1_SHIFT, sched->weight1);
	set_field(&node->entry.data[0], SCHED_WEIGHT2_MASK, SCHED_WEIGHT2_SHIFT, sched->weight2);
	set_field(&node->entry.data[0], SCHED_WEIGHT3_MASK, SCHED_WEIGHT3_SHIFT, sched->weight3);
	set_field(&node->entry.data[0], SCHED_WEIGHT4_MASK, SCHED_WEIGHT4_SHIFT, sched->weight4);
	set_field(&node->entry.data[0], SCHED_WEIGHT5_MASK, SCHED_WEIGHT5_SHIFT, sched->weight5);
	set_field(&node->entry.data[0], SCHED_WEIGHT6_MASK, SCHED_WEIGHT6_SHIFT, sched->weight6);
	set_field(&node->entry.data[0], SCHED_WEIGHT7_MASK, SCHED_WEIGHT7_SHIFT, sched->weight7);

	set_field(&node->entry.data[1], SCHED_SP_EN_MASK, SCHED_SP_EN_SHIFT, sched->sp_en);
	set_field(&node->entry.data[1], SCHED_BE_EN_MASK, SCHED_BE_EN_SHIFT, sched->be_en);
	set_field(&node->entry.data[1], SCHED_ENTRY_EN_MASK, SCHED_ENTRY_EN_SHIFT,
		  sched->entry_en);
	set_field(&node->entry.data[1], SCHED_WRR_PRIO_MASK, SCHED_WRR_PRIO_SHIFT,
		  sched->wrr_priority);

	node->entry.addr = ((node->layer_id + SCHED_LEVEL_BASE) << 24) | (node->node_id);

	memcpy(node->entry.mask, mask, sizeof(u32) * 8);
}

/**
 * yk3_qos_set_qp_tc - Set Traffic Class for RDMA Queue Pair
 * @pdev_priv: PCI device private data
 * @qp: Queue Pair ID
 * @tc: Traffic Class ID (0-7)
 *
 * Maps an RDMA QP to a specific Traffic Class for QoS prioritization.
 * Typically used in RDMA over Converged Ethernet (RoCEv2) scenarios
 * where different QPs require different QoS treatment (e.g., lossless
 * traffic with PFC enabled for certain TCs).
 *
 * Hardware behavior:
 * - Sets TC_ID, PORT_ID, and PFC_ID for the queue corresponding to this QP
 * - PFC_ID = (PF_ID << 3) | TC_ID (enables priority-based flow control)
 *
 * Returns: 0 on success, negative error code on failure
 */
int yk3_qos_set_qp_tc(struct yk3_pdev_priv *pdev_priv, int qp, int tc)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret;

	/* Validate TC range */
	if (tc < 0 || tc >= QOS_TRAFFIC_CLASS) {
		yk3_dev_err("Invalid TC %d for QP %d (valid range: 0-%d)",
			    tc, qp, QOS_TRAFFIC_CLASS - 1);
		return -EINVAL;
	}

	/* Validate QP range */
	if (qp < 0 || qp >= QOS_RDMA_QP_MAXNUM) {
		yk3_dev_err("Invalid QP number %d", qp);
		return -EINVAL;
	}

	/* Only PF can configure QP TC mapping */
	if (!yk3_pdev_is_pf(pdev_priv)) {
		yk3_dev_err("Only PF can set QP TC mapping");
		return -EOPNOTSUPP;
	}

	/* Send mailbox message to MGR device */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_QP_SET_TC;
	msg->qp_tc.qp_id = qp + QOS_RDMA_QP_OFFSET;
	msg->qp_tc.tc_id = tc;
	msg->qp_tc.pf_id = pdev_priv->pf_id;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret) {
		yk3_dev_err("Failed to send QP %d TC mapping message: %d", qp, ret);
		return ret;
	}

	yk3_dev_info("RDMA QP %d mapped to TC %d (PF %d)",
		     qp, tc, pdev_priv->pf_id);
	return 0;
}
EXPORT_SYMBOL(yk3_qos_set_qp_tc);

/**
 * yk3_qos_set_qp_rate - Set rate limit for RDMA Queue Pair
 * @pdev_priv: PCI device private data
 * @qp: Queue Pair ID
 * @maxrate: Maximum rate in Mbps
 *
 * Configures per-QP rate limiting for RDMA traffic. This allows fine-grained
 * control over individual queue pair bandwidth, useful for:
 * - Preventing QP starvation
 * - Enforcing tenant SLAs in multi-tenant environments
 * - Implementing DCQCN/ECN rate control
 *
 * Hardware behavior:
 * - Configures the shaper for the L0 (queue) node corresponding to this QP
 * - Uses max_rate only (min_rate typically not needed for RDMA QPs)
 *
 * Returns: 0 on success, negative error code on failure
 */
int yk3_qos_set_qp_rate(struct yk3_pdev_priv *pdev_priv, int qp, u32 maxrate)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret;

	/* Validate QP range */
	if (qp < 0 || qp >= QOS_RDMA_QP_MAXNUM) {
		yk3_dev_err("Invalid QP number %d", qp);
		return -EINVAL;
	}

	/* Only PF can configure QP rate */
	if (!yk3_pdev_is_pf(pdev_priv)) {
		yk3_dev_err("Only PF can set QP rate");
		return -EOPNOTSUPP;
	}

	/* Send mailbox message to MGR device */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_QP_SET_RATE;
	msg->qp_rate.qp_id = qp + QOS_RDMA_QP_OFFSET;
	msg->qp_rate.maxrate = maxrate;
	msg->qp_rate.pf_id = pdev_priv->pf_id;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret) {
		yk3_dev_err("Failed to send QP %d rate limit message: %d", qp, ret);
		return ret;
	}

	yk3_dev_info("RDMA QP %d rate limited to %u Mbps (PF %d)",
		     qp, maxrate, pdev_priv->pf_id);
	return 0;
}
EXPORT_SYMBOL(yk3_qos_set_qp_rate);

void yk3_qos_init_rt_dst(struct yk3_qos *qos)
{
	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(8), 0x023f0200);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(7), 1);

	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(9), 0x027f0240);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(6), 1);

	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(10), 0x02bf0280);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(5), 0x23);

	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(11), 0x02ff02c0);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(4), 0x23);

	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(12), 0x033f0300);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(3), 2);

	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(13), 0x037f0340);
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(2), 2);
}

int yk3_qos_set_rt_dst(struct yk3_pdev_priv *pdev_priv,
		       u8 qrange_id, u32 qrange, u8 rt_dst_id, u32 rt_dst)
{
	struct yk3_card *card;
	struct yk3_mbox_msg mbox_msg = {};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {};
	int ret = 0;
	u16 q_low = qrange & 0xFFFF;
	u16 q_high = (qrange >> 16) & 0xFFFF;

	if (!pdev_priv)
		return -EINVAL;
	card = pdev_priv->card;

	if (qrange_id > 15 || rt_dst_id > 15)
		return -EINVAL;
	if (q_low < 0x200 || q_low > 0x3FF || q_high < 0x200 || q_high > 0x3FF || q_low > q_high)
		return -EINVAL;

	if (card->mode != YK3_MODE_RCARD)
		return -EPERM;

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_QP_SET_RT_DST;
	msg->rt_dst.qrange_id = qrange_id;
	msg->rt_dst.qrange = qrange;
	msg->rt_dst.rt_dst_id = rt_dst_id;
	msg->rt_dst.rt_dst = rt_dst;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret)
		yk3_dev_err("qos set vf maxrate mbox failed, ret %d", ret);

	return ret;
}
EXPORT_SYMBOL(yk3_qos_set_rt_dst);

static void yk3_qos_set_rt_dst_handle(struct yk3_pdev_priv *pdev_priv, struct qos_rt_dst rt_dst)
{
	struct yk3_qos *qos;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	yk3_wr32(qos->hw_base, QOS_ROUTINGDEST(rt_dst.rt_dst_id),
		 rt_dst.rt_dst & QOS_RT_DEST_MASK);
	yk3_wr32(qos->hw_base, QOS_RTD_QRANGE(rt_dst.qrange_id),
		 rt_dst.qrange);
}

static int yk3_qdisc_setup_mqprio_map(struct yk3_pdev_priv *pdev_priv, u32 pf_id,
				      u32 qstart, struct tc_map *mqprio_qopt)
{
	u32 i = 0, j = 0;
	struct yk3_qos *qos;
	struct yk3_qos_node *node;
	struct qos_node_entry entry = {};
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L3_NODE_HEAD + pf_id * QOS_TRAFFIC_CLASS];

	for (i = 0; i < mqprio_qopt->num_tc; i++) {
		node[i].in_use = 1;

		qos_update_entry_topo(&node[i]);
		qos_entry_write(qos, &node[i].entry);

		for (j = 0; j < mqprio_qopt->count[i]; j++) {
			set_field(&entry.data[0], TC_ID_MASK, TC_ID_SHIFT, i);
			set_field(&entry.data[0], PORT_ID_MASK, PORT_ID_SHIFT, pf_id);
			set_field(&entry.data[0], PFC_ID_MASK, PFC_ID_SHIFT, (pf_id << 3) + i);
			entry.addr = qstart + mqprio_qopt->offset[i] + j;
			memcpy(entry.mask, mask, sizeof(u32) * 8);

			qos_entry_write(qos, &entry);
		}
	}

	return 0;
}

static int yk3_qdisc_setup_mqprio_shaper(struct yk3_pdev_priv *pdev_priv, u32 pf_id,
					 u64 *rate, bool is_minrate)
{
	u32 i = 0;
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L3_NODE_HEAD + pf_id * QOS_TRAFFIC_CLASS];

	for (i = 0; node[i].in_use && i < QOS_TRAFFIC_CLASS; i++) {
		if (is_minrate)
			node[i].min_rate = rate[i] / 125000;  /* node rate unit:Mbps */
		else
			node[i].max_rate = rate[i] / 125000;  /* node rate unit:Mbps */

		node->in_use = 1;
		qos_update_entry_topo(node);
		qos_update_entry_rate(&node[i]);

		qos_entry_write(qos, &node[i].entry);
	}

	return 0;
}

static void yk3_qos_update_tc_map(struct net_device *ndev, struct tc_map *map)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	u8 tc = 0, prio = 0;
	u16 queue = 0;

	netdev_set_num_tc(ndev, map->num_tc);

	if (map->num_tc > 1)
		yk3_set_pfc_pcie_l1vq(ndev, 0x100, 0xb0);
	else
		yk3_set_pfc_pcie_l1vq(ndev, 0x200, 0x160);

	ndev_priv->tc_cfg.numtc = map->num_tc;
	for (tc = 0; tc < map->num_tc; tc++) {
		ndev_priv->tc_cfg.tc_info[tc].qoffset = map->offset[tc];
		ndev_priv->tc_cfg.tc_info[tc].qcount_tx = map->count[tc];
		ndev_priv->tc_cfg.tc_info[tc].netdev_tc = tc;
		ndev_priv->tc_cfg.ena_tc |= BIT(tc);

		netdev_set_tc_queue(ndev, tc, map->count[tc], map->offset[tc]);

		for (queue = 0; queue < map->count[tc]; queue++) {
			yk3_edma_set_queue_group(ndev_priv, queue + map->offset[tc],
						 ndev_priv->pf_id << 3 | tc);
		}
	}

	for (tc = map->num_tc; tc < QOS_TRAFFIC_CLASS; tc++) {
		ndev_priv->tc_cfg.tc_info[tc].qoffset = 0;
		ndev_priv->tc_cfg.tc_info[tc].qcount_tx = 1;
		ndev_priv->tc_cfg.tc_info[tc].netdev_tc = 0;
		ndev_priv->tc_cfg.ena_tc &= ~BIT(tc);
	}

	for (prio = 0; prio < QOS_TRAFFIC_CLASS; prio++) {
		yk3_set_prio_map_tc(ndev_priv, prio, map->prio_tc_map[prio]);
		netdev_set_prio_tc_map(ndev, prio, map->prio_tc_map[prio]);
	}
}

#ifdef YK3_MQPRIO_OFFLOAD
static void yk3_qdisc_dump_mqprio(struct yk3_pdev_priv *pdev_priv,
				  struct tc_mqprio_qopt_offload *mqprio_qopt)
{
	u8 i = 0;

	yk3_dev_info("mode:%d, shaper:%d, flags:%d, num_tc:%d, hw:%d",
		     mqprio_qopt->mode, mqprio_qopt->shaper, mqprio_qopt->flags,
		     mqprio_qopt->qopt.num_tc, mqprio_qopt->qopt.hw);

	for (i = 0; i < TC_QOPT_MAX_QUEUE; i++) {
		yk3_dev_info("prio:%d -> tc:%d", i, mqprio_qopt->qopt.prio_tc_map[i]);

		yk3_dev_info("count:%d @ offset:%d", mqprio_qopt->qopt.count[i],
			     mqprio_qopt->qopt.offset[i]);

		yk3_dev_info("min_rate[%d]:%lld, max_rate[%d]:%lld,",
			     i, mqprio_qopt->min_rate[i],
			     i, mqprio_qopt->max_rate[i]);
	}
}

/* Returns true if the intervals [a, b) and [c, d) overlap. */
static inline bool intervals_overlap(int a, int b, int c, int d)
{
	int left = max(a, c), right = min(b, d);

	return left < right;
}

/**
 * yk3_qdisc_validate_shaper - Validate MQPRIO shaper configuration
 * @ndev: Network device
 * @mqprio: MQPRIO configuration
 *
 * Validates:
 * - Shaper type is supported (only BW_RATE)
 * - min_rate <= max_rate for all TCs
 * - Rates don't exceed hardware limits
 *
 * Returns: 0 on success, negative error code on failure
 */
static int yk3_qdisc_validate_shaper(struct net_device *ndev,
				     struct tc_mqprio_qopt_offload *mqprio)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int i;
	u64 total_min_rate = 0;

	if (!mqprio->shaper)
		return 0;

	/* Only support bandwidth rate limiting */
	if (mqprio->shaper != TC_MQPRIO_SHAPER_BW_RATE) {
		yk3_dev_err("Unsupported shaper type: %d (only BW_RATE supported)",
			    mqprio->shaper);
		return -EOPNOTSUPP;
	}

	for (i = 0; i < mqprio->qopt.num_tc; i++) {
		u64 min_bps = mqprio->min_rate[i];
		u64 max_bps = mqprio->max_rate[i];
		u32 min_mbps = min_bps / 125000;  /* Byte/s -> Mbps */
		u32 max_mbps = max_bps / 125000;

		total_min_rate += min_mbps;
		if (total_min_rate > ndev_priv->link_speed) {
			yk3_dev_err("total_min_rate %llu Mbps exceeds hardware limit %u Mbps",
				    total_min_rate, ndev_priv->link_speed);
			return -EINVAL;
		}

		/* Validate min_rate <= max_rate */
		if (min_bps > 0 && max_bps > 0 && min_bps > max_bps) {
			yk3_dev_err("TC %d: min_rate %llu Byte/s > max_rate %llu Byte/s",
				    i, min_bps, max_bps);
			return -EINVAL;
		}

		/* Validate max_rate doesn't exceed hardware capability */
		if (max_mbps > ndev_priv->link_speed) {
			yk3_dev_err("TC %d: max_rate %u Mbps exceeds hardware limit %u Mbps",
				    i, max_mbps, ndev_priv->link_speed);
			return -EINVAL;
		}
	}

	return 0;
}

static int yk3_qdisc_valid_mqprio(struct net_device *ndev, struct tc_mqprio_qopt *qopt)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int i, j;

	/* Verify num_tc is not out of max range */
	if (qopt->num_tc > QOS_TRAFFIC_CLASS) {
		yk3_dev_err("Number of traffic classes is outside valid range");
		return -EINVAL;
	}

	/* Verify priority mapping uses valid tcs */
	for (i = 0; i < QOS_TRAFFIC_CLASS; i++) {
		if (qopt->prio_tc_map[i] >= qopt->num_tc) {
			yk3_dev_err("Invalid priority to traffic class mapping");
			return -EINVAL;
		}
	}

	for (i = 0; i < qopt->num_tc; i++) {
		unsigned int last = qopt->offset[i] + qopt->count[i];

		if (!qopt->count[i]) {
			yk3_dev_err("No queues for TC %d", i);
			return -EINVAL;
		}

		/* Verify the queue count is in tx range being equal to the
		 * real_num_tx_queues indicates the last queue is in use.
		 */
		if (qopt->offset[i] >= ndev->real_num_tx_queues ||
		    last > ndev->real_num_tx_queues) {
			yk3_dev_err("Queues %d:%d for TC %d exceed the %d TX queues available",
				    qopt->count[i], qopt->offset[i],
				    i, ndev->real_num_tx_queues);
			return -EINVAL;
		}

		/* Verify that the offset and counts do not overlap */
		for (j = i + 1; j < qopt->num_tc; j++) {
			if (intervals_overlap(qopt->offset[i], last, qopt->offset[j],
					      qopt->offset[j] + qopt->count[j])) {
				yk3_dev_err("TC %d queues %d@%d overlap with TC %d queues %d@%d",
					    i, qopt->count[i], qopt->offset[i],
					    j, qopt->count[j], qopt->offset[j]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

int yk3_qos_setup_tc_mqprio_qdisc(struct net_device *ndev, void *type_data)
{
	struct tc_mqprio_qopt_offload *mqprio_qopt = type_data;
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	int ret = 0, queue = 0;

	struct yk3_mbox_msg mbox_msg = {0}, mbox_ack_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	/* Handle qdisc deletion */
	if (!mqprio_qopt->qopt.num_tc) {
		yk3_dev_info("MQPRIO qdisc deletion requested");

		/* Send cleanup message to MGR to reset hardware QoS config */
		mbox_msg.dst_id = yk3_mbox_master_id();
		mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

		msg = (struct yk3_qos_msg *)mbox_msg.data;
		msg->msgid = MSG_DQISC_CLEANUP_TC;
		msg->tc_cleanup.pf_id = pdev_priv->pf_id;
		msg->tc_cleanup.g_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
		msg->tc_cleanup.qnum = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].num;

		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
		if (ret)
			yk3_dev_err("Failed to send MQPRIO cleanup message: %d", ret);

		ndev_priv->tc_cfg.numtc = 1;
		ndev_priv->tc_cfg.ena_tc = 0;

		for (queue = 0; queue < ndev_priv->ndev->real_num_tx_queues; queue++) {
			yk3_edma_set_queue_group(ndev_priv, queue,
						 ndev_priv->pf_id << 3);
		}

		/* Reset netdev TC configuration */
		netdev_reset_tc(ndev);

		clear_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags);

		if (test_bit(YK3_ETS_PENDING, ndev_priv->tc_cfg.flags)) {
			_yk3_dcbnl_setets(ndev);
			clear_bit(YK3_ETS_PENDING, ndev_priv->tc_cfg.flags);
		} else {
			yk3_set_pfc_pcie_l1vq(ndev, 0x200, 0x160);
		}

		return ret;
	}

	if (mqprio_qopt->mode != TC_MQPRIO_MODE_CHANNEL) {
		yk3_dev_err("Unsupported MQPRIO mode: %d (supported: CHANNEL=%d)",
			    mqprio_qopt->mode, TC_MQPRIO_MODE_CHANNEL);
		return -EOPNOTSUPP;
	}

	/* Validate hardware offload flag */
	if (!mqprio_qopt->qopt.hw) {
		yk3_dev_err("Software MQPRIO mode not supported. Please use hw 1 for hardware offload");
		return -EOPNOTSUPP;
	}

	/* Dump configuration for debugging */
	yk3_qdisc_dump_mqprio(pdev_priv, mqprio_qopt);

	/* Validate MQPRIO parameters */
	ret = yk3_qdisc_valid_mqprio(ndev, &mqprio_qopt->qopt);
	if (ret) {
		yk3_dev_err("MQPRIO parameter validation failed: %d", ret);
		return -EINVAL;
	}

	/* Validate shaper configuration */
	ret = yk3_qdisc_validate_shaper(ndev, mqprio_qopt);
	if (ret) {
		yk3_dev_err("Shaper validation failed: %d", ret);
		return ret;
	}

	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_DQISC_SET_TC_MAP;
	msg->tc.pf_id = pdev_priv->pf_id;
	msg->tc.g_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	msg->tc.tc_qopt.num_tc = mqprio_qopt->qopt.num_tc;
	memcpy(msg->tc.tc_qopt.prio_tc_map, mqprio_qopt->qopt.prio_tc_map,
	       sizeof(msg->tc.tc_qopt.prio_tc_map));
	memcpy(msg->tc.tc_qopt.count, mqprio_qopt->qopt.count,
	       sizeof(msg->tc.tc_qopt.count));
	memcpy(msg->tc.tc_qopt.offset, mqprio_qopt->qopt.offset,
	       sizeof(msg->tc.tc_qopt.offset));

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
	if (ret) {
		yk3_dev_err("qos set tc map mbox failed, ret %d", ret);
		return ret;
	}

	yk3_qos_update_tc_map(ndev, &msg->tc.tc_qopt);

	/* shaper rate unit:Byte/s */
	if (mqprio_qopt->shaper) {
		msg->msgid = MSG_DQISC_SET_TC_MIN_RATE;
		msg->tc_shaper_min.pf_id = pdev_priv->pf_id;
		memcpy(msg->tc_shaper_min.min_rate, mqprio_qopt->min_rate,
		       sizeof(u64) * QOS_TRAFFIC_CLASS);

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
		if (ret) {
			yk3_dev_err("qos set tc min mbox failed, ret %d", ret);
			return ret;
		}
	}

	/* shaper rate unit:Byte/s */
	if (mqprio_qopt->shaper) {
		msg->msgid = MSG_DQISC_SET_TC_MAX_RATE;
		msg->tc_shaper_max.pf_id = pdev_priv->pf_id;
		memcpy(msg->tc_shaper_max.max_rate, mqprio_qopt->max_rate,
		       sizeof(u64) * QOS_TRAFFIC_CLASS);

		opt.wait_reply = MB_WAIT_REPLY;
		opt.timeout = 1000;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
		if (ret) {
			yk3_dev_err("qos set tc max mbox failed, ret %d", ret);
			return ret;
		}
	}

	set_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags);
	if (test_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags)) {
		clear_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags);
		set_bit(YK3_ETS_PENDING, ndev_priv->tc_cfg.flags);
	}

	return 0;
}
#endif

int yk3_qos_set_link_speed(struct yk3_pdev_priv *pdev_priv, u32 speed_mbps)
{
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	int ret = 0;

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_SET_LINK_SPEED;
	msg->link_speed.pf_id = pdev_priv->pf_id;
	msg->link_speed.speed_mbps = speed_mbps;

	ret = yk3_mbox_send_msg_atomic(pdev_priv, &mbox_msg);
	if (ret) {
		yk3_dev_err("qos set link speed mbox failed, ret %d", ret);
		return -1;
	}

	return 0;
}

static void yk3_qos_set_link_speed_handle(struct yk3_pdev_priv *pdev_priv,
					  u32 pf_id, u32 minrate, u32 maxrate)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	qos->link_speed = maxrate;
	node = &qos->root[QOS_L4_NODE_HEAD + pf_id];

	node->min_rate = maxrate * qos->maxrate_ratio / 1000;
	node->max_rate = maxrate * qos->maxrate_ratio / 1000;

	qos_update_entry_rate(node);

	qos_entry_write(qos, &node->entry);
}

int yk3_qos_get_vf_rate(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg mbox_ack_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret = 0;

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_FUNC_GET_RATE;
	msg->func_rate.req.qset_id = ndev_priv->qsetid;

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
	if (ret) {
		yk3_dev_err("qos get vf maxrate mbox failed, ret %d", ret);
		return 0;
	}

	msg = (struct yk3_qos_msg *)mbox_ack_msg.data;
	if (msg->func_rate.req.qset_id == ndev_priv->qsetid &&
	    (msg->func_rate.req.minrate || msg->func_rate.req.maxrate))
		return 1;

	return 0;
}

/* ndo_set_vf_rate callback to set function rate */
int yk3_qos_set_vf_rate(struct net_device *ndev, int vf, int minrate, int maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret = 0;

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_FUNC_SET_RATE;
	msg->func_rate.req.qset_id = pdev_priv->sriov_priv->vf_info[vf].qsetid;
	msg->func_rate.req.minrate = minrate;
	msg->func_rate.req.maxrate = maxrate;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret)
		yk3_dev_err("qos set vf maxrate mbox failed, ret %d", ret);

	return ret;
}

/* PF throttling:ysc link set dev IFNAME tx_rate x_mbps */
int yk3_qos_set_tx_rate(struct yk3_ndev_priv *ndev_priv, u32 maxrate)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret = 0;

	if (maxrate > ndev_priv->link_speed)
		return -EINVAL;

	if (yk3_pdev_is_mgr(pdev_priv))
		return -EOPNOTSUPP;

	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_FUNC_SET_RATE;
	msg->func_rate.req.qset_id = ndev_priv->qsetid;
	msg->func_rate.req.minrate = maxrate;
	msg->func_rate.req.maxrate = maxrate;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret) {
		yk3_net_err("qos set tx maxrate mbox failed, ret %d", ret);
		return ret;
	}

	ndev_priv->hqos_tx_rate = maxrate;

	return 0;
}

static void yk3_qos_set_func_rate_handle(struct yk3_pdev_priv *pdev_priv,
					 u32 qset_id, u32 minrate, u32 maxrate)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L1_NODE_HEAD + qset_id];

	node->min_rate = maxrate;
	node->max_rate = maxrate;
	node->in_use = 1;

	qos_update_entry_topo(node);
	qos_update_entry_rate(node);

	qos_entry_write(qos, &node->entry);
}

static void yk3_qos_get_func_rate_handle(struct yk3_pdev_priv *pdev_priv,
					 u32 qset_id, u32 *minrate, u32 *maxrate)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L1_NODE_HEAD + qset_id];

	*minrate = node->min_rate;
	*maxrate = node->max_rate;
}

/* ndo_set_tx_maxrate callback to set queue maxrate Mbps*/
int yk3_qos_set_queue_rate(struct net_device *ndev, int index, u32 maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_queuebase g_queue = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];

	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};

	int ret = 0;

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;
	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_QUEUE_SET_RATE;

	msg->queue_rate.req.g_qid = g_queue.start + index;
	msg->queue_rate.req.maxrate = maxrate;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret)
		yk3_dev_err("qos set queue maxrate mbox failed, ret %d", ret);

	return ret;
}

static void yk3_qos_set_queue_rate_handle(struct yk3_pdev_priv *pdev_priv,
					  u32 qid, u32 maxrate)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L0_NODE_HEAD + qid];

	node->min_rate = maxrate;
	node->max_rate = maxrate;
	node->in_use = 1;

	qos_update_entry_topo(node);
	qos_update_entry_rate(node);

	qos_entry_write(qos, &node->entry);
}

/* Init queue attr */
static int yk3_qos_queue_attr_init(struct yk3_qos *qos, u32 pf_id, u32 offset, u32 qid)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(qos->pdev);
	struct qos_node_entry entry = {};
	u8 tc_id = 0, pfc_id = 0;
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;

	tc_id = offset % 8;
	pfc_id = (pf_id << 3) + tc_id;

	set_field(&entry.data[0], TC_ID_MASK, TC_ID_SHIFT, tc_id);
	set_field(&entry.data[0], PORT_ID_MASK, PORT_ID_SHIFT, pf_id);
	set_field(&entry.data[0], PFC_ID_MASK, PFC_ID_SHIFT, pfc_id);
	entry.addr = qid;
	memcpy(entry.mask, mask, sizeof(u32) * 8);

	qos_entry_write(qos, &entry);

	yk3_dev_debug("qos set queue[%d] attr:pfc_id: %d, port_id: %d, tc_id: %d",
		      qid, pfc_id, pf_id, tc_id);

	return 0;
}

/* Init qos default config of queue after edma queue is created*/
static int yk3_qos_queue_init(struct yk3_qos *qos, struct yk3_qos_node *parent, u32 qid)
{
	struct yk3_qos_node *node;

	node = &qos->root[QOS_L0_NODE_HEAD + qid];
	node->parent = parent;
	node->num_child = 0;
	node->min_rate = 0;
	node->max_rate = 0;
	node->in_use = 0;

	qos_update_entry_topo(node);
	qos_update_entry_rate(node);

	qos_entry_write(qos, &node->entry);

	return 0;
}

/* Init qos default config of function after netdev is created*/
int yk3_qos_ndev_init(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;

	struct yk3_ets_cfg *etscfg, *etsrec;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};

	int ret = 0;

	if (ndev_priv->pf_id > 3) {  //TEMP: BNIC has 4 PFs.
		yk3_dev_err("qos set queue attr failed, pf_id: %d", ndev_priv->pf_id);
		return -EINVAL;
	}

	if (yk3_ndev_is_pf(ndev_priv)) {
		etscfg = kzalloc(sizeof(*etscfg), GFP_KERNEL);
		if (!etscfg)
			return -ENOMEM;
		ndev_priv->etscfg = etscfg;
		ndev_priv->etscfg->willing = 1;
		ndev_priv->etscfg->cbs = 0;
		ndev_priv->etscfg->maxtcs = QOS_TRAFFIC_CLASS;

		etsrec = kzalloc(sizeof(*etsrec), GFP_KERNEL);
		if (!etsrec) {
			kfree(ndev_priv->etscfg);
			ndev_priv->etscfg = NULL;
			return -ENOMEM;
		}
		ndev_priv->etsrec = etsrec;
		ndev_priv->etsrec->maxtcs = QOS_TRAFFIC_CLASS;
		ndev_priv->etsrec->tcbwtable[0] = ETS_BW_DEFAULT;
	}

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;
	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_NDEV_TOPO_INIT;
	msg->ndev_init.req.pf_id = ndev_priv->pf_id;
	msg->ndev_init.req.vf_id = ndev_priv->vf_id;
	msg->ndev_init.req.qset_id = ndev_priv->qsetid;
	msg->ndev_init.req.g_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	msg->ndev_init.req.qnum = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].num;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	if (ret) {
		kfree(ndev_priv->etscfg);
		ndev_priv->etscfg = NULL;

		kfree(ndev_priv->etsrec);
		ndev_priv->etsrec = NULL;

		yk3_dev_err("qos ndev init mbox failed, ret %d", ret);
	}

	return ret;
}

void yk3_qos_ndev_exit(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);

	kfree(ndev_priv->etscfg);
	kfree(ndev_priv->etsrec);

	ndev_priv->etscfg = NULL;
	ndev_priv->etsrec = NULL;
}

static void yk3_qos_ndev_init_handle(struct yk3_pdev_priv *pdev_priv, u32 pf_id,
				     u32 vf_id, u32 qset_id, struct yk3_queuebase g_qbase)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;
	u32 i = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	node = &qos->root[QOS_L1_NODE_HEAD + qset_id];

	node->parent = &qos->root[QOS_L2_NODE_HEAD];
	node->num_child = g_qbase.num;
	node->min_rate = 0;
	node->max_rate = 0;
	node->in_use = 0;

	qos_update_entry_topo(node);
	qos_update_entry_rate(node);

	qos_entry_write(qos, &node->entry);

	for (i = 0; i < g_qbase.num; i++) {
		yk3_qos_queue_init(qos, node, g_qbase.start + i);
		yk3_qos_queue_attr_init(qos, pf_id, 0, g_qbase.start + i);
	}
}

/**
 * yk3_qos_set_qp_tc_handle - Handle QP to TC mapping on MGR device
 * @pdev_priv: PCI device private data
 * @qp_id: Queue Pair ID
 * @tc_id: Traffic Class ID
 * @pf_id: PF ID
 *
 * Configures the queue attributes for RDMA QP:
 * - TC_ID: Traffic Class for scheduling priority
 * - PORT_ID: PF identifier
 * - PFC_ID: Priority Flow Control identifier for lossless traffic
 */
static void yk3_qos_set_qp_tc_handle(struct yk3_pdev_priv *pdev_priv,
				     u32 qp_id, u32 tc_id, u32 pf_id)
{
	struct yk3_qos *qos;
	struct qos_node_entry entry = {};
	u32 pfc_id;
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	if (!qos)
		return;

	/* Calculate PFC ID: (PF_ID << 3) | TC_ID for priority flow control */
	pfc_id = (pf_id << 3) | tc_id;

	/* Configure queue attributes for this QP */
	set_field(&entry.data[0], TC_ID_MASK, TC_ID_SHIFT, tc_id);
	set_field(&entry.data[0], PORT_ID_MASK, PORT_ID_SHIFT, pf_id);
	set_field(&entry.data[0], PFC_ID_MASK, PFC_ID_SHIFT, pfc_id);
	entry.addr = qp_id;
	memcpy(entry.mask, mask, sizeof(u32) * 8);

	qos_entry_write(qos, &entry);

	yk3_dev_info("QP %u: TC=%u, PORT=%u, PFC=%u configured",
		     qp_id, tc_id, pf_id, pfc_id);
}

/**
 * yk3_qos_set_qp_rate_handle - Handle QP rate limiting on MGR device
 * @pdev_priv: PCI device private data
 * @qp_id: Queue Pair ID
 * @maxrate: Maximum rate in Mbps
 * @pf_id: PF ID
 *
 * Configures rate limiting for RDMA QP at L0 (queue) level
 */
static void yk3_qos_set_qp_rate_handle(struct yk3_pdev_priv *pdev_priv,
				       u32 qp_id, u32 maxrate, u32 pf_id)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	if (!qos)
		return;

	/* Get queue node at L0 level */
	if (qp_id >= QOS_L0_QUEUE_NUM) {
		yk3_dev_err("Invalid QP ID %u (max: %u)", qp_id, QOS_L0_QUEUE_NUM - 1);
		return;
	}

	node = &qos->root[QOS_L0_NODE_HEAD + qp_id];

	node->min_rate = maxrate;
	node->max_rate = maxrate;

	qos_update_entry_rate(node);
	qos_entry_write(qos, &node->entry);

	yk3_dev_info("QP %u rate limited to %u Mbps", qp_id, maxrate);
}

/**
 * yk3_qdisc_cleanup_mqprio_map - Cleanup MQPRIO configuration on MGR device
 * @pdev_priv: PCI device private data
 * @pf_id: PF ID
 * @qstart: Global queue base
 * @qnum: Number of queues
 *
 * Resets QoS hardware configuration when MQPRIO qdisc is deleted:
 * - Disables all TC nodes for this PF
 * - Resets all queue attributes to default (TC 0)
 */
static void yk3_qdisc_cleanup_mqprio_map(struct yk3_pdev_priv *pdev_priv,
					 u32 pf_id, u32 qstart, u32 qnum)
{
	struct yk3_qos *qos;
	struct yk3_qos_node *node;
	struct qos_node_entry entry = {};
	u32 mask[8] = QOS_ENTRY_MASK_DFLT;
	u32 i;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	qos = pdev_priv->qos;
	if (!qos)
		return;

	yk3_dev_info("Cleaning MQPRIO for PF %u: qbase=%u, qnum=%u",
		     pf_id, qstart, qnum);

	/* 1. Disable all TC nodes for this PF */
	node = &qos->root[QOS_L3_NODE_HEAD + pf_id * QOS_TRAFFIC_CLASS];
	for (i = 0; i < QOS_TRAFFIC_CLASS; i++) {
		if (node[i].in_use) {
			node[i].in_use = 0;
			node[i].min_rate = 0;
			node[i].max_rate = 0;

			qos_update_entry_topo(&node[i]);
			qos_update_entry_rate(&node[i]);
			qos_entry_write(qos, &node[i].entry);

			yk3_dev_debug("TC node %u disabled", i);
		}
	}

	/* 2. Reset all queue attributes to default (TC 0) */
	for (i = 0; i < qnum; i++) {
		memset(&entry, 0, sizeof(entry));
		set_field(&entry.data[0], TC_ID_MASK, TC_ID_SHIFT, 0);  //0 or default tc?
		set_field(&entry.data[0], PORT_ID_MASK, PORT_ID_SHIFT, pf_id);
		set_field(&entry.data[0], PFC_ID_MASK, PFC_ID_SHIFT, pf_id << 3);
		entry.addr = qstart + i;
		memcpy(entry.mask, mask, sizeof(u32) * 8);

		qos_entry_write(qos, &entry);
	}

	yk3_dev_info("MQPRIO cleanup completed for PF %u", pf_id);
}

static void yk3_qos_dump_ets(struct yk3_pdev_priv *pdev_priv,
			     struct yk3_ets_cfg *ets)
{
	u8 i = 0;

	yk3_dev_debug("willing:%d, cbs:%d", ets->willing, ets->cbs);

	for (i = 0; i < YK3_MAX_TRAFFIC_CLASS; i++) {
		yk3_dev_debug("prio[%d]: tc[%d] bw[%d] tsa[%d] max_rate[%lld]Mbps", i,
			      ets->prio_table[i], ets->tcbwtable[i],
			      ets->tsatable[i], ets->max_rate[i]);
	}
}

static int yk3_qos_set_ets(struct yk3_pdev_priv *pdev_priv, u32 pf_id,
			   struct yk3_ets_cfg *ets)
{
	struct yk3_qos *qos = pdev_priv->qos;
	struct yk3_qos_node *tc_node, *port_node, tmp_node;
	struct qos_sched_entry sched = {};
	u8 num_tc, sp_en = 0;
	u8 weights[QOS_TRAFFIC_CLASS] = {};
	int i;

	num_tc = yk3_qos_get_num_tc(ets);
	if (num_tc < 1 || num_tc > QOS_TRAFFIC_CLASS)
		return -EINVAL;

	yk3_qos_dump_ets(pdev_priv, ets);

	for (i = 0; i < num_tc; i++) {
		if (ets->tsatable[i] == IEEE_8021QAZ_TSA_STRICT) {
			sp_en |= BIT(i);
			weights[i] = 0;
		} else {
			/* ETS/WRR: use configured bandwidth percentage as weight */
			weights[i] = ets->tcbwtable[i] ? ets->tcbwtable[i] : 1;
		}
	}
	for (i = num_tc; i < QOS_TRAFFIC_CLASS; i++)
		weights[i] = 0;

	memset(&sched, 0, sizeof(sched));
	sched.weight0 = weights[0];
	sched.weight1 = weights[1];
	sched.weight2 = weights[2];
	sched.weight3 = weights[3];
	sched.weight4 = weights[4];
	sched.weight5 = weights[5];
	sched.weight6 = weights[6];
	sched.weight7 = weights[7];
	sched.sp_en = sp_en;
	sched.entry_en = 1;
	sched.wrr_priority = 0;

	port_node = &qos->root[QOS_L4_NODE_HEAD + pf_id];
	memcpy(&tmp_node, port_node, sizeof(tmp_node));
	qos_update_entry_sched(&tmp_node, &sched);
	qos_entry_write(qos, &tmp_node.entry);

	yk3_wr32(qos->hw_base, QOS_BUFF_ID_MODE_OFFSET(QOS_PORT_LAYER_ID), QOS_BUFFID_MODE_TC);
	yk3_wr32(qos->hw_base, QOS_DISTRIBUT_EN_OFFSET(QOS_PORT_LAYER_ID), 0x1);
	yk3_wr32(qos->hw_base, QOS_EXTSWITCH_EN_OFFSET(QOS_TC_LAYER_ID), 0x1);

	for (i = 0; i < num_tc; i++) {
		tc_node = &qos->root[QOS_L3_NODE_HEAD + pf_id * QOS_TRAFFIC_CLASS + i];
		tc_node->sched_mode = QOS_BUCKET_MODE_DCB;
		tc_node->in_use = 1;
		tc_node->prio_slot = i;
		qos_update_entry_topo(tc_node);
		qos_entry_write(qos, &tc_node->entry);

		if (ets->tsatable[i] == IEEE_8021QAZ_TSA_STRICT) {
			/* strict has no yellow traffic */
			if (!ets->max_rate[i]) {
				tc_node->min_rate = qos->link_speed / num_tc;
				tc_node->max_rate = qos->link_speed / num_tc;
			} else {
				tc_node->min_rate = ets->max_rate[i];
				tc_node->max_rate = ets->max_rate[i];
			}
		} else {
			tc_node->min_rate = 0;  //ets has no green traffic
			tc_node->max_rate = ets->max_rate[i];
			if (!ets->max_rate[i])
				tc_node->max_rate = qos->link_speed;
		}

		qos_update_entry_rate(tc_node);
		qos_entry_write(qos, &tc_node->entry);
	}

	for (i = num_tc; i < QOS_TRAFFIC_CLASS; i++) {
		tc_node = &qos->root[QOS_L3_NODE_HEAD + pf_id * QOS_TRAFFIC_CLASS + i];
		if (tc_node->in_use) {
			tc_node->in_use = 0;
			tc_node->min_rate = 0;
			tc_node->max_rate = 0;
			tc_node->prio_slot = 0;
			qos_update_entry_topo(tc_node);
			qos_update_entry_rate(tc_node);
			qos_entry_write(qos, &tc_node->entry);
		}
	}

	return 0;
}

/**
 * yk3_qos_get_num_tc - Get the number of TCs from ets config
 * @etscfg: config to retrieve number of TCs from
 */
u8 yk3_qos_get_num_tc(struct yk3_ets_cfg *etscfg)
{
	bool tc_unused = false;
	u8 num_tc = 0;
	u8 ret = 0;
	int i;

	/* Scan the ETS Config Priority Table to find traffic classes
	 * enabled and create a bitmask of enabled TCs
	 */
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
		num_tc |= BIT(etscfg->prio_table[i]);

	/* Scan bitmask for contiguous TCs starting with TC0 */
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		if (num_tc & BIT(i)) {
			if (!tc_unused) {
				ret++;
			} else {
				pr_err("Non-contiguous TCs - Disabling DCB\n");
				return 1;
			}
		} else {
			tc_unused = true;
		}
	}

	/* There is always at least 1 TC */
	if (!ret)
		ret = 1;

	return ret;
}

static int yk3_qos_ets_chk(struct yk3_ets_cfg *etscfg)
{
	u8 num_tc, total_bw = 0;
	int i;

	/* returns number of contigous TCs and 1 TC for non-contigous TCs,
	 * since at least 1 TC has to be configured
	 */
	num_tc = yk3_qos_get_num_tc(etscfg);

	/* no bandwidth checks required if there's only one TC, so assign
	 * all bandwidth to TC0 and return
	 */
	if (num_tc == 1) {
		etscfg->tcbwtable[0] = ETS_BW_DEFAULT;
		return 0;
	}

	for (i = 0; i < num_tc; i++)
		total_bw += etscfg->tcbwtable[i];

	if (total_bw && total_bw != ETS_BW_DEFAULT)
		return -EINVAL;

	return 0;
}

int yk3_ets_set_tc_default(struct net_device *ndev, struct yk3_ets_cfg *cfg)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0}, mbox_ack_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret = 0, i = 0, offset = 0;
	u16 base, remainder;
	u8 tc_num = yk3_qos_get_num_tc(cfg);

	if (tc_num > ndev->real_num_tx_queues) {
		yk3_dev_warn("tc num[%d] bigger than queue num[%d]",
			     tc_num, ndev->real_num_tx_queues);
		tc_num = 1;
	}

	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_DQISC_SET_TC_MAP;
	msg->tc.pf_id = pdev_priv->pf_id;
	msg->tc.g_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	msg->tc.tc_qopt.num_tc = tc_num;
	memcpy(msg->tc.tc_qopt.prio_tc_map, cfg->prio_table,
	       sizeof(msg->tc.tc_qopt.prio_tc_map));

	base = ndev->real_num_tx_queues / tc_num;
	remainder = ndev->real_num_tx_queues % tc_num;

	for (i = 0; i < tc_num; i++) {
		msg->tc.tc_qopt.count[i] = base + (i < remainder ? 1 : 0);
		if (!msg->tc.tc_qopt.count[i])
			msg->tc.tc_qopt.count[i] = 1;
		msg->tc.tc_qopt.offset[i] = offset;
		offset += msg->tc.tc_qopt.count[i];
	}

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
	if (ret) {
		yk3_dev_err("qos set tc map mbox failed, ret %d", ret);
		return ret;
	}

	yk3_qos_update_tc_map(ndev, &msg->tc.tc_qopt);
	return 0;
}

static int _yk3_dcbnl_setets(struct net_device *ndev)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0}, mbox_ack_msg = {0};
	struct yk3_qos_msg *msg;
	struct yk3_mbox_option opt = {0};
	struct yk3_ets_cfg tmp = {};
	int ret = 0;

	if (!ndev_priv->etscfg || !ndev_priv->etsrec)
		return -EINVAL;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	memcpy(&tmp, ndev_priv->etscfg, sizeof(tmp));
	ret = yk3_ets_set_tc_default(ndev, &tmp);
	if (ret) {
		yk3_dev_err("ets set tc default map failed, ret %d", ret);
		return ret;
	}

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QOS;

	msg = (struct yk3_qos_msg *)mbox_msg.data;
	msg->msgid = MSG_SET_ETS;
	msg->ets_cfg.pf_id = pdev_priv->pf_id;
	msg->ets_cfg.ets = tmp;

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
	if (ret) {
		yk3_dev_err("qos set ets tc maxrate mbox failed, ret %d", ret);
		return ret;
	}

	set_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags);
	return ret;
}

int yk3_dcbnl_setets(struct net_device *ndev, struct ieee_ets *ets)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_ets_cfg tmp = {};
	u32 i, bwrec = 0;

	if (!ndev_priv->etscfg || !ndev_priv->etsrec)
		return -EINVAL;

	if (!yk3_pdev_is_pf(pdev_priv))
		return -EOPNOTSUPP;

	memcpy(&tmp, ndev_priv->etscfg, sizeof(tmp));
	tmp.willing = ets->willing;
	for (i = 0; i < QOS_TRAFFIC_CLASS; i++) {
		tmp.tsatable[i] = ets->tc_tsa[i];
		tmp.tcbwtable[i] = ets->tc_tx_bw[i];
		tmp.prio_table[i] = ets->prio_tc[i];

		ndev_priv->etsrec->tcbwtable[i] = ets->tc_reco_bw[i];
		ndev_priv->etsrec->tsatable[i] = ets->tc_reco_tsa[i];
		ndev_priv->etsrec->prio_table[i] = ets->reco_prio_tc[i];
		bwrec += ets->tc_reco_bw[i];
	}

	if (!bwrec)
		ndev_priv->etsrec->tcbwtable[0] = 100;

	if (yk3_qos_ets_chk(&tmp))
		return -EINVAL;
	memcpy(ndev_priv->etscfg, &tmp, sizeof(tmp));

	if ((test_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags))) {
		set_bit(YK3_ETS_PENDING, ndev_priv->tc_cfg.flags);
		return 0;
	}

	return _yk3_dcbnl_setets(ndev);
}

int yk3_dcbnl_getets(struct net_device *netdev, struct ieee_ets *ets)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(netdev);

	if (!ndev_priv->etscfg || !ndev_priv->etsrec)
		return -EINVAL;

	ets->willing = ndev_priv->etscfg->willing;
	ets->ets_cap = ndev_priv->etscfg->maxtcs;
	ets->cbs = ndev_priv->etscfg->cbs;
	memcpy(ets->tc_tx_bw, ndev_priv->etscfg->tcbwtable, sizeof(ets->tc_tx_bw));
	memcpy(ets->tc_rx_bw, ndev_priv->etscfg->tcbwtable, sizeof(ets->tc_rx_bw));
	memcpy(ets->tc_tsa, ndev_priv->etscfg->tsatable, sizeof(ets->tc_tsa));
	memcpy(ets->prio_tc, ndev_priv->etscfg->prio_table, sizeof(ets->prio_tc));
	memcpy(ets->tc_reco_bw, ndev_priv->etsrec->tcbwtable,
	       sizeof(ets->tc_reco_bw));
	memcpy(ets->tc_reco_tsa, ndev_priv->etsrec->tsatable,
	       sizeof(ets->tc_reco_tsa));
	memcpy(ets->reco_prio_tc, ndev_priv->etsrec->prio_table,
	       sizeof(ets->reco_prio_tc));
	return 0;
}

int yk3_dcbnl_getmaxrate(struct net_device *netdev, struct ieee_maxrate *maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(netdev);
	int i = 0;

	if (!ndev_priv->etscfg)
		return -EINVAL;

	for (i = 0; i < QOS_TRAFFIC_CLASS; i++)
		maxrate->tc_maxrate[i] = ndev_priv->etscfg->max_rate[i] * 125000;  //Mbps->Bps

	return 0;
}

int yk3_dcbnl_setmaxrate(struct net_device *ndev, struct ieee_maxrate *maxrate)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	int i = 0;

	if (!ndev_priv->etscfg)
		return -EINVAL;

	for (i = 0; i < QOS_TRAFFIC_CLASS; i++)
		ndev_priv->etscfg->max_rate[i] = maxrate->tc_maxrate[i] / 125000;  //Bps->Mbps

	if (test_bit(YK3_QDISC_VALID, ndev_priv->tc_cfg.flags))
		return 0;

	if (test_bit(YK3_ETS_VALID, ndev_priv->tc_cfg.flags))
		_yk3_dcbnl_setets(ndev);

	return 0;
}

static void yk3_qos_mbox_cb(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = param;
	struct yk3_qos_msg *req;
	struct yk3_qos_msg *rsp;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt;
	u32 qid, pf_id, vf_id, qset_id, minrate, maxrate, qstart;
	struct qos_rt_dst rt_dst;
	struct yk3_queuebase g_qbase;
	struct tc_map qdisc_mqprio;
	u64 tc_rate[QOS_TRAFFIC_CLASS];

	if (msg->opcode != YK3_MBOX_OPCODE_QOS) {
		yk3_dev_err("qos mbox cb, opcode mismatch");
		return;
	}

	req = (struct yk3_qos_msg *)msg->data;
	rsp = (struct yk3_qos_msg *)ack_msg.data;
	rsp->msgid = req->msgid;
	yk3_dev_debug("qos mbox cb, msgid: %d", req->msgid);

	switch (req->msgid) {
	case MSG_NDEV_TOPO_INIT:
		pf_id = req->ndev_init.req.pf_id;
		vf_id = req->ndev_init.req.vf_id;
		qset_id = req->ndev_init.req.qset_id;
		g_qbase.start = req->ndev_init.req.g_qbase;
		g_qbase.num = req->ndev_init.req.qnum;
		yk3_qos_ndev_init_handle(pdev_priv, pf_id, vf_id, qset_id, g_qbase);

		yk3_dev_info("ndev init topo qset:%d\n", qset_id);
		break;

	case MSG_QUEUE_SET_RATE:
		qid = req->queue_rate.req.g_qid;
		maxrate = req->queue_rate.req.maxrate;
		yk3_qos_set_queue_rate_handle(pdev_priv, qid, maxrate);

		yk3_dev_info("set queue:%d maxrate:%d\n", qid, maxrate);
		break;

	case MSG_FUNC_SET_RATE:
		qset_id = req->func_rate.req.qset_id;
		minrate = req->func_rate.req.minrate;
		maxrate = req->func_rate.req.maxrate;
		yk3_qos_set_func_rate_handle(pdev_priv, qset_id, minrate, maxrate);

		yk3_dev_info("set vf qset:%d minrate:%d maxrate:%d\n",
			     qset_id, minrate, maxrate);
		break;

	case MSG_FUNC_GET_RATE:
		qset_id = req->func_rate.req.qset_id;
		minrate = 0;
		maxrate = 0;
		yk3_qos_get_func_rate_handle(pdev_priv, qset_id, &minrate, &maxrate);
		rsp->func_rate.req.qset_id = qset_id;
		rsp->func_rate.req.minrate = minrate;
		rsp->func_rate.req.maxrate = maxrate;
		yk3_dev_info("get vf qset:%d minrate:%d maxrate:%d\n",
			     qset_id, minrate, maxrate);

		ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
		ack_msg.seqno = msg->seqno;
		ack_msg.dst_id = msg->src_id;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
		break;

	case MSG_DQISC_SET_TC_MAP:
		qdisc_mqprio = req->tc.tc_qopt;
		pf_id = req->tc.pf_id;
		qstart = req->tc.g_qbase;
		yk3_qdisc_setup_mqprio_map(pdev_priv, pf_id, qstart, &qdisc_mqprio);

		ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
		ack_msg.seqno = msg->seqno;
		ack_msg.dst_id = msg->src_id;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
		break;

	case MSG_DQISC_SET_TC_MIN_RATE:
		pf_id = req->tc_shaper_min.pf_id;
		memcpy(tc_rate, req->tc_shaper_min.min_rate, sizeof(u64) * QOS_TRAFFIC_CLASS);
		yk3_qdisc_setup_mqprio_shaper(pdev_priv, pf_id, tc_rate, true);
		break;

	case MSG_DQISC_SET_TC_MAX_RATE:
		pf_id = req->tc_shaper_max.pf_id;
		memcpy(tc_rate, req->tc_shaper_max.max_rate, sizeof(u64) * QOS_TRAFFIC_CLASS);
		yk3_qdisc_setup_mqprio_shaper(pdev_priv, pf_id, tc_rate, false);

		ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
		ack_msg.seqno = msg->seqno;
		ack_msg.dst_id = msg->src_id;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
		break;

	case MSG_DQISC_CLEANUP_TC:
		pf_id = req->tc_cleanup.pf_id;
		qstart = req->tc_cleanup.g_qbase;
		yk3_qdisc_cleanup_mqprio_map(pdev_priv, pf_id, qstart,
					     req->tc_cleanup.qnum);
		yk3_dev_debug("MQPRIO cleanup for PF %u", pf_id);
		ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
		ack_msg.seqno = msg->seqno;
		ack_msg.dst_id = msg->src_id;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
		break;

	case MSG_QP_SET_TC:
		yk3_qos_set_qp_tc_handle(pdev_priv, req->qp_tc.qp_id,
					 req->qp_tc.tc_id, req->qp_tc.pf_id);
		yk3_dev_info("QP %u mapped to TC %u (PF %u)",
			     req->qp_tc.qp_id, req->qp_tc.tc_id, req->qp_tc.pf_id);
		break;

	case MSG_QP_SET_RATE:
		yk3_qos_set_qp_rate_handle(pdev_priv, req->qp_rate.qp_id,
					   req->qp_rate.maxrate, req->qp_rate.pf_id);
		yk3_dev_info("QP %u rate set to %u Mbps",
			     req->qp_rate.qp_id, req->qp_rate.maxrate);
		break;

	case MSG_QP_SET_RT_DST:
		rt_dst = req->rt_dst;
		yk3_qos_set_rt_dst_handle(pdev_priv, rt_dst);

		yk3_dev_info("set qrange_id_%d:%d to rt_dst_id_%d:%d\n",
			     rt_dst.qrange_id, rt_dst.qrange, rt_dst.rt_dst_id, rt_dst.rt_dst);
		break;

	case MSG_SET_LINK_SPEED:
		pf_id = req->link_speed.pf_id;
		minrate = req->link_speed.speed_mbps;
		maxrate = req->link_speed.speed_mbps;
		yk3_qos_set_link_speed_handle(pdev_priv, pf_id,
					      minrate, maxrate);
		yk3_dev_info("PF %u link speed set to %u Mbps",
			     pf_id, maxrate);
		break;

	case MSG_SET_ETS:
		pf_id = req->ets_cfg.pf_id;
		yk3_qos_set_ets(pdev_priv, pf_id, &req->ets_cfg.ets);

		ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
		ack_msg.seqno = msg->seqno;
		ack_msg.dst_id = msg->src_id;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
		yk3_dev_debug("PF %u set ets", pf_id);
		break;

	default:
		yk3_dev_err("qos msgid %d not supported", req->msgid);
		break;
	}
}

#define DEFINE_SHOW_STORE_ATTRIBUTE(__name)				\
static int __name ## _open(struct inode *inode, struct file *file)	\
{									\
	return single_open(file, __name ## _show, inode->i_private);	\
}									\
									\
static const struct file_operations __name ## _fops = {			\
	.owner		= THIS_MODULE,					\
	.open		= __name ## _open,				\
	.read		= seq_read,					\
	.write		= __name ## _write,				\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
}

static int yk3_qos_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_qos *qos = pdev_priv->qos;
	int i;

	seq_printf(seq, "\t    %-16s : %-16d\n", "sysclk", qos->qos_clk);
	seq_printf(seq, "\t    %-16s : %-16d\n", "leaf", qos->leaf_num);
	seq_printf(seq, "\t    %-16s : %-16d\n", "queues_per_leaf", qos->qnum_per_leaf);
	seq_printf(seq, "\t    %-16s : %-16d\n", "maxrate_ratio", qos->maxrate_ratio);

	for (i = QOS_L4_NODE_HEAD; i < QOS_TOTAL_NODE_NUM; i++) {
		qos_entry_read(qos, &qos->root[i]);
		qos_dump_entry(qos, &qos->root[i].entry);
	}

	return 0;
}

static ssize_t yk3_qos_debugfs_write(struct file *filp,
				     const char __user *user_buf,
				     size_t count,
				     loff_t *ppos)
{
	struct seq_file *seq = filp->private_data;
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_qos *qos = pdev_priv->qos;
	struct yk3_qos_node *port_node;
	char buf[32] = {};
	u32 val = 0;
	int ret = 0, i = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return -EINVAL;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buf, count))
		return -EFAULT;

	buf[count] = '\0';

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;

	if (val > 1000)
		return -EINVAL;

	qos->maxrate_ratio = val ? val : 1;

	for (i = 0; i < QOS_L4_PORT_NUM; i++) {
		port_node = &qos->root[QOS_L4_NODE_HEAD + i];
		port_node->max_rate = qos->link_speed * qos->maxrate_ratio / 1000;
		port_node->min_rate = qos->link_speed * qos->maxrate_ratio / 1000;
		qos_update_entry_rate(port_node);
		qos_entry_write(qos, &port_node->entry);
	}

	return count;
}

DEFINE_SHOW_STORE_ATTRIBUTE(yk3_qos_debugfs);

static void yk3_qos_debug_init(struct yk3_qos *qos)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(qos->pdev);
	struct dentry *entry;

	if (!pdev_priv->dbgfs_dir)
		return;

	entry = debugfs_create_file("qos_info", 0644, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_qos_debugfs_fops);
	if (!entry) {
		yk3_err("Failed to create debugfs irq info file for pdev %s", pdev_priv->name);
		return;
	}

	qos->dbgfs_info_file = entry;
}

/* Init sw struct from hw */
static void yk3_qos_init_sw(struct yk3_qos *qos)
{
	u32 clk;
	u32 leaf_cap;
	struct yk3_pdev_priv *priv = pci_get_drvdata(qos->pdev);

	clk = yk3_rd32(qos->hw_base, QOS_SYS_CLK_PERIOD);  //reset value:0x1DCD6500 = 500,000,000
	qos->qos_clk = clk;  //Firmware should set QOS_SYS_CLK_PERIOD reg

	if (priv->card->emp_info->vpd.chip_type) {
		/* TEMP: umac use 62.5Mhz in prototype xmac is 50Mhz */
		qos->qos_clk = 50000000;
		if (priv->card->hw_bits & YK3_HW_BIT_UMAC)
			qos->qos_clk = 62500000;
	} else {
		qos->qos_clk = 350000000;  //TEMP default
	}

	leaf_cap = yk3_rd32(qos->hw_base, QOS_LEAF_CAP);
	qos->leaf_num = (leaf_cap >> 16) & 0xff;
	qos->qnum_per_leaf = leaf_cap & 0xffff;

	qos->maxrate_ratio = 998;  //25Gbps * 998/1000 = 24.95Gbps
	qos->link_speed = SPEED_100G;
}

static int yk3_qos_init_topo_hw(struct yk3_qos_node *node)
{
	if (!yk3_pdev_is_mgr(pci_get_drvdata(node->qos->pdev)))		//TEMP
		return 0;

	qos_update_entry_topo(node);

	qos_update_entry_rate(node);

	qos_entry_write(node->qos, &node->entry);

	return 0;
}

/* Init default topology */
static int yk3_qos_init_topo(struct yk3_qos *qos)
{
	struct yk3_qos_node *node;
	u32 num = QOS_TOTAL_NODE_NUM;
	u32 i = 0, j = 0;

	node = kcalloc(num, sizeof(*node), GFP_KERNEL);
	if (!node)
		return -ENOMEM;

	qos->root = node;
	j++;

	for (i = 0; i < QOS_L4_PORT_NUM; i++, j++) {
		node[j].parent = qos->root;
		node[j].layer_id = QOS_PORT_LAYER_ID;
		node[j].node_id = i;
		node[j].min_rate = qos->link_speed * qos->maxrate_ratio / 1000;
		node[j].max_rate = qos->link_speed * qos->maxrate_ratio / 1000;
		node[j].sched_mode = QOS_BUCKET_MODE_SCHEDLER;  //scheduler
		node[j].qos = qos;
		node[j].in_use = 1;
		yk3_qos_init_topo_hw(&node[j]);

		yk3_wr32(qos->hw_base, QOS_DISTRIBUT_EN_OFFSET(QOS_PORT_LAYER_ID), 0);
		yk3_wr32(qos->hw_base, QOS_EXTSWITCH_EN_OFFSET(QOS_TC_LAYER_ID), 0);
	}

	for (i = 0; i < QOS_L3_TC_NUM; i++, j++) {
		node[j].parent = &node[QOS_L4_NODE_HEAD + i / 8];
		node[j].layer_id = QOS_TC_LAYER_ID;
		node[j].node_id = i;
		node[j].min_rate = 0;
		node[j].max_rate = 0;
		node[j].sched_mode = QOS_BUCKET_MODE_DCB;  //dfb
		node[j].qos = qos;
		node[j].in_use = 0;
		node[j].prio_slot = 0;
		yk3_qos_init_topo_hw(&node[j]);
	}

	for (i = 0; i < QOS_L2_GROUP_NUM; i++, j++) {
		node[j].parent = &node[QOS_L3_NODE_HEAD];  //TC0
		node[j].layer_id = QOS_GROUP_LAYER_ID;
		node[j].node_id = i;
		node[j].min_rate = 0;
		node[j].max_rate = 0;
		node[j].sched_mode = QOS_BUCKET_MODE_SHAPER;  //shaper
		node[j].qos = qos;
		node[j].in_use = 0;
		yk3_qos_init_topo_hw(&node[j]);
	}

	for (i = 0; i < QOS_L1_QSET_NUM; i++, j++) {
		node[j].parent = &node[QOS_L2_NODE_HEAD];
		node[j].layer_id = QOS_QSET_LAYER_ID;
		node[j].node_id = i;
		node[j].min_rate = 0;
		node[j].max_rate = 0;
		node[j].sched_mode = QOS_BUCKET_MODE_DCB;  //dfb
		node[j].qos = qos;
		node[j].in_use = 0;
		yk3_qos_init_topo_hw(&node[j]);
	}

	for (i = 0; i < QOS_L0_QUEUE_NUM; i++, j++) {
		node[j].parent = &node[QOS_L1_NODE_HEAD];
		node[j].layer_id = QOS_QUEUE_LAYER_ID;
		node[j].node_id = i;
		node[j].min_rate = 0;
		node[j].max_rate = 0;
		node[j].sched_mode = QOS_BUCKET_MODE_SHAPER;  //shaper
		node[j].qos = qos;
		node[j].in_use = 0;
		yk3_qos_init_topo_hw(&node[j]);

		yk3_qos_queue_attr_init(qos, 0, 0, i);
	}

	return 0;
}

/* Init hw once*/
static void yk3_qos_init_hw(struct yk3_qos *qos)
{
	u32 i = 0, j = 0;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(qos->pdev);
	struct yk3_card *card = pdev_priv->card;

	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(0), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(1), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(2), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(3), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(4), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(5), 0);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(6), 0x00a50000);
	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_0(7), 0x00000413);

	yk3_wr32(qos->hw_base, QOS_ST_CONTEXT_EN, 1);  //read always 0?

	yk3_wr32(qos->hw_base, QOS_PRE_CREDIT0, 0x0002ee00);
	yk3_wr32(qos->hw_base, QOS_PRE_CREDIT1, 0x0002ee00);
	yk3_wr32(qos->hw_base, QOS_PRE_CREDIT2, 0x0002ee00);
	yk3_wr32(qos->hw_base, QOS_PRE_CREDIT3, 0x0002ee00);

	yk3_wr32(qos->hw_base, QOS_CLASSID_MODE_OFFSET(QOS_PORT_LAYER_ID),
		 QOS_CLASSID_MODE_PORT);	//level 4 set class_id to port
	yk3_wr32(qos->hw_base, QOS_CLASSID_MODE_OFFSET(QOS_TC_LAYER_ID),
		 QOS_CLASSID_MODE_PORT_TC);	//level 3 set class_id to port_tc
	yk3_wr32(qos->hw_base, QOS_CLASSID_MODE_OFFSET(QOS_QUEUE_LAYER_ID),
		 QOS_CLASSID_MODE_QID);	//level 0 set class_id to queue

	/* enalbe queue */
	for (i = 0; i < qos->leaf_num; i++) {
		/* TEMP:bind all leaf to edma */
		yk3_wr32(qos->hw_base, QOS_LEAF_REG_SEL, i);
		yk3_wr32(qos->hw_base, QOS_LEAF_N_FILTER_PORT,
			 FIELD_PREP(QOS_LEAF_EN_FIELD, 1) |
			 FIELD_PREP(QOS_LEAF_PORT_FIELD, QOS_LEAF_PORT_HOST));

		if (i == 1 && card->mode == YK3_MODE_RCARD) {  //np use leaf 1
			yk3_wr32(qos->hw_base, QOS_LEAF_N_FILTER_PORT,
				 FIELD_PREP(QOS_LEAF_EN_FIELD, 1) |
				 FIELD_PREP(QOS_LEAF_PORT_FIELD, QOS_LEAF_PORT_NP));

			yk3_qos_init_rt_dst(qos);
		}

		/* Enable queues of leaf */
		yk3_wr32(qos->hw_base, QOS_LEAF_REG_WSTRB, 1);
		for (j = 0; j < qos->qnum_per_leaf; j++)
			yk3_wr32(qos->hw_base, QOS_LEAF_N_QUEUE_CFG(j), 1);
		yk3_wr32(qos->hw_base, QOS_LEAF_REG_WSTRB, 0);
	}

	//yk3_wr32(qos->hw_base, QOS_PULSE_SWITCH, 1);	//BUG:will cause rate double
	yk3_wr32(qos->hw_base, QOS_GLOBAL_ENABLE, 1);
}

/* Init global qos and default config */
int yk3_qos_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_qos *qos = NULL;
	int ret = 0;

	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	BUILD_BUG_ON(sizeof(struct yk3_qos_msg) > YK3_MBOX_DATA_LEN);
	ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_QOS,
					 yk3_qos_mbox_cb, pdev_priv);
	if (ret) {
		yk3_dev_err("register qos mbox callback failed");
		return ret;
	}

	qos = kzalloc(sizeof(*qos), GFP_KERNEL);
	if (!qos)
		return -ENOMEM;
	pdev_priv->qos = qos;

	qos->pdev = pdev_priv->pdev;
	qos->hw_base = pdev_priv->bar_addr[YK3_BAR0] + YK3_QOS_BASE_ADDR;
	spin_lock_init(&qos->lock);

	yk3_qos_debug_init(qos);

	yk3_qos_init_sw(qos);

	yk3_qos_init_hw(qos);

	ret = yk3_qos_init_topo(qos);
	if (ret) {
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_QOS);
		debugfs_remove(qos->dbgfs_info_file);
		kfree(qos);
		pdev_priv->qos = NULL;
	}

	return ret;
}

void yk3_qos_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_qos *qos = NULL;

	if (!pdev_priv) {
		yk3_err("%s failed, ret=%d", __func__, -EINVAL);
		return;
	}

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_QOS);

	qos = pdev_priv->qos;
	if (!qos) {
		yk3_err("%s failed, ret=%d", __func__, -EINVAL);
		return;
	}

	debugfs_remove(qos->dbgfs_info_file);
	qos->dbgfs_info_file = NULL;

	kfree(qos->root);

	kfree(qos);
	pdev_priv->qos = NULL;
}

