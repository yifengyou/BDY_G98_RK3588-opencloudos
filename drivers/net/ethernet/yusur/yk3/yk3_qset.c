// SPDX-License-Identifier: GPL-2.0

#include "yk3.h"
#include "yk3_edma_priv.h"
#include "yk3_qset_priv.h"

void yk3_qset_set_qset2q(struct yk3_pdev_priv *pdev_priv, u16 qsetid,
			 struct yk3_queuebase qbase)
{
	u32 val = 0;

	val = FIELD_PREP(YK3_RE_QSET2Q_QSTART_GMASK, qbase.start);
	val |= FIELD_PREP(YK3_RE_QSET2Q_QNUM_GMASK, qbase.num);
	if (qbase.num) {
		val |= FIELD_PREP(YK3_RE_QSET2Q_VALID_GMASK, 1);
		val |= FIELD_PREP(YK3_RE_QSET2Q_RSS_EN_GMASK, 1);
	}

	yk3_wr32(yk3_edma_hwaddr(pdev_priv), YK3_RE_QSET2Q(qsetid), val);
}

void yk3_qset_set_q2qset(struct yk3_pdev_priv *pdev_priv, u16 qsetid,
			 struct yk3_queuebase qbase)
{
	u32 val;
	int i;

	for (i = qbase.start; i < (qbase.start + qbase.num); i++) {
		val = FIELD_PREP(YK3_RE_Q2QSET_QSETID_GMASK, qsetid);
		yk3_wr32(yk3_edma_hwaddr(pdev_priv), YK3_RE_Q2QSET(i), val);
	}
}

int yk3_qset_start(struct yk3_ndev_priv *ndev_priv, u16 txqnum, u16 rxqnum)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_queuebase qbase;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qset_msg *msg;
	struct yk3_mbox_option opt = {0};
	int ret = 0;

	if (yk3_pdev_is_pf(pdev_priv)) {
		qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
		qbase.num = min_t(u16, qbase.num, rxqnum);
		yk3_qset_set_qset2q(pdev_priv, ndev_priv->qsetid, qbase);
		qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
		qbase.num = min_t(u16, qbase.num, txqnum);
		yk3_qset_set_q2qset(pdev_priv, ndev_priv->qsetid, qbase);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_msg.opcode = YK3_MBOX_OPCODE_QSET;
		msg = (struct yk3_qset_msg *)mbox_msg.data;
		msg->msgid = MSG_QSET_START;
		msg->qset_start.req.qsetid = ndev_priv->qsetid;
		msg->qset_start.req.rx_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
		msg->qset_start.req.rx_qbase.num = rxqnum;
		msg->qset_start.req.tx_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
		msg->qset_start.req.tx_qbase.num = txqnum;

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	} else {
		yk3_dev_err("unsupported pdev type");
		ret = -EOPNOTSUPP;
	}

	return ret;
}

void yk3_qset_stop(struct yk3_ndev_priv *ndev_priv)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_queuebase qbase = {0, 0};
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qset_msg *msg;
	struct yk3_mbox_option opt = {0};

	if (yk3_pdev_is_pf(pdev_priv)) {
		yk3_qset_set_qset2q(pdev_priv, ndev_priv->qsetid, qbase);
		qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];
		yk3_qset_set_q2qset(pdev_priv, 0, qbase);
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		mbox_msg.opcode = YK3_MBOX_OPCODE_QSET;
		msg = (struct yk3_qset_msg *)mbox_msg.data;
		msg->msgid = MSG_QSET_STOP;
		msg->qset_stop.req.qsetid = ndev_priv->qsetid;
		msg->qset_stop.req.tx_qbase = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL];

		opt.wait_reply = MB_NO_REPLY;
		opt.timeout = 0;
		yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL);
	}
}

int yk3_qsetid_alloc(struct yk3_pdev_priv *pdev_priv, enum yk3_ndev_type type)
{
	struct yk3_card *card = pdev_priv->card;
	struct yk3_qset_table *table;
	int ret;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_msg mbox_ack_msg = {0};
	struct yk3_qset_msg *msg;
	struct yk3_mbox_option opt = {0};

	if (card->mode == YK3_MODE_ECARD && yk3_pdev_is_pf(pdev_priv))
		return pdev_priv->pf_id;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		table = card->qset_table;
		spin_lock(&table->slock);
		ret = idr_alloc(&table->idr, table, table->idr_start, table->idr_end, GFP_ATOMIC);
		spin_unlock(&table->slock);
		if (ret < 0)
			yk3_dev_err("alloc qsetid failed");
		return ret;
	}

	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QSET;
	msg = (struct yk3_qset_msg *)mbox_msg.data;

	opt.wait_reply = MB_WAIT_REPLY;
	opt.timeout = 1000;

	if (yk3_pdev_is_pf(pdev_priv)) {
		msg->msgid = MSG_QSETID_ALLOC;
		msg->qsetid_alloc.req.pf_id = pdev_priv->pf_id;
		msg->qsetid_alloc.req.vf_id = pdev_priv->vf_id;
		msg->qsetid_alloc.req.type = type;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
		if (ret) {
			yk3_dev_err("qsetid alloc mbox failed");
			return ret;
		}

		msg = (struct yk3_qset_msg *)mbox_ack_msg.data;
		ret = msg->qsetid_alloc.rsp.qsetid;
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		msg->msgid = MSG_QSETID_GET;
		msg->qsetid_get.req.pf_id = pdev_priv->pf_id;
		msg->qsetid_get.req.vf_id = pdev_priv->vf_id;
		msg->qsetid_get.req.type = type;

		ret = yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, &mbox_ack_msg);
		if (ret) {
			yk3_dev_err("qsetid get mbox failed");
			return ret;
		}

		msg = (struct yk3_qset_msg *)mbox_ack_msg.data;
		ret = msg->qsetid_get.rsp.qsetid;
	} else {
		yk3_dev_err("unsupported pdev type");
		ret = -EOPNOTSUPP;
	}

	if (ret < 0)
		yk3_dev_err("qsetid alloc failed");

	return ret;
}

