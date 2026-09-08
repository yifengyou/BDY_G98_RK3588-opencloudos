// SPDX-License-Identifier: GPL-2.0

#include "yk3_mbox_priv.h"

static u32 yk3_mbox_base = YK3_MBOX_DPU_HOST_BASE;

static inline u16 slot_to_opcode(u16 slot)
{
	u16 opcode = slot;

	/* for emp */
	if (slot >= 128 && slot < 256)
		opcode = slot - 128 + YK3_MBOX_EMP_CB_START;

	return opcode;
}

static int yk3_pdev_mbox_debugfs_show(struct seq_file *seq, void *v)
{
	struct yk3_pdev_priv *pdev_priv = seq->private;
	struct yk3_mbox *mbox = pdev_priv->mbox;
	int i, idx;

	seq_printf(seq, "\t    %-16s : %-16d\n", "err_locked",
		   atomic_read(&mbox->stats.err_locked));
	seq_printf(seq, "\t    %-16s : %-16d\n", "err_not_take",
		   atomic_read(&mbox->stats.err_not_take));
	seq_printf(seq, "\t    %-16s : %-16d\n", "err_resp_timeout",
		   atomic_read(&mbox->stats.err_resp_timeout));
	seq_printf(seq, "\t    %-16s : %-16d\n", "err_invalid_msg", mbox->stats.err_invalid_msg);
	seq_printf(seq, "\t    %-16s : %-16d\n", "err_irq_lost", mbox->stats.err_irq_lost);
	seq_printf(seq, "\t    %-16s : %-16d\n", "cnt_tx", atomic_read(&mbox->stats.cnt_tx));
	seq_printf(seq, "\t    %-16s : %-16d\n", "cnt_rx", mbox->stats.cnt_rx);
	seq_printf(seq, "\t    %-16s : %-16d\n", "cnt_fwd", mbox->stats.cnt_fwd);
	seq_printf(seq, "\t    %-16s : %-16d\n", "cnt_atomic", mbox->stats.cnt_atomic);

	for (i = 0; i < YK3_MBOX_CB_NUM; i++) {
		int tx_req = atomic_read(&mbox->opcode_tx_req_cnt[i]);
		int tx_resp = atomic_read(&mbox->opcode_tx_resp_cnt[i]);
		int rx_req = atomic_read(&mbox->opcode_rx_req_cnt[i]);
		int rx_resp = atomic_read(&mbox->opcode_rx_resp_cnt[i]);

		if (!tx_req && !tx_resp && !rx_req && !rx_resp)
			continue;

		seq_printf(seq, "\t    opcode:%-9x : tx-req: %-8d tx-resp: %-8d rx-req: %-8d rx-resp: %-8d\n",
			   slot_to_opcode(i),
			   atomic_read(&mbox->opcode_tx_req_cnt[i]),
			   atomic_read(&mbox->opcode_tx_resp_cnt[i]),
			   atomic_read(&mbox->opcode_rx_req_cnt[i]),
			   atomic_read(&mbox->opcode_rx_resp_cnt[i]));
	}

	idx = atomic_read(&pdev_priv->mbox->stats.cur) & RB_MASK;

	for (i = 0; i < RB_SIZE; i++) {
		idx = ((idx + 1) & RB_MASK);
		if (!strlen(pdev_priv->mbox->stats.buffer[idx]))
			continue;
		seq_printf(seq, "%s\n", pdev_priv->mbox->stats.buffer[idx]);
	}

	return 0;
}

DEFINE_SHOW_ATTRIBUTE(yk3_pdev_mbox_debugfs);

static int yk3_debug_mbox_init(struct yk3_pdev_priv *pdev_priv)
{
	struct dentry *entry;

	if (!pdev_priv->dbgfs_dir)
		return -ENOENT;

	entry = debugfs_create_file("mbox_info", 0444, pdev_priv->dbgfs_dir, pdev_priv,
				    &yk3_pdev_mbox_debugfs_fops);
	if (!entry) {
		yk3_dev_err("Failed to create debugfs irq info file for pdev %s", pdev_priv->name);
		return -ENOMEM;
	}
	pdev_priv->mbox->dbgfs_info_file = entry;

	return 0;
}

static void yk3_debug_mbox_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (!pdev_priv->mbox)
		return;

	debugfs_remove(pdev_priv->mbox->dbgfs_info_file);
	pdev_priv->mbox->dbgfs_info_file = NULL;
}

static inline const char *mb_role_str(u16 chn)
{
	struct yk3_mbox_id id = *((struct yk3_mbox_id *)&chn);

	switch (id.type) {
	case MB_VF_TYPE:
		return "VF";
	case MB_PF_OR_EMP_TYPE:
		if (id.func_id == YK3_MBOX_EMP_ID)
			return "EMP";
		if (id.func_id == YK3_MBOX_MASTER_PF_ID)
			return "MGR";
		return "PF";
	default:
		return "Unsupported MB ROLE";
	}
}

static inline int mb_func_id(u16 chn)
{
	struct yk3_mbox_id id = *((struct yk3_mbox_id *)&chn);

	switch (id.type) {
	case MB_VF_TYPE:
		return id.func_id;
	case MB_PF_OR_EMP_TYPE:
		if (id.func_id == YK3_MBOX_EMP_ID)
			return 0;
		if (id.func_id == YK3_MBOX_MASTER_PF_ID)
			return 0;
		return id.func_id;
	default:
		return 0;
	}
}

static inline u16 yk3_mbox_my_id(struct yk3_pdev_priv *pdev_priv)
{
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;

	mb_id->type = MB_PF_OR_EMP_TYPE;
	if (pdev_priv->mbox->role == MB_VF) {
		mb_id->type = MB_VF_TYPE;
		mb_id->func_id = pdev_priv->vf_id - 1;
	} else if (pdev_priv->mbox->role == MB_PF) {
		mb_id->func_id = pdev_priv->pf_id;
	} else if (pdev_priv->mbox->role == MB_MASTER) {
		mb_id->func_id = YK3_MBOX_MASTER_PF_ID;
	} else {
		id = 0;
	}

	return id;
}

static inline void yk3_mbox_memcpy_fromio(void *buffer, void __iomem *addr, size_t size)
{
	u32 i;

	for (i = 0; i < size / sizeof(u32); i++)
		*((u32 *)buffer + i) = yk3_rd32(addr, i * sizeof(u32));
}

static inline void yk3_mbox_memcpy_toio(void __iomem *addr, void *buffer, size_t size)
{
	u32 i;

	for (i = 0; i < size / sizeof(u32); i++)
		yk3_wr32(addr, i * sizeof(u32), *((u32 *)buffer + i));
}

static inline void yk3_mbox_memset_io(void __iomem *addr, int value, size_t size)
{
	u32 i;

	for (i = 0; i < size / sizeof(u32); i++)
		yk3_wr32(addr, i * sizeof(u32), value);
}

static inline bool is_opcode_valid(u16 opcode)
{
	/* opcode
	 * [0,127]   for host driver
	 * [128,255] for emp
	 */

	/* invalid range: [128,1024) [1152,65535]*/
	if ((opcode > 127 && opcode < YK3_MBOX_EMP_CB_START) ||
	    (opcode >= (YK3_MBOX_EMP_CB_START + 128))) {
		yk3_err("%s failed, opcode %x is out of range\n", __func__, opcode);
		return false;
	}

	return true;
}

