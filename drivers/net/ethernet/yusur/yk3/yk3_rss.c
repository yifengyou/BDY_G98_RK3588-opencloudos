// SPDX-License-Identifier: GPL-2.0
#include "yk3.h"
#include "yk3_rss_priv.h"

#define YK3_RSS_INDIR_MAX_IDX_NUM	(4 * 64)
#define YK3_RSS_INDIR_Q_CHUNK_SIZE	16

static u8 yk3_default_hash_key[] = {0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e,
	0xc2, 0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3,
	0x8f, 0xb0, 0xd0, 0xca, 0x2b, 0xcb, 0xae,
	0x7b, 0x30, 0xb4, 0x77, 0xcb, 0x2d, 0xa3,
	0x80, 0x30, 0xf2, 0x0c, 0x6a, 0x42, 0xb7,
	0x3b, 0xbe, 0xac, 0x01, 0xfa
};

static void yk3_pf_rss_indir_table_default(void __iomem *hw_addr, u16 qstart, u16 qnb)
{
	u32 i = 0, tmp = 0, val = 0;

	for (i = 0; i < 4 * qnb; i++) {
		if (tmp == qnb)
			tmp = 0;
		val = val + (tmp << (8 * (i & 0x03)));
		tmp++;
		if ((i & 0x03) == 0x03) {
			yk3_wr32(hw_addr, YK3_RSS_INDIRECT_BASE + 4 * qstart + 4 * (i >> 2), val);
			val = 0;
		}
	}
}

static void yk3_pf_rss_indir_table_set(void __iomem *hw_addr, u16 qstart, u16 qnb, u8 *data)
{
	u32 i = 0, val = 0;

	for (i = 0; i < 4 * qnb; i += 4) {
		val = ((data[i + 3] & 0xff) << 24) | ((data[i + 2] & 0xff) << 16)
			| ((data[i + 1] & 0xff) << 8) | (data[i] & 0xff);
		yk3_wr32(hw_addr, YK3_RSS_INDIRECT_BASE + 4 * qstart + i, val);
	}
}

static void yk3_pf_rss_indir_table_get(void __iomem *hw_addr, u16 qstart, u16 qnb, u8 *out)
{
	u32 i = 0, j = 0, val = 0;

	for (i = 0; i < qnb; i++) {
		val = yk3_rd32(hw_addr, (u32)(YK3_RSS_INDIRECT_BASE + 4 * qstart + 4 * i));
		out[j++] = val & 0xff;
		out[j++] = (val >> 8) & 0xff;
		out[j++] = (val >> 16) & 0xff;
		out[j++] = (val >> 24) & 0xff;
	}
}

static void yk3_pf_hash_key_set(void __iomem *hw_addr, const u8 *key)
{
	u32 i, val;

	for (i = 0; i <= YK3_RSS_HASH_KEY_SIZE - 4; i += 4) {
		val = *((u32 *)(key + i));
		yk3_wr32(hw_addr, YK3_RSS_KEY_ADDR + YK3_RSS_HASH_KEY_SIZE - 4 - i,
			 (__force u32)htonl(val));
	}
}

static void yk3_pf_hash_key_get(void __iomem *hw_addr, u8 *out)
{
	u32 i, val;

	for (i = 0; i <= YK3_RSS_HASH_KEY_SIZE - 4; i += 4) {
		val = yk3_rd32(hw_addr, (u32)(YK3_RSS_KEY_ADDR + YK3_RSS_HASH_KEY_SIZE - 4 - i));
		*((u32 *)(out + i)) = (__force u32)htonl(val);
	}
}

void yk3_rss_indir_table_set(struct net_device *ndev, const u32 *indir, u16 qcount)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_rss_indir_cmd *cmd;
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u32 i = 0, val = 0;
	u16 qstart = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	u16 q_done = 0, q_left = 0;

	if (yk3_ndev_is_vf(ndev_priv)) {
		mbox_msg.opcode = YK3_MBOX_OPCODE_RSS_INDIRECT;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		cmd = (struct yk3_mbox_rss_indir_cmd *)mbox_msg.data;
		for (q_done = 0; q_done < qcount;) {
			memset(mbox_msg.data, 0, YK3_MBOX_DATA_LEN);
			cmd->cmd_type = YK3_CMD_RSS_INDIRECT_TABLE_SET;
			q_left = qcount - q_done;
			cmd->qnb = min_t(u16, q_left, YK3_RSS_INDIR_Q_CHUNK_SIZE);
			cmd->qstart = qstart + q_done;
			for (i = 0; i < 4 * cmd->qnb; i++)
				cmd->cmd_data[i] = (u8)(indir[4 * q_done + i] & 0xff);
			if (yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL))
				return;
			q_done += cmd->qnb;
		}
	} else {
		for (i = 0; i < 4 * qcount; i += 4) {
			val = ((indir[i + 3] & 0xff) << 24) | ((indir[i + 2] & 0xff) << 16)
				| ((indir[i + 1] & 0xff) << 8) | (indir[i] & 0xff);
			yk3_wr32(hw_addr, YK3_RSS_INDIRECT_BASE + 4 * qstart + i, val);
		}
	}
}