void yk3_qsetid_free(struct yk3_pdev_priv *pdev_priv, u16 qsetid)
{
	struct yk3_card *card = pdev_priv->card;
	struct yk3_qset_table *table;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_qset_msg *msg;
	struct yk3_mbox_option opt = {0};

	if (card->mode == YK3_MODE_ECARD && yk3_pdev_is_pf(pdev_priv))
		return;

	if (yk3_pdev_is_mgr(pdev_priv)) {
		table = card->qset_table;

		spin_lock(&table->slock);
		idr_remove(&table->idr, qsetid);
		spin_unlock(&table->slock);

		return;
	}

	/* mbox */
	mbox_msg.dst_id = yk3_mbox_master_id();
	mbox_msg.opcode = YK3_MBOX_OPCODE_QSET;
	msg = (struct yk3_qset_msg *)mbox_msg.data;
	msg->msgid = MSG_QSETID_FREE;
	msg->qsetid_free.req.qsetid = qsetid;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;

	if (yk3_mbox_send_msg(pdev_priv, &mbox_msg, &opt, NULL))
		yk3_dev_err("qsetid free mbox failed");
}

int yk3_qsetid_get_peer(struct yk3_pdev_priv *pdev_priv, struct yk3_ndev_priv *ndev_priv)
{
	return -1;
}

static void yk3_qset_mbox_cb(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = param;
	struct yk3_qset_msg *req, *rsp;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option opt;
	struct yk3_queuebase qbase;
	struct yk3_sriov_priv *sriov_priv;
	u16 vfidx;
	int ret;

	if (msg->opcode != YK3_MBOX_OPCODE_QSET) {
		yk3_dev_err("qset mbox cb, opcode mismatch");
		return;
	}

	req = (struct yk3_qset_msg *)msg->data;
	rsp = (struct yk3_qset_msg *)ack_msg.data;

	rsp->msgid = req->msgid;

	switch (req->msgid) {
	case MSG_QSETID_ALLOC:
		ret = yk3_qsetid_alloc(pdev_priv, req->qsetid_alloc.req.type);
		rsp->qsetid_alloc.rsp.qsetid = ret;
		break;
	case MSG_QSETID_FREE:
		yk3_qsetid_free(pdev_priv, req->qsetid_free.req.qsetid);
		return;
	case MSG_QSETID_GET:
		sriov_priv = yk3_sriov_get_priv(pdev_priv);
		vfidx = req->qsetid_get.req.vf_id - 1;
		if (!sriov_priv || !yk3_sriov_get_vfinfo(sriov_priv, vfidx))
			ret = -EINVAL;
		else
			ret = yk3_sriov_get_vfinfo(sriov_priv, vfidx)->qsetid;
		if (sriov_priv)
			yk3_sriov_put_priv(sriov_priv);
		rsp->qsetid_get.rsp.qsetid = ret;
		break;
	case MSG_QSETID_GET_PEER:
		rsp->qsetid_get_peer.rsp.qsetid = -1;
		break;
	case MSG_QSET_START:
		qbase = req->qset_start.req.rx_qbase;
		yk3_qset_set_qset2q(pdev_priv, req->qset_start.req.qsetid, qbase);
		qbase = req->qset_start.req.tx_qbase;
		yk3_qset_set_q2qset(pdev_priv, req->qset_start.req.qsetid, qbase);
		return;
	case MSG_QSET_STOP:
		qbase.start = 0;
		qbase.num = 0;
		yk3_qset_set_qset2q(pdev_priv, req->qset_stop.req.qsetid, qbase);
		qbase = req->qset_stop.req.tx_qbase;
		yk3_qset_set_q2qset(pdev_priv, 0, qbase);
		return;
	default:
		yk3_dev_err("qset msgid not support");
		return;
	}

	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;

	opt.wait_reply = MB_NO_REPLY;
	opt.timeout = 0;
	yk3_mbox_send_msg(pdev_priv, &ack_msg, &opt, NULL);
}

int yk3_qset_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_card *card = pdev_priv->card;
	struct yk3_qset_table *table;
	int ret;

	if (yk3_pdev_is_mgr(pdev_priv) || yk3_pdev_is_pf(pdev_priv)) {
		ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_QSET,
						 yk3_qset_mbox_cb, pdev_priv);
		if (ret) {
			yk3_dev_err("register qset mbox callback failed");
			return ret;
		}
	}

	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	spin_lock_init(&table->slock);
	idr_init(&table->idr);
	idr_init(&table->rep_idr);

	switch (card->mode) {
	case YK3_MODE_TCARD:
	case YK3_MODE_RCARD:
		table->idr_start = pdev_priv->card->qsetid_base;
		table->idr_end = pdev_priv->card->qsetid_num;
		break;
	default:
		yk3_dev_err("only support tcard and rcard now");
		return -EINVAL;
	}

	card->qset_table = table;

	return 0;
}

void yk3_qset_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (yk3_pdev_is_mgr(pdev_priv) || yk3_pdev_is_pf(pdev_priv))
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_QSET);

	if (!yk3_pdev_is_mgr(pdev_priv))
		return;

	kfree(pdev_priv->card->qset_table);
	pdev_priv->card->qset_table = NULL;
}