static inline u16 opcode_to_slot(u16 opcode)
{
	u16 slot = opcode;

	/* for emp */
	if (opcode >= YK3_MBOX_EMP_CB_START && opcode < (YK3_MBOX_EMP_CB_START + 128))
		slot = opcode - YK3_MBOX_EMP_CB_START + 128;

	return slot;
}

static void yk3_mbox_get_send_offset(struct yk3_mbox *mbox,
				     u16 send_id,
				     struct yk3_mbox_offset_ctx *offset_ctx)
{
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_mbox_id *dst_id;
	struct yk3_mbox_priv *yk3_mbox_priv;
	u32 offset = 0;
	u32 trigger_offset = 0;
	u32 trigger_id = 0;
	u32 pf_id;

	pdev_priv = pci_get_drvdata(mbox->pdev);
	dst_id = (struct yk3_mbox_id *)&send_id;
	yk3_mbox_priv = (struct yk3_mbox_priv *)mbox->mb_priv;

	if (mbox->role == MB_MASTER) {
		switch (dst_id->type) {
		case MB_PF_OR_EMP_TYPE:
			if (dst_id->func_id == YK3_MBOX_MASTER_PF_ID) {
				/* master -> master */
				offset = YK3_MBOX_M2M_CHN + YK3_MBOX_SH_BUF_TH;
				//if (pdev_priv->dpu_mode == MODE_DPU_SOC)
				//	offset = YK3_MBOX_M2M_CHN + YK3_MBOX_SH_BUF_BH;
				trigger_offset = YK3_MBOX_H2S_IRQ_TRIGGER;
			} else if (dst_id->func_id == YK3_MBOX_EMP_ID) {
				offset = YK3_MBOX_M2EMP_CHN + YK3_MBOX_SH_BUF_TH;
				trigger_offset = YK3_MBOX_H2M_IRQ_TRIGGER;
			} else {
				if (dst_id->func_id > YK3_MBOX_LF_END)
					dbgf("mbox dst pf id err, pf id:%u", dst_id->func_id);
				/* master -> pf */
				pf_id = yk3_mbox_priv->pf2lf_table[dst_id->func_id];
				offset = YK3_MBOX_PF2PF_BUF_OFFSET(pf_id) + YK3_MBOX_SH_BUF_TH;
				trigger_offset = YK3_MBOX_PF2PF_IRQ_TRIGGER;
				trigger_id = dst_id->func_id;
			}
			break;
		/* forward to gateway pf */
		/* WARNING master pf no vf */
		case MB_VF_TYPE:
			/* check pf id */
			if (dst_id->gateway > YK3_MBOX_LF_END) {
				dbgf("mbox dst invalid pf id, pf id:%u", dst_id->gateway);
				break;
			}
			/* check vf id */
			if (dst_id->func_id >= YK3_MBOX_MAX_VF) {
				dbgf("mbox dst vf id err, vf id:%u", dst_id->func_id);
				break;
			}
			/* master -> pf(which create vf) */
			pf_id = yk3_mbox_priv->pf2lf_table[dst_id->gateway];
			offset = YK3_MBOX_PF2PF_BUF_OFFSET(pf_id) + YK3_MBOX_SH_BUF_TH;
			trigger_offset = YK3_MBOX_PF2PF_IRQ_TRIGGER;
			trigger_id = dst_id->gateway;
			break;
		default:
			dbgf("mbox get unknown dst id 0x%04x!", send_id);
		}
	} else if (mbox->role == MB_PF) {
		switch (dst_id->type) {
		case MB_PF_OR_EMP_TYPE:
			/* pf -> master */
			offset = YK3_MBOX_PF2PF_BUF_BASE +
				 YK3_MBOX_SH_BUF_BH;
			trigger_offset = YK3_MBOX_PF2PF_IRQ_TRIGGER;
			break;
		case MB_VF_TYPE:
			/* check vf id */
			if (dst_id->func_id >= YK3_MBOX_MAX_VF)
				dbgf("mbox dst vf id err, vf id:%u", dst_id->func_id);
			/* pf -> vf */
			offset = YK3_MBOX_VF_PF_BUF_OFFSET(dst_id->func_id) +
				 YK3_MBOX_SH_BUF_TH;
			trigger_offset = YK3_MBOX_PF2VF_IRQ_TRIGGER;
			trigger_id = dst_id->func_id + 1;
			break;
		default:
			dbgf(" dst type is unknown!!");
		}
	} else if (mbox->role == MB_VF) {
		/* vf -> pf */
		offset = YK3_MBOX_VF_PF_BUF_BASE + YK3_MBOX_SH_BUF_BH;
		trigger_offset = YK3_MBOX_VF_IRQ_TRIGGER;
	}

	offset_ctx->offset = offset;
	offset_ctx->trigger_offset = trigger_offset;
	offset_ctx->trigger_id = trigger_id;
}

static void yk3_mbox_hw_send_msg(struct yk3_mbox *mbox, void *data, u16 send_id)
{
	struct yk3_mbox_offset_ctx offset_ctx = {0};

	yk3_mbox_get_send_offset(mbox, send_id, &offset_ctx);
	/* send data */
	yk3_mbox_memcpy_toio(mbox->addr + offset_ctx.offset, data, YK3_MBOX_MSG_LEN);
	/* trigger interrupt */
	yk3_wr32(mbox->addr, offset_ctx.trigger_offset, offset_ctx.trigger_id);
}

static u32 yk3_mbox_get_recv_offset(struct yk3_mbox *mbox, u32 recv_id)
{
	//struct yk3_pdev_priv *pdev_priv;
	struct yk3_mbox_id *src_id;
	struct yk3_mbox_priv *yk3_mbox_priv;
	u32 pf_id;
	u32 offset = 0;

	//pdev_priv = pci_get_drvdata(mbox->pdev);
	yk3_mbox_priv = (struct yk3_mbox_priv *)mbox->mb_priv;
	src_id = (struct yk3_mbox_id *)&recv_id;

	if (mbox->role == MB_MASTER) {
		if (src_id->type == MB_PF_OR_EMP_TYPE) {
			if (src_id->func_id == YK3_MBOX_MASTER_PF_ID) {
				/* recv master msg */
				/* recv host/soc master msg */
				offset = YK3_MBOX_M2M_CHN + YK3_MBOX_SH_BUF_BH;
				//if (pdev_priv->dpu_mode == MODE_DPU_SOC) {
				//	/* recv soc master msg */
				//	offset = YK3_MBOX_M2M_CHN + YK3_MBOX_SH_BUF_TH;
				//}
			} else if (src_id->func_id == YK3_MBOX_EMP_ID) {
				/* recv emp msg */
				offset = YK3_MBOX_M2EMP_CHN + YK3_MBOX_SH_BUF_BH;
			} else {
				pf_id = yk3_mbox_priv->pf2lf_table[src_id->func_id];
				offset = YK3_MBOX_PF2PF_BUF_OFFSET(pf_id) + YK3_MBOX_SH_BUF_BH;
			}
		} else if (src_id->type == MB_VF_TYPE) {
			/* master pf <- vf */
			offset = YK3_MBOX_VF_PF_BUF_OFFSET(src_id->func_id) +
				YK3_MBOX_SH_BUF_BH;
		}
	} else if (mbox->role == MB_PF) {
		if (src_id->type == MB_PF_OR_EMP_TYPE) {
			/* pf <- master/emp */
			/* recv local master msg */
			offset = YK3_MBOX_PF2PF_BUF_BASE +
				 YK3_MBOX_SH_BUF_TH;
		} else if (src_id->type == MB_VF_TYPE) {
			/* pf <- vf */
			offset = YK3_MBOX_VF_PF_BUF_OFFSET(src_id->func_id) +
				 YK3_MBOX_SH_BUF_BH;
		}
	} else if (mbox->role == MB_VF) {
		/* vf <- pf */
		offset = YK3_MBOX_VF_PF_BUF_BASE +
			 YK3_MBOX_SH_BUF_TH;
	}

	return offset;
}