void yk3_rss_indir_table_get(struct net_device *ndev, u32 *indir)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0}, ack_msg = {0};
	struct yk3_mbox_rss_indir_cmd *cmd, *ack_cmd;
	struct yk3_mbox_option mbox_opt = {0};
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u32 i = 0, j = 0, val = 0;
	u16 q_done = 0, q_left = 0;
	u16 qstart = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	u32 qcount = ndev->real_num_rx_queues;

	if (yk3_ndev_is_vf(ndev_priv)) {
		mbox_msg.opcode = YK3_MBOX_OPCODE_RSS_INDIRECT;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		cmd = (struct yk3_mbox_rss_indir_cmd *)mbox_msg.data;
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 1000;
		for (q_done = 0; q_done < qcount;) {
			memset(mbox_msg.data, 0, YK3_MBOX_DATA_LEN);
			cmd->cmd_type = YK3_CMD_RSS_INDIRECT_TABLE_GET;
			q_left = qcount - q_done;
			cmd->qnb = min_t(u16, q_left, YK3_RSS_INDIR_Q_CHUNK_SIZE);
			cmd->qstart = qstart + q_done;
			if (yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg))
				return;
			ack_cmd = (struct yk3_mbox_rss_indir_cmd *)ack_msg.data;
			for (i = 0; i < 4 * cmd->qnb; i++)
				indir[4 * q_done + i] = ack_cmd->cmd_data[i];
			q_done += cmd->qnb;
			memset(&ack_msg, 0, sizeof(ack_msg));
		}
	} else {
		for (i = 0; i < qcount; i++) {
			val = yk3_rd32(hw_addr, YK3_RSS_INDIRECT_BASE + 4 * qstart + 4 * i);
			indir[j++] = val & 0xff;
			indir[j++] = (val >> 8) & 0xff;
			indir[j++] = (val >> 16) & 0xff;
			indir[j++] = (val >> 24) & 0xff;
		}
	}
}

void yk3_rss_key_set(struct net_device *ndev, const u8 *key)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_rss_indir_cmd *cmd;
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u16 qstart = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	u32 qcount = ndev->real_num_rx_queues;

	if (yk3_ndev_is_vf(ndev_priv)) {
		mbox_msg.opcode = YK3_MBOX_OPCODE_RSS_INDIRECT;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		cmd = (struct yk3_mbox_rss_indir_cmd *)mbox_msg.data;
		cmd->cmd_type = YK3_CMD_RSS_INDIRECT_KEY_SET;
		cmd->qstart = qstart;
		cmd->qnb = (u16)(qcount);
		memcpy(cmd->cmd_data, key, YK3_RSS_HASH_KEY_SIZE);
		yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	} else {
		yk3_pf_hash_key_set(hw_addr, key);
	}
}

void yk3_rss_key_get(struct net_device *ndev, u8 *key)
{
	struct yk3_ndev_priv *ndev_priv = netdev_priv(ndev);
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0}, ack_msg = {0};
	struct yk3_mbox_rss_indir_cmd *cmd, *ack_cmd;
	struct yk3_mbox_option mbox_opt = {0};
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u16 qstart = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;
	u32 qcount = ndev->real_num_rx_queues;

	if (yk3_ndev_is_vf(ndev_priv)) {
		mbox_msg.opcode = YK3_MBOX_OPCODE_RSS_INDIRECT;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		cmd = (struct yk3_mbox_rss_indir_cmd *)mbox_msg.data;
		cmd->cmd_type = YK3_CMD_RSS_INDIRECT_KEY_GET;
		cmd->qstart = qstart;
		cmd->qnb = (u16)(qcount);
		mbox_opt.wait_reply = MB_WAIT_REPLY;
		mbox_opt.timeout = 1000;
		if (!yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, &ack_msg)) {
			ack_cmd = (struct yk3_mbox_rss_indir_cmd *)ack_msg.data;
			memcpy(key, ack_cmd->cmd_data, YK3_RSS_HASH_KEY_SIZE);
		}
	} else {
		yk3_pf_hash_key_get(hw_addr, key);
	}
}

int yk3_rss_indir_table_init(struct yk3_ndev_priv *ndev_priv, u16 rxqnum)
{
	struct yk3_pdev_priv *pdev_priv = ndev_priv->pdev_priv;
	struct yk3_mbox_msg mbox_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	struct yk3_mbox_rss_indir_cmd *cmd;
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u16 qstart = ndev_priv->qbase[YK3_QUEUE_T_GLOBAL].start;

	if (yk3_ndev_is_vf(ndev_priv)) {
		/*request pf set vf hash*/
		mbox_msg.opcode = YK3_MBOX_OPCODE_RSS_INDIRECT;
		mbox_msg.dst_id = yk3_mbox_pf_id(0);
		cmd = (struct yk3_mbox_rss_indir_cmd *)mbox_msg.data;
		cmd->cmd_type = YK3_CMD_RSS_INDIRECT_TABLE_INIT;
		cmd->qstart = qstart;
		cmd->qnb = rxqnum;
		return yk3_mbox_send_msg(pdev_priv, &mbox_msg, &mbox_opt, NULL);
	}

	yk3_pf_rss_indir_table_default(hw_addr, qstart, rxqnum);
	return 0;
}

