/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#ifndef __UNIC_STATS_H__
#define __UNIC_STATS_H__

#include <linux/ethtool.h>
#include <linux/netdevice.h>
#include <ub/ubase/ubase_comm_cmd.h>
#include <ub/ubase/ubase_comm_ctrlq.h>

#define UNIC_FEC_CORR_BLOCKS	BIT(0)
#define UNIC_FEC_UNCORR_BLOCKS	BIT(1)
#define UNIC_FEC_CORR_BITS	BIT(2)

#define UNIC_TX_CMDQ_DEPTH		UBASE_CSQ_DEPTH_REG
#define UNIC_TX_CMDQ_TAIL		UBASE_CSQ_TAIL_REG
#define UNIC_TX_CMDQ_HEAD		UBASE_CSQ_HEAD_REG
#define UNIC_RX_CMDQ_DEPTH		UBASE_CRQ_DEPTH_REG
#define UNIC_RX_CMDQ_TAIL		UBASE_CRQ_TAIL_REG
#define UNIC_RX_CMDQ_HEAD		UBASE_CRQ_HEAD_REG
#define UNIC_CMDQ_INT_GEN		0x18000
#define UNIC_CMDQ_INT_SCR		0x18004
#define UNIC_CMDQ_INT_MASK		0x18008
#define UNIC_CMDQ_INT_STS		0x1800c

#define UNIC_TX_CTRLQ_DEPTH		UBASE_CTRLQ_CSQ_DEPTH_REG
#define UNIC_TX_CTRLQ_TAIL		UBASE_CTRLQ_CSQ_TAIL_REG
#define UNIC_TX_CTRLQ_HEAD		UBASE_CTRLQ_CSQ_HEAD_REG
#define UNIC_RX_CTRLQ_DEPTH		UBASE_CTRLQ_CRQ_DEPTH_REG
#define UNIC_RX_CTRLQ_TAIL		UBASE_CTRLQ_CRQ_TAIL_REG
#define UNIC_RX_CTRLQ_HEAD		UBASE_CTRLQ_CRQ_HEAD_REG
#define UNIC_CTRLQ_INT_GEN		0x18010
#define UNIC_CTRLQ_INT_SCR		0x18014
#define UNIC_CTRLQ_INT_MASK		0x18018
#define UNIC_CTRLQ_INT_STS		0x1801c

enum unic_reg_num_idx {
	UNIC_REG_NUM_IDX_DL = 0,
	UNIC_REG_NUM_IDX_NL,
	UNIC_REG_NUM_IDX_BA,
	UNIC_REG_NUM_IDX_TP,
	UNIC_REG_NUM_IDX_TA,
	UNIC_REG_NUM_IDX_MAX,
};

enum unic_reg_tag {
	UNIC_TAG_CMDQ = 0,
	UNIC_TAG_CTRLQ,
	UNIC_TAG_DL,
	UNIC_TAG_NL,
	UNIC_TAG_BA,
	UNIC_TAG_TP,
	UNIC_TAG_TA,
	UNIC_TAG_MAX,
};

struct unic_res_regs_group {
	u16 tag;
	u32 *regs_addr;
	u32 regs_count;
	bool (*is_supported)(struct auxiliary_device *adev);
};

struct unic_dump_reg_hdr {
	u8 flag;
	u8 rsv[3];
};

struct unic_tlv_hdr {
	u16 tag;
	u16 len;
};

struct unic_dfx_regs_group {
	u16 regs_idx;
	u16 tag;
	u16 opcode;
	u32 property;
	bool (*is_supported)(struct unic_dev *unic_dev, u32 property);
};

int unic_get_regs_len(struct net_device *netdev);
void unic_get_regs(struct net_device *netdev, struct ethtool_regs *cmd,
		   void *data);
void unic_get_fec_stats(struct net_device *ndev,
			struct ethtool_fec_stats *fec_stats);

#endif /* __UNIC_STATS_H__ */