static u8 yk3_mbox_cal_checksum(struct yk3_mbox_msg *mbox_msg)
{
	struct yk3_mbox_msg tmp;
	u8 new_checksum = 0;
	int i;

	if (!mbox_msg)
		return 0;

	memcpy(&tmp, mbox_msg, sizeof(tmp));
	for (i = 0; i < YK3_MBOX_DATA_LEN; i++)
		new_checksum += tmp.data[i];

	return new_checksum;
}

static int yk3_mbox_validate_checksum(struct yk3_mbox_msg *mbox_msg)
{
	return yk3_mbox_cal_checksum(mbox_msg) == mbox_msg->data_chksum;
}

static void yk3_mbox_print(struct yk3_mbox *mbox, struct yk3_mbox_msg *mbox_msg, char *msg)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);

	yk3_dev_debug("%s", msg);
	yk3_dev_debug("mbox_msg->magic: 0x%x\n", mbox_msg->magic);
	yk3_dev_debug("mbox_msg->opcode: 0x%x\n", mbox_msg->opcode);
	yk3_dev_debug("mbox_msg->data_length: 0x%x\n", mbox_msg->data_length);
	yk3_dev_debug("mbox_msg->data_chksum: 0x%x\n", mbox_msg->data_chksum);
	yk3_dev_debug("mbox_msg->dst_id: 0x%x\n", mbox_msg->dst_id);
	yk3_dev_debug("mbox_msg->src_id: 0x%x\n", mbox_msg->src_id);
	yk3_dev_debug("mbox_msg->flag: 0x%x\n",   mbox_msg->flag);
	yk3_dev_debug("mbox_msg->seqno: 0x%x\n",  mbox_msg->seqno);
	yk3_dev_debug("mbox_msg->data[0]: %x\n",  mbox_msg->data[0]);
	yk3_dev_debug("mbox_msg->data[1]: %x\n",  mbox_msg->data[1]);
	yk3_dev_debug("mbox_msg->data[2]: %x\n",  mbox_msg->data[2]);
	yk3_dev_debug("mbox_msg->data[3]: %x\n",  mbox_msg->data[3]);
}

static int yk3_mbox_recv_check(struct yk3_mbox_msg *msg)
{
	if (!msg->opcode)
		return -1;

	if (msg->flag & BIT(YK3_MBOX_FLAG_NO_CHKSUM_BIT))
		return 0;

	if (!yk3_mbox_validate_checksum(msg)) {
		yk3_err("msg checksum error!\n"
			"msg.checksum:%x, checksum:%x,\n"
			"msg.opcode:%x,\n"
			"msg.data:%s\n",
			msg->data_chksum, yk3_mbox_cal_checksum(msg),
			msg->opcode, msg->data);
		return -1;
	}

	return 0;
}

static int yk3_mbox_hw_recv_msg(struct yk3_mbox *mbox, void *data, u32 recv_id)
{
	u32 offset;

	offset = yk3_mbox_get_recv_offset(mbox, recv_id);
	yk3_mbox_memcpy_fromio(data, mbox->addr + offset, YK3_MBOX_MSG_LEN);

	yk3_mbox_print(mbox, (struct yk3_mbox_msg *)data, "===mbox msg recv===");
	return yk3_mbox_recv_check((struct yk3_mbox_msg *)data);
}

static struct yk3_mbox_irq_info yk3_mbox_get_irq_status(struct yk3_mbox *mbox)
{
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_mbox_id *msg_id;
	struct yk3_mbox_irq_info irq_info;
	u32 pending = 0, id;
	u16 neigh_id = 0;

	pdev_priv = pci_get_drvdata(mbox->pdev);
	memset(&irq_info, 0, sizeof(struct yk3_mbox_irq_info));
	msg_id = (struct yk3_mbox_id *)&neigh_id;
	if (mbox->role == MB_MASTER) {
		pending = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
		if (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, pending)) {
			irq_info.irq_status = 1;
			id = FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_PF_ID, pending);
			/* recv pf message */
			msg_id->type = MB_PF_OR_EMP_TYPE;
			msg_id->func_id = id;
			goto irq_data;
		}
	} else if (mbox->role == MB_PF) {
		/* pf <- master */
		pending = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
		if (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, pending)) {
			irq_info.irq_status = 1;
			id = FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_PF_ID, pending);
			/* recv emp master msg */
			/* recv local master msg */
			msg_id->type = MB_PF_OR_EMP_TYPE;
			msg_id->func_id = YK3_MBOX_MASTER_PF_ID;
			goto irq_data;
		}
		/* pf <- vf */
		pending = yk3_rd32(mbox->addr, YK3_MBOX_PF2VF_IRQ_PENDING);
		if (FIELD_GET(YK3_MBOX_PF2VF_IRQ_P_STATUS, pending)) {
			irq_info.irq_status = 1;
			id = FIELD_GET(YK3_MBOX_PF2VF_IRQ_P_VF_ID, pending);
			msg_id->type = MB_VF_TYPE;
			msg_id->func_id = id - 1;
			goto irq_data;
		}
	} else if (mbox->role == MB_VF) {
		/* vf <- pf */
		pending = yk3_rd32(mbox->addr, YK3_MBOX_VF_IRQ_PENDING);
		if (FIELD_GET(YK3_MBOX_VF2PF_IRQ_P_STATUS, pending)) {
			irq_info.irq_status = 1;
			id = FIELD_GET(YK3_MBOX_PF2VF_IRQ_P_VF_ID, pending);
			msg_id->type = MB_PF_OR_EMP_TYPE;
			msg_id->func_id = 0;
			goto irq_data;
		}
	}
irq_data:
	irq_info.msg_id = neigh_id;
	if (pending & GENMASK(31, 31)) //bit[31] is set, means it's an errcode like 0xdeadxxxx
		irq_info.irq_status = 0;
	if (irq_info.irq_status)
		yk3_dev_debug("[IRQ] got irq from %s%d(0x%x), irq_status %d\n",
			      mb_role_str(neigh_id), mb_func_id(neigh_id), neigh_id,
			      irq_info.irq_status);
	return irq_info;
}

static void yk3_mbox_clear_send_mailbox(struct yk3_mbox *mbox, u16 clear_id)
{
	struct yk3_mbox_offset_ctx offset_ctx;

	memset(&offset_ctx, 0, sizeof(struct yk3_mbox_offset_ctx));
	yk3_mbox_get_send_offset(mbox, clear_id, &offset_ctx);

	yk3_mbox_memset_io(mbox->addr + offset_ctx.offset, 0, YK3_MBOX_MSG_LEN);
}

static void yk3_mbox_clear_recv_mailbox(struct yk3_mbox *mbox, u32 clear_id)
{
	u32 offset = 0;

	offset = yk3_mbox_get_recv_offset(mbox, clear_id);

	yk3_mbox_memset_io(mbox->addr + offset, 0, YK3_MBOX_MSG_LEN);
}