static void yk3_mbox_rss_indir_cb(struct yk3_mbox_msg *msg, void *param)
{
	struct yk3_pdev_priv *pdev_priv = param;
	struct yk3_mbox_rss_indir_cmd *cmd, *ack_cmd;
	struct yk3_mbox_msg ack_msg = {0};
	struct yk3_mbox_option mbox_opt = {0};
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;

	cmd = (struct yk3_mbox_rss_indir_cmd *)msg->data;
	ack_msg.opcode = msg->opcode | (1 << YK3_MBOX_OPCODE_MASK_ACK);
	ack_msg.seqno = msg->seqno;
	ack_msg.dst_id = msg->src_id;
	ack_cmd = (struct yk3_mbox_rss_indir_cmd *)ack_msg.data;

	switch (cmd->cmd_type) {
	case YK3_CMD_RSS_INDIRECT_TABLE_INIT:
		yk3_pf_rss_indir_table_default(hw_addr, cmd->qstart, cmd->qnb);
		break;
	case YK3_CMD_RSS_INDIRECT_TABLE_GET:
		yk3_pf_rss_indir_table_get(hw_addr, cmd->qstart, cmd->qnb, ack_cmd->cmd_data);
		ack_cmd->cmd_status = 0;
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &mbox_opt, NULL);
		break;
	case YK3_CMD_RSS_INDIRECT_TABLE_SET:
		yk3_pf_rss_indir_table_set(hw_addr, cmd->qstart, cmd->qnb, cmd->cmd_data);
		break;
	case YK3_CMD_RSS_INDIRECT_KEY_SET:
		yk3_pf_hash_key_set(hw_addr, cmd->cmd_data);
		break;
	case YK3_CMD_RSS_INDIRECT_KEY_GET:
		ack_cmd->cmd_status = 0;
		yk3_pf_hash_key_get(hw_addr, ack_cmd->cmd_data);
		yk3_mbox_send_msg(pdev_priv, &ack_msg, &mbox_opt, NULL);
		break;
	default:
		break;
	}
}

u32 yk3_rss_key_size(void)
{
	return YK3_RSS_HASH_KEY_SIZE;
}

int yk3_rss_init(struct yk3_pdev_priv *pdev_priv)
{
	void __iomem *hw_addr = pdev_priv->bar_addr[0] + YK3_RSS_BAR_BASE;
	u32 val, i;
	int ret = 0;

	if (yk3_pdev_is_pf(pdev_priv)) {
		ret = yk3_mbox_register_callback(pdev_priv, YK3_MBOX_OPCODE_RSS_INDIRECT,
						 yk3_mbox_rss_indir_cb, pdev_priv);
		if (ret) {
			yk3_dev_err("register rss mbox callback failed");
			return ret;
		}
	}

	if (!yk3_pdev_is_mgr(pdev_priv))
		return 0;

	/* set rss default key */
	for (i = 0; i <= YK3_RSS_HASH_KEY_SIZE - 4; i += 4) {
		val = *((u32 *)(yk3_default_hash_key + i));
		yk3_wr32(hw_addr, YK3_RSS_KEY_ADDR + YK3_RSS_HASH_KEY_SIZE - 4 - i,
			 (__force u32)htonl(val));
	}

	/* set rss indirect scale & bias & sw_fr */
	val = yk3_rd32(hw_addr, YK3_RSS_INDIRECT_SCALE_BIAS_ADDR);
	val &= ~(YK3_RSS_INDIRECT_SCALE | YK3_RSS_INDIRECT_BIAS
		| YK3_RSS_INDIRECT_SW_FR);
	val |= FIELD_PREP(YK3_RSS_INDIRECT_SCALE,
			  YK3_RSS_INDIRECT_SCALE_POWER_VALUE);
	val |= FIELD_PREP(YK3_RSS_INDIRECT_BIAS, YK3_RSS_INDIRECT_BIAS_VALUE);
	val |= FIELD_PREP(YK3_RSS_INDIRECT_SW_FR, YK3_RSS_INDIRECT_SW_FR_VALUE);
	yk3_wr32(hw_addr, YK3_RSS_INDIRECT_SCALE_BIAS_ADDR, val);
	return 0;
}

void yk3_rss_exit(struct yk3_pdev_priv *pdev_priv)
{
	if (yk3_pdev_is_pf(pdev_priv))
		yk3_mbox_unregister_callback(pdev_priv, YK3_MBOX_OPCODE_RSS_INDIRECT);
}

int yk3_rss_ndev_init(struct yk3_ndev_priv *ndev_priv)
{
	struct net_device *ndev = ndev_priv->ndev;

	return yk3_rss_indir_table_init(ndev_priv, (u16)ndev->real_num_rx_queues);
}

void yk3_rss_ndev_exit(struct yk3_ndev_priv *ndev_priv)
{
}