static bool yk3_mbox_check_mailbox_unlocked(struct yk3_mbox *mbox, u16 send_id)
{
	struct yk3_mbox_offset_ctx offset_ctx = {0};
	struct yk3_mbox_msg mbox_msg, null_msg = {0};
	u32 first_dword = 0;

	yk3_mbox_get_send_offset(mbox, send_id, &offset_ctx);

	yk3_mbox_memcpy_fromio((void *)&first_dword, mbox->addr + offset_ctx.offset, sizeof(u32));
	if (first_dword > 0 && first_dword < 0xFFFFFFFF)
		return false;

	yk3_mbox_memcpy_fromio((void *)&mbox_msg, mbox->addr + offset_ctx.offset, YK3_MBOX_MSG_LEN);

	if (memcmp(&mbox_msg, &null_msg, sizeof(mbox_msg)) == 0)
		return true;

	return false;
}

static inline u32 hash_function(u16 opcode, s32 seqno)
{
	return (opcode + seqno) % YK3_MBOX_WAIT_REPLY_SIZE;
}

static inline u16 get_chn_mlock_id(struct yk3_mbox *mbox, u16 msg_id)
{
	struct yk3_mbox_id *msg_dst_id = (struct yk3_mbox_id *)&msg_id;

	switch (mbox->role) {
	case MB_MASTER:
		if (msg_dst_id->type == MB_VF_TYPE) // mgr -> vf
			return yk3_mbox_pf_id(msg_dst_id->gateway);
		return msg_id;
	case MB_PF:
		if (msg_dst_id->type == MB_PF_OR_EMP_TYPE) // pf -> mgr || pf -> emp
			return yk3_mbox_master_id();
		return msg_id;
	case MB_VF:
		return yk3_mbox_pf_id(0); // vf -> any
	default:
		return msg_id;
	}
}

static int yk3_mbox_send_logic(struct yk3_mbox *mbox,
			       struct yk3_mbox_msg *send_msg,
			       bool wait_reply,
			       u32 expect_opcode,
			       u32 reply_timeout,
			       struct yk3_mbox_msg *recv_msg)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	unsigned long timeout;
	bool unlocked = false;
	u16 dst_id;
	long wait_ret;
	struct yk3_mbox_wait_reply_node wr_node;
	u32 bucket = 0;
	u16 slot;
	int ret = YK3_MBOX_SEND_OK;

	dst_id = send_msg->dst_id % YK3_MBOX_MAX_CHANNEL;
	timeout = jiffies + msecs_to_jiffies(YK3_MBOX_UNLOCK_TIMEOUT);
	while (time_before(jiffies, timeout)) {
		if (yk3_mbox_check_mailbox_unlocked(mbox, dst_id)) {
			unlocked = true;
			break;
		}
		usleep_range(1000, 2000);
	}
	if (!unlocked) {
		dbgf("%s:mbox is locked for more than %dms, clearing lock, to:%s%d(0x%x)",
		     __func__, YK3_MBOX_UNLOCK_TIMEOUT,
		     mb_role_str(send_msg->dst_id), mb_func_id(send_msg->dst_id),
		     send_msg->dst_id);
		yk3_mbox_clear_send_mailbox(mbox, dst_id);
		atomic_inc(&mbox->stats.err_locked);
		return YK3_MBOX_SEND_L2_FAILED;
	}

	if (wait_reply) {
		wr_node.opcode = expect_opcode;
		wr_node.seqno = send_msg->seqno;
		bucket = hash_function(wr_node.opcode, wr_node.seqno);

		init_completion(&wr_node.comp);
		mutex_lock(&mbox->wait_reply_mlock);
		hlist_add_head(&wr_node.node, &mbox->wait_reply_hlist[bucket]);
		mutex_unlock(&mbox->wait_reply_mlock);
	}

	mutex_lock(&mbox->chn_mlock[get_chn_mlock_id(mbox, dst_id)]);

	/* 执行硬件发送动作 */
	yk3_mbox_hw_send_msg(mbox, (void *)send_msg, dst_id);
	yk3_dev_debug("%s : send to dst %s%d(0x%x) (opcode %02x seq %d) expect %02x\n",
		      __func__, mb_role_str(dst_id), mb_func_id(dst_id), dst_id,
		      send_msg->opcode, send_msg->seqno, expect_opcode);

	unlocked = false;
	timeout = jiffies + msecs_to_jiffies(YK3_MBOX_UNLOCK_TIMEOUT);
	while (time_before(jiffies, timeout)) {
		if (yk3_mbox_check_mailbox_unlocked(mbox, dst_id)) {
			unlocked = true;
			break;
		}
		usleep_range(50, 100);
	}

	mutex_unlock(&mbox->chn_mlock[get_chn_mlock_id(mbox, dst_id)]);

	if (!unlocked) {
		dbgf("%s : %s%d(0x%x) did not take the message, opcode %x seq %d",
		     __func__, mb_role_str(dst_id), mb_func_id(dst_id), dst_id,
		     send_msg->opcode, send_msg->seqno);

		atomic_inc(&mbox->stats.err_not_take);
		if (wait_reply) {
			ret = YK3_MBOX_SEND_L2_FAILED;
			goto clean_return;
		}

		return YK3_MBOX_SEND_L2_FAILED;
	}

	slot = opcode_to_slot(send_msg->opcode & GENMASK(14, 0));
	if (send_msg->opcode >> YK3_MBOX_OPCODE_MASK_ACK)
		atomic_inc(&mbox->opcode_tx_resp_cnt[slot]);
	else
		atomic_inc(&mbox->opcode_tx_req_cnt[slot]);

	if (!wait_reply)
		return YK3_MBOX_SEND_OK;

	wait_ret = wait_for_completion_timeout(&wr_node.comp, msecs_to_jiffies(reply_timeout));

	if (!wait_ret) {
		dbgf("%s : wait resp from %s%d(0x%x) bucket %u (op: %x seq:%d) timed out",
		     __func__, mb_role_str(dst_id), mb_func_id(dst_id), dst_id, bucket,
		     expect_opcode, send_msg->seqno);

		atomic_inc(&mbox->stats.err_resp_timeout);
		ret = YK3_MBOX_SEND_L3_NO_ACK;
		goto clean_return;
	}

	*recv_msg = mbox->resp_msg[bucket];
	yk3_dev_debug("%s : msg has arrived from %s%d(0x%x) (op: %x seq:%d)\n",
		      __func__, mb_role_str(dst_id), mb_func_id(dst_id), dst_id,
		      recv_msg->opcode, recv_msg->seqno);

clean_return:
	mutex_lock(&mbox->wait_reply_mlock);
	hlist_del_init(&wr_node.node);
	mutex_unlock(&mbox->wait_reply_mlock);

	memset(&mbox->resp_msg[bucket], 0, sizeof(mbox->resp_msg[bucket]));
	return ret;
}

int yk3_mbox_send_msg_atomic(struct yk3_pdev_priv *pdev_priv, struct yk3_mbox_msg *send_msg)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	u32 copied;

	if (kfifo_is_full(&mbox->atomic_fifo))
		return -EAGAIN;

	spin_lock(&mbox->atomic_fifo_lock);
	copied = kfifo_in(&pdev_priv->mbox->atomic_fifo, send_msg, 1);
	spin_unlock(&mbox->atomic_fifo_lock);
	if (!copied)
		return -EAGAIN;

	queue_work(system_unbound_wq, &mbox->work_atomic);
	return 0;
}

int yk3_mbox_send_msg(struct yk3_pdev_priv *pdev_priv,
		      struct yk3_mbox_msg *send_msg,
		      struct yk3_mbox_option *option,
		      struct yk3_mbox_msg *recv_msg)
{
	int ret = 0;
	static atomic_t seq = ATOMIC_INIT(400000000);

	if (!pdev_priv->mbox || !send_msg || !option) {
		yk3_dev_err("%s error, param is null\n", __func__);
		return -EINVAL;
	}

	send_msg->magic = YK3_MBOX_MAGIC_DATA;
	send_msg->src_id = yk3_mbox_my_id(pdev_priv);
	send_msg->data_chksum = yk3_mbox_cal_checksum(send_msg);
	if (option->wait_reply == MB_NO_REPLY)
		send_msg->flag |= BIT(YK3_MBOX_FLAG_NO_REPLY_BIT);
	else
		send_msg->flag &= ~BIT(YK3_MBOX_FLAG_NO_REPLY_BIT);

	if (send_msg->opcode >> YK3_MBOX_OPCODE_MASK_ACK == 0 && option->wait_reply)
		send_msg->seqno = atomic_inc_return(&seq);

	ret = yk3_mbox_send_logic(pdev_priv->mbox, send_msg, option->wait_reply,
				  send_msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK),
				  option->timeout > YK3_MBOX_REPLY_TIMEOUT ?
				  option->timeout : YK3_MBOX_REPLY_TIMEOUT,
				  recv_msg);
	if (ret != 0) {
		dbgf("mailbox send failed, dst %s%d(0x%x), ret %d opcode %x\n",
		     mb_role_str(send_msg->dst_id), mb_func_id(send_msg->dst_id),
		     send_msg->dst_id, ret, send_msg->opcode);
	} else {
		atomic_inc(&pdev_priv->mbox->stats.cnt_tx);
	}

	return ret;
}

static bool yk3_mbox_msg_is_local(struct yk3_pdev_priv *pdev_priv,
				  u16 src_id, struct yk3_mbox_msg *msg)
{
	struct yk3_mbox *mbox = pdev_priv->mbox;
	struct yk3_mbox_id *msg_dst_id = (struct yk3_mbox_id *)&msg->dst_id;
	struct yk3_mbox_id *msg_src_id = (struct yk3_mbox_id *)&msg->src_id;
	struct yk3_mbox_id *neigh_id = (struct yk3_mbox_id *)&src_id;
	u16 local_type = mbox->role != MB_VF ? MB_PF_OR_EMP_TYPE : MB_VF_TYPE;

	if (local_type != msg_dst_id->type)
		return false;

	switch (mbox->role) {
	case MB_MASTER:
		if (msg_dst_id->func_id == YK3_MBOX_MASTER_PF_ID)
			return true;
		break;
	case MB_PF:
		if (neigh_id->type == MB_VF_TYPE && msg_dst_id->func_id == 0)
			return true;
		if (msg_dst_id->func_id == pdev_priv->pf_id)
			return true;
		break;
	case MB_VF:
		if (msg_dst_id->func_id == pdev_priv->vf_id - 1) {
			// clear pf src id for vf receiver
			if (msg_src_id->func_id != YK3_MBOX_MASTER_PF_ID &&
			    msg_src_id->func_id != YK3_MBOX_EMP_ID)
				msg_src_id->func_id = 0;
			return true;
		}
		break;
	default:
		break;
	}

	return false;
}

/*
 * VF0 ←---↘
 * VF1 ←---→ PF0 ←---→  Host Master  ←---→ EMP
 *                     ↗           ↖       ↕
 * VF2 ←---→ PF1 ←---↗              ↖---→ Soc Master
 * VF3 ←---↗
 */
static void request_deal(struct yk3_mbox *mbox, struct yk3_mbox_msg *msg)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	u16 slot;

	if (msg->opcode >> YK3_MBOX_OPCODE_MASK_ACK)
		return;

	if (!is_opcode_valid(msg->opcode)) {
		dbgf("[BH] %s failed, opcode 0x%0x invalid\n", __func__, msg->opcode);
		mbox->stats.err_invalid_msg++;
		return;
	}

	slot = opcode_to_slot(msg->opcode);
	mbox->opcode_cb[slot](msg, mbox->opcode_cb_param[slot]);
	atomic_dec(&mbox->opcode_cb_run_cnt[slot]);
}

static void response_deal(struct yk3_mbox *mbox, struct yk3_mbox_msg *msg)
{
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_wait_reply_node *entry;
	u32 bucket = hash_function(msg->opcode, msg->seqno);

	if (!(msg->opcode >> YK3_MBOX_OPCODE_MASK_ACK))
		return;

	if (hlist_empty(&mbox->wait_reply_hlist[bucket])) {//sender已超时退出
		yk3_dev_warn("[RESP] resp deal: nobody waits for (opcode: %x seqno: %d)",
			     msg->opcode, msg->seqno);
		return;
	}

	mutex_lock(&mbox->wait_reply_mlock);
	hlist_for_each_entry(entry, &mbox->wait_reply_hlist[bucket], node) {
		if (msg->opcode == entry->opcode && msg->seqno == entry->seqno) {
			mbox->resp_msg[bucket] = *msg;
			complete(&entry->comp);
			mutex_unlock(&mbox->wait_reply_mlock);
			return;
		}
	}
	mutex_unlock(&mbox->wait_reply_mlock);
}

static bool msg_must_drop(struct yk3_mbox *mbox, u16 opcode)
{
	u16 slot;

	/* opcode invalid */
	if (!is_opcode_valid(opcode))
		return true;

	slot = opcode_to_slot(opcode);

	/* cb不存在或者置位stopping */
	if (!mbox->opcode_cb[slot] || test_bit(slot, mbox->opcode_cb_stopping_bitmap))
		return true;

	return false;
}

static void yk3_mbox_work_bh(struct work_struct *work)
{
	struct yk3_mbox *mbox = container_of(work, struct yk3_mbox, work_bh);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_msg msg = {0};
	u16 slot;
	int chn;
	u32 copied = 0;

	for_each_set_bit(chn, mbox->chn_bitmap, YK3_MBOX_MAX_CHANNEL) {
		mbox->stats.cnt_rx++;
		if (yk3_mbox_hw_recv_msg(mbox, &msg, chn)) { // invalid msg
			mbox->stats.err_invalid_msg++;
			clear_bit(chn, mbox->chn_bitmap);
			yk3_mbox_clear_recv_mailbox(mbox, chn);
			continue;
		}

		if (!yk3_mbox_msg_is_local(pdev_priv, chn, &msg)) { //forward
			do {
				copied = kfifo_in(&mbox->fwd_fifo, &msg, 1);
				if (!copied) {
					yk3_dev_debug("[BH] src_id 0x%x fwd fifo full op %x seq %d",
						      msg.src_id, msg.opcode, msg.seqno);
					usleep_range(5000, 10000);
				}
			} while (!copied);
			queue_work(system_unbound_wq, &mbox->work_fwd);
		} else if (msg.opcode >> YK3_MBOX_OPCODE_MASK_ACK) { //response
			if (msg.src_id >= YK3_MBOX_MAX_CHANNEL) {
				dbgf("[BH] response.src_id 0x%x invalid! opcode %x seq %d",
				     msg.src_id, msg.opcode, msg.seqno);
				mbox->stats.err_invalid_msg++;
			} else {
				kfifo_in(&mbox->resp_fifo, &msg, 1);
				queue_work(system_unbound_wq, &mbox->work_resp);
			}
		} else { // request
			if (msg_must_drop(mbox, msg.opcode)) {
				dbgf("[BH] request opcode %x dropped, no callback registered",
				     msg.opcode);
				mbox->stats.err_invalid_msg++;
			} else {
				kfifo_in(&mbox->req_fifo, &msg, 1);
				slot = opcode_to_slot(msg.opcode);
				atomic_inc(&mbox->opcode_cb_run_cnt[slot]);
				queue_work(system_unbound_wq, &mbox->work_req);
			}
		}

		yk3_dev_debug("[BH] chn %s%d(0x%x) unlocked",
			      mb_role_str(chn), mb_func_id(chn), chn);
		clear_bit(chn, mbox->chn_bitmap);
		yk3_mbox_clear_recv_mailbox(mbox, chn);
		slot = opcode_to_slot(msg.opcode & GENMASK(14, 0));
		if (msg.opcode >> YK3_MBOX_OPCODE_MASK_ACK)
			atomic_inc(&mbox->opcode_rx_resp_cnt[slot]);
		else
			atomic_inc(&mbox->opcode_rx_req_cnt[slot]);
	}

	if (!bitmap_empty(mbox->chn_bitmap, YK3_MBOX_MAX_CHANNEL))
		queue_work(system_unbound_wq, work);
}

static void yk3_mbox_work_req(struct work_struct *work)
{
	struct yk3_mbox *mbox = container_of(work, struct yk3_mbox, work_req);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_msg msg = {0};
	int copied;

	while (!kfifo_is_empty(&mbox->req_fifo)) {
		copied = kfifo_out(&mbox->req_fifo, &msg, 1);
		yk3_dev_debug("[REQ] kfifo_out %d req from %s%d(0x%x) opcode %x seqno %d\n",
			      copied, mb_role_str(msg.src_id), mb_func_id(msg.src_id), msg.src_id,
			      msg.opcode, msg.seqno);
		request_deal(mbox, &msg);
	}
}

static void yk3_mbox_work_resp(struct work_struct *work)
{
	struct yk3_mbox *mbox = container_of(work, struct yk3_mbox, work_resp);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_msg msg = {0};
	int copied;

	while (!kfifo_is_empty(&mbox->resp_fifo)) {
		copied = kfifo_out(&mbox->resp_fifo, &msg, 1);
		yk3_dev_debug("[RESP] kfifo_out %d resp from %s%d(0x%x) opcode %x seqno %d\n",
			      copied, mb_role_str(msg.src_id), mb_func_id(msg.src_id), msg.src_id,
			      msg.opcode, msg.seqno);
		response_deal(mbox, &msg);
	}
}

static void yk3_mbox_work_fwd(struct work_struct *work)
{
	struct yk3_mbox *mbox = container_of(work, struct yk3_mbox, work_fwd);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_id *dst_id;
	struct yk3_mbox_id *src_id;
	struct yk3_mbox_msg msg = {0};
	int copied;

	while (!kfifo_is_empty(&mbox->fwd_fifo)) {
		copied = kfifo_out(&mbox->fwd_fifo, &msg, 1);
		dst_id = (struct yk3_mbox_id *)&msg.dst_id;
		src_id = (struct yk3_mbox_id *)&msg.src_id;

		if (mbox->role == MB_PF) {
			if (src_id->type == MB_VF_TYPE) {
				//PF收到从VF来的消息，转发至Master
				//记录自己的PF_ID，接收者回复时以此为准
				src_id->gateway = pdev_priv->pf_id & 0x3F;
			} else { //MB_PF_OR_EMP_TYPE
				//PF收到从从Master/EMP来的消息，转发至VF
				//到达终点，清除gateway信息
				dst_id->gateway = 0;
			}
		} else if (mbox->role == MB_MASTER) {
			// Message form emp need update src id
			if (msg.src_id == YK3_MBOX_SRC_ID_NULL)
				msg.src_id = yk3_mbox_emp_id();
		} else {
			//VF和EMP不需要做转发
			yk3_dev_warn("[BH] mbox vf can not forward msg!\n");
			continue;
		}

		yk3_dev_debug("[FWD] mbox forward %d msg(opcode %x) form %s%d(0x%x) to %s%d(0x%x)",
			      copied, msg.opcode,
			      mb_role_str(msg.src_id), mb_func_id(msg.src_id), msg.src_id,
			      mb_role_str(msg.dst_id), mb_func_id(msg.dst_id), msg.dst_id);
		if (yk3_mbox_send_logic(mbox, &msg, MB_NO_REPLY, 0, 0, NULL))
			dbgf("[FWD] mbox forward msg(op %x) form %s%d(0x%x) to %s%d(0x%x) failed",
			     msg.opcode,
			     mb_role_str(msg.src_id), mb_func_id(msg.src_id), msg.src_id,
			     mb_role_str(msg.dst_id), mb_func_id(msg.dst_id), msg.dst_id);
		mbox->stats.cnt_fwd++;
	}
}

static void yk3_mbox_work_atomic(struct work_struct *work)
{
	struct yk3_mbox *mbox = container_of(work, struct yk3_mbox, work_atomic);
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_msg msg = {0};
	int copied;
	struct yk3_mbox_option option = {
		.wait_reply = MB_NO_REPLY,
		.timeout = 0
	};
	int ret;

	yk3_dev_debug("[ATOMIC] enterd");
	while (!kfifo_is_empty(&mbox->atomic_fifo)) {
		copied = kfifo_out(&mbox->atomic_fifo, &msg, 1);
		yk3_dev_debug("[ATOMIC] kfifo_out %d req from %s%d(0x%x) opcode %x seqno %d\n",
			      copied, mb_role_str(msg.src_id), mb_func_id(msg.src_id), msg.src_id,
			      msg.opcode, msg.seqno);
		ret = yk3_mbox_send_msg(pdev_priv, &msg, &option, NULL);
		if (ret) {
			dbgf("%s : %s%d(0x%x) mbox send msg failed, opcode %x seq %d\n",
			     __func__, mb_role_str(msg.dst_id), mb_func_id(msg.dst_id),
			     msg.dst_id, msg.opcode, msg.seqno);
			continue;
		}
		mbox->stats.cnt_atomic++;
	}
}

static irqreturn_t yk3_mbox_handle(int irqn, void *data)
{
	struct yk3_mbox *mbox = data;
	struct yk3_pdev_priv *pdev_priv = pci_get_drvdata(mbox->pdev);
	struct yk3_mbox_irq_info irq_info;
	u32 loop = 0;

	do {
		irq_info = yk3_mbox_get_irq_status(mbox);
		/* only pf need to check irq_status */
		if (!yk3_pdev_is_vf(pdev_priv) && irq_info.irq_status == 0)
			break;

		if (test_and_set_bit(irq_info.msg_id, mbox->chn_bitmap)) {
			dbgf("[IRQ] mbox interrupt lost, chn 0x%0x", irq_info.msg_id);
			mbox->stats.err_irq_lost++;
		}

		/* vf has no pending flag, only deal one msg */
		if (yk3_pdev_is_vf(pdev_priv))
			break;

	} while (++loop < YK3_MBOX_MAX_CHANNEL);

	queue_work(system_unbound_wq, &mbox->work_bh);

	return IRQ_HANDLED;
}

static int yk3_mbox_unregister_irqs(struct pci_dev *pdev)
{
	struct yk3_pdev_priv *pdev_priv = NULL;
	struct yk3_mbox *mbox;
	struct yk3_irq_param param;

	if (!(pdev))
		return -EINVAL;

	pdev_priv = pci_get_drvdata(pdev);
	if (!(pdev_priv))
		return -EINVAL;

	mbox = pdev_priv->mbox;
	if (!mbox) {
		yk3_err("%s, mbox is null\n", __func__);
		return -EINVAL;
	}

	memset(&param, 0, sizeof(param));
	param.vector = YK3_MBOX_IRQ;
	param.handler = yk3_mbox_handle;
	param.data = mbox;
	yk3_irq_free(pdev_priv, &param);

	return 0;
}

static int yk3_mbox_register_irq(struct pci_dev *pdev)
{
	struct yk3_pdev_priv *pdev_priv;
	struct yk3_irq_param param;
	struct yk3_mbox *mbox;
	int ret;

	if (!pdev) {
		yk3_err("%s, pdev is null\n", __func__);
		return -EINVAL;
	}

	pdev_priv = pci_get_drvdata(pdev);
	if (!pdev_priv) {
		yk3_err("%s, pdev_priv is null\n", __func__);
		return -EINVAL;
	}

	mbox = pdev_priv->mbox;
	if (!mbox) {
		yk3_err("%s, mbox is null\n", __func__);
		return -EINVAL;
	}

	memset(&param, 0, sizeof(param));
	snprintf(param.name, sizeof(param.name),
		 "%s(%s)-mbox",
		 pdev_priv->name,
		 pci_name(pdev_priv->pdev));
	param.vector = YK3_MBOX_IRQ;
	param.handler = yk3_mbox_handle;
	param.data = mbox;
	param.flags = YK3_F_IRQ_EXCLUSIVE;
	INIT_WORK(&mbox->work_bh, yk3_mbox_work_bh);
	INIT_WORK(&mbox->work_req, yk3_mbox_work_req);
	INIT_WORK(&mbox->work_resp, yk3_mbox_work_resp);
	INIT_WORK(&mbox->work_fwd, yk3_mbox_work_fwd);
	INIT_WORK(&mbox->work_atomic, yk3_mbox_work_atomic);

	ret = yk3_irq_request(pdev_priv, &param);
	if (ret < 0) {
		yk3_dev_err("Setup mbox irq error: %d", ret);
		return ret;
	}

	return 0;
}

void yk3_mbox_exit(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = NULL;

	mbox = pdev_priv->mbox;
	if (!mbox) {
		yk3_dev_err("%s failed, ret=%d", __func__, -EINVAL);
		return;
	}

	yk3_debug_mbox_exit(pdev_priv);

	yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_TEST);

	yk3_mbox_unregister_irqs(mbox->pdev);

	flush_work(&mbox->work_bh);
	flush_work(&mbox->work_req);
	flush_work(&mbox->work_resp);
	flush_work(&mbox->work_fwd);
	flush_work(&mbox->work_atomic);

	if (yk3_pdev_is_mgr(pdev_priv))
		yk3_wr32(mbox->addr, YK3_MBOX_G_TIMEOUT_ENABLE, 0);

	kfree(mbox->mb_priv);
	kfree(mbox);
	pdev_priv->mbox = NULL;
}

int yk3_mbox_init(struct yk3_pdev_priv *pdev_priv)
{
	struct yk3_mbox *mbox = NULL;
	u32 reg;
	u32 i;
	u32 pf_id;
	u32 retry_count = 0;
	u32 retry_max = 512;
	struct yk3_mbox_priv *yk3_mbox_priv = NULL;
	u16 id = 0;
	struct yk3_mbox_id *mb_id = (struct yk3_mbox_id *)&id;
	int ret;

	if (!yk3_pdev_is_mgr(pdev_priv)) {
		reg = yk3_rd32(pdev_priv->bar_addr[YK3_MBOX_BAR], YK3_MBOX_G_TIMEOUT_ENABLE);
		if (reg == 0 || reg == 0xdeadbeaf) {
			yk3_dev_debug("probe deferred");
			return -EPROBE_DEFER;
		}
	}

	mbox = kzalloc(sizeof(*mbox), GFP_KERNEL);
	if (!mbox)
		return -ENOMEM;

	mbox->pdev = pdev_priv->pdev;
	mbox->addr = (void __iomem *)pdev_priv->bar_addr[YK3_MBOX_BAR];
	/* init mbox base offset */
	yk3_mbox_base = YK3_MBOX_DPU_HOST_BASE;
	//if (dpu_soc) {
	//	yk3_mbox_base = YK3_MBOX_DPU_SOC_BASE;
	//	reg = yk3_rd32(mbox->addr, YK3_MBOX_G_MAILBOX_VERSION);
	//	if (reg == 0xdeadbad5) {
	//		if (pdev_priv->pf_id == 0)
	//			mbox->role = MB_MASTER;
	//		else
	//			mbox->role = MB_PF;
	//		return 0;
	//	}
	//}

	yk3_mbox_priv = kzalloc(sizeof(*yk3_mbox_priv), GFP_KERNEL);
	if (!yk3_mbox_priv) {
		kfree(mbox);
		return -ENOMEM;
	}
	mbox->mb_priv = (void *)yk3_mbox_priv;
	mbox->role = MB_VF;

	for (i = 0; i < YK3_MBOX_MAX_CHANNEL; i++)
		mutex_init(&mbox->chn_mlock[i]);
	for (i = 0; i < YK3_MBOX_WAIT_REPLY_SIZE; i++)
		INIT_HLIST_HEAD(&mbox->wait_reply_hlist[i]);
	mutex_init(&mbox->wait_reply_mlock);
	spin_lock_init(&mbox->atomic_fifo_lock);
	INIT_KFIFO(mbox->atomic_fifo);
	INIT_KFIFO(mbox->req_fifo);
	INIT_KFIFO(mbox->resp_fifo);
	INIT_KFIFO(mbox->fwd_fifo);

	if (yk3_pdev_is_mgr(pdev_priv)) {
		mbox->role = MB_MASTER;
		/* get lf to pf map table */
		for (i = YK3_MBOX_LF_START; i <= YK3_MBOX_LF_END; ++i) {
			reg = yk3_rd32(mbox->addr, YK3_MBOX_LFX_MEM_OFFSET(i));
			pf_id = (u32)FIELD_GET(YK3_MBOX_LFX_MEM_PF_ID, reg);
			yk3_dev_debug("addr:%x", YK3_MBOX_LFX_MEM_OFFSET(i));
			yk3_dev_debug("master lf %d to pf %x, reg:%x", i, pf_id, reg);
			yk3_mbox_priv->pf2lf_table[pf_id] = i;
		}
		/* config pf to pf-master interrupt vector */
		yk3_wr32(mbox->addr, YK3_MBOX_PF2PF_IRQ_VECTOR, YK3_MBOX_IRQ);
		/* config soc master to host pf master interrupt vector */
		yk3_wr32(mbox->addr, YK3_MBOX_S2H_IRQ_VECTOR, YK3_MBOX_IRQ);
		/* config emp master to host pf master interrupt vector*/
		yk3_wr32(mbox->addr, YK3_MBOX_M2H_IRQ_VECTOR, YK3_MBOX_IRQ);

		/* the interrupt timeout was enabled */
		yk3_wr32(mbox->addr, YK3_MBOX_G_TIMEOUT_ENABLE, 0);
		yk3_wr32(mbox->addr, YK3_MBOX_G_TIMEOUT_CNT, YK3_MBOX_G_TIMEOUT);
		yk3_wr32(mbox->addr, YK3_MBOX_G_TIMEOUT_ENABLE, 1);

		/* clearing pf2pf interrupt */
		reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
		retry_count = 0;
		do {
			if (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, reg)) {
				reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
				yk3_dev_debug("read YK3_MBOX_PF2PF_IRQ_PENDING:%08x\n", reg);
			}

			retry_count++;
			if (retry_count > retry_max) {
				yk3_dev_err("mailbox cannot clear interrupt %08x\n", reg);
				break;
			}
		} while (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, reg));
	} else if (yk3_pdev_is_pf(pdev_priv)) {
		mbox->role = MB_PF;
		/* config pf to pf-master interrupt vector */
		yk3_wr32(mbox->addr, YK3_MBOX_PF2PF_IRQ_VECTOR, YK3_MBOX_IRQ);

		/* clearing pf2pf interrupt */
		reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
		retry_count = 0;
		do {
			if (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, reg)) {
				reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2PF_IRQ_PENDING);
				yk3_dev_debug("read YK3_MBOX_PF2PF_IRQ_PENDING:%08x\n", reg);
			}

			retry_count++;
			if (retry_count > retry_max) {
				yk3_dev_err("mailbox cannot clear interrupt %08x\n", reg);
				break;
			}
		} while (FIELD_GET(YK3_MBOX_PF2PF_IRQ_P_STATUS, reg));
		/* config pf2vf interrupt vector */
		yk3_wr32(mbox->addr, YK3_MBOX_PF2VF_IRQ_VECTOR, YK3_MBOX_IRQ);

		/* clearing pf2vf interrupt */
		reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2VF_IRQ_PENDING);
		retry_count = 0;
		do {
			if (FIELD_GET(YK3_MBOX_PF2VF_IRQ_P_STATUS, reg)) {
				reg = yk3_rd32(mbox->addr, YK3_MBOX_PF2VF_IRQ_PENDING);
				yk3_dev_debug("read YK3_MBOX_PF2VF_IRQ_PENDING:%08x\n", reg);
			}

			retry_count++;
			if (retry_count > retry_max) {
				yk3_dev_err("mailbox cannot clear interrupt %08x\n", reg);
				break;
			}
		} while (FIELD_GET(YK3_MBOX_PF2VF_IRQ_P_STATUS, reg));
	} else if (yk3_pdev_is_vf(pdev_priv)) {
		/* config vf2pf interrupt vector */
		yk3_wr32(mbox->addr, YK3_MBOX_VF_IRQ_VECTOR, YK3_MBOX_IRQ);

		/* clearing vf2pf interrupt */
		reg = yk3_rd32(mbox->addr, YK3_MBOX_VF_IRQ_PENDING);
		retry_count = 0;
		do {
			if (FIELD_GET(YK3_MBOX_VF2PF_IRQ_P_STATUS, reg)) {
				reg = yk3_rd32(mbox->addr, YK3_MBOX_VF_IRQ_PENDING);
				yk3_dev_debug("read YK3_MBOX_VF_IRQ_PENDING:%08x\n", reg);
			}

			retry_count++;
			if (retry_count > retry_max) {
				yk3_dev_err("mailbox cannot clear interrupt %08x\n", reg);
				break;
			}
		} while (FIELD_GET(YK3_MBOX_VF2PF_IRQ_P_STATUS, reg));
	}
	/* clear mbox shmem */
	if (!yk3_pdev_is_vf(pdev_priv)) {
		mb_id->type = MB_PF_OR_EMP_TYPE;
		if (mbox->role == MB_MASTER) {
			/* clear send to PF mbox */
			for (i = 0; i < YK3_MBOX_LF_END; i++) {
				/* do not clear master share mem */
				if (i == pdev_priv->pf_id)
					continue;
				mb_id->func_id = i;
				yk3_mbox_clear_send_mailbox(mbox, id);
				yk3_mbox_clear_recv_mailbox(mbox, id);
			}
			/* clear SOC-HOST mbox */
			mb_id->func_id = YK3_MBOX_MASTER_PF_ID;
			yk3_mbox_clear_send_mailbox(mbox, id);
			yk3_mbox_clear_recv_mailbox(mbox, id);
			/* clear EMP mbox */
			mb_id->func_id = YK3_MBOX_EMP_ID;
			yk3_mbox_clear_send_mailbox(mbox, id);
			yk3_mbox_clear_recv_mailbox(mbox, id);
		} else {
			/* clear send to master mbox */
			mb_id->func_id = 0;
			yk3_mbox_clear_send_mailbox(mbox, id);
			yk3_mbox_clear_recv_mailbox(mbox, id);
		}
	} else {
		yk3_mbox_clear_send_mailbox(mbox, 0);
		yk3_mbox_clear_recv_mailbox(mbox, 0);
	}

	pdev_priv->mbox = mbox;
	ret = yk3_mbox_register_irq(pdev_priv->pdev);
	if (ret) {
		yk3_dev_err("mbox failed to register irq\n");
		goto err_free;
	}

	ret = yk3_debug_mbox_init(pdev_priv);
	if (ret) {
		yk3_dev_err("failed to init mbox debug\n");
		goto err_free;
	}

	return 0;

err_free:
	kfree(mbox->mb_priv);
	if (yk3_pdev_is_mgr(pdev_priv))
		yk3_wr32(mbox->addr, YK3_MBOX_G_TIMEOUT_ENABLE, 0);
	kfree(mbox);
	pdev_priv->mbox = NULL;

	return ret;
}

int yk3_mbox_register_callback(struct yk3_pdev_priv *pdev_priv,
			       u16 opcode,
			       void (*callback)(struct yk3_mbox_msg *msg, void *param),
			       void *param)
{
	u16 slot;

	if (!pdev_priv->mbox) {
		yk3_dev_err("%s failed, pdev_priv->mbox is null\n", __func__);
		return -EINVAL;
	}

	if (!is_opcode_valid(opcode)) {
		yk3_dev_err("%s failed, opcode 0x%0x is out of range\n", __func__, opcode);
		return -EINVAL;
	}

	slot = opcode_to_slot(opcode);

	if (pdev_priv->mbox->opcode_cb[slot]) {
		yk3_dev_err("%s failed, callback function already registered for opcode %x\n",
			    __func__, opcode);
		return -EINVAL;
	}

	pdev_priv->mbox->opcode_cb[slot] = callback;
	pdev_priv->mbox->opcode_cb_param[slot] = param;

	return 0;
}

void yk3_mbox_unregister_callback(struct yk3_pdev_priv *pdev_priv, u16 opcode)
{
	u16 slot;
	int run_cnt = 0;
	unsigned long timeout;

	if (!is_opcode_valid(opcode)) {
		yk3_dev_err("%s failed, opcode 0x%0x is out of range\n", __func__, opcode);
		return;
	}

	slot = opcode_to_slot(opcode);

	set_bit(slot, pdev_priv->mbox->opcode_cb_stopping_bitmap);
	run_cnt = atomic_read(&pdev_priv->mbox->opcode_cb_run_cnt[slot]);
	timeout = jiffies + msecs_to_jiffies(2 * YK3_MBOX_REPLY_TIMEOUT);
	while (run_cnt) {
		if (time_after(jiffies, timeout)) {
			yk3_dev_err("%s waits for opcode %x more than %d ms",
				    __func__, slot_to_opcode(slot), 2 * YK3_MBOX_REPLY_TIMEOUT);
			break;
		}
		usleep_range(1000, 2000);
		run_cnt = atomic_read(&pdev_priv->mbox->opcode_cb_run_cnt[slot]);
	}

	pdev_priv->mbox->opcode_cb[slot] = NULL;
	pdev_priv->mbox->opcode_cb_param[slot] = NULL;
}
