// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include <linux/phy.h>
#include <ub/ubase/ubase_comm_cmd.h>
#include <ub/ubase/ubase_comm_stats.h>

#include "unic.h"
#include "unic_dev.h"
#include "unic_hw.h"
#include "unic_netdev.h"
#include "unic_stats.h"

static u32 cmdq_regs_addr[] = {
	UNIC_TX_CMDQ_DEPTH,
	UNIC_TX_CMDQ_TAIL,
	UNIC_TX_CMDQ_HEAD,
	UNIC_RX_CMDQ_DEPTH,
	UNIC_RX_CMDQ_TAIL,
	UNIC_RX_CMDQ_HEAD,
	UNIC_CMDQ_INT_GEN,
	UNIC_CMDQ_INT_SCR,
	UNIC_CMDQ_INT_MASK,
	UNIC_CMDQ_INT_STS,
};

static u32 ctrlq_regs_addr[] = {
	UNIC_TX_CTRLQ_DEPTH,
	UNIC_TX_CTRLQ_TAIL,
	UNIC_TX_CTRLQ_HEAD,
	UNIC_RX_CTRLQ_DEPTH,
	UNIC_RX_CTRLQ_TAIL,
	UNIC_RX_CTRLQ_HEAD,
	UNIC_CTRLQ_INT_GEN,
	UNIC_CTRLQ_INT_SCR,
	UNIC_CTRLQ_INT_MASK,
	UNIC_CTRLQ_INT_STS,
};

static const struct unic_res_regs_group unic_res_reg_arr[] = {
	{
		UNIC_TAG_CMDQ, cmdq_regs_addr, ARRAY_SIZE(cmdq_regs_addr),
		NULL
	}, {
		UNIC_TAG_CTRLQ, ctrlq_regs_addr, ARRAY_SIZE(ctrlq_regs_addr),
		ubase_adev_ctrlq_supported
	},
};

static bool unic_dfx_reg_support(struct unic_dev *unic_dev, u32 property)
{
	if (((property & UBASE_SUP_UBL) && unic_dev_ubl_supported(unic_dev)) ||
	    ((property & UBASE_SUP_ETH) && unic_dev_eth_mac_supported(unic_dev)))
		return true;

	return false;
}

static struct unic_dfx_regs_group unic_dfx_reg_arr[] = {
	{
		UNIC_REG_NUM_IDX_TA, UNIC_TAG_TA, UBASE_OPC_DFX_TA_REG,
		UBASE_SUP_UBL_ETH, unic_dfx_reg_support
	}, {
		UNIC_REG_NUM_IDX_TP, UNIC_TAG_TP, UBASE_OPC_DFX_TP_REG,
		UBASE_SUP_UBL_ETH, unic_dfx_reg_support
	}, {
		UNIC_REG_NUM_IDX_BA, UNIC_TAG_BA, UBASE_OPC_DFX_BA_REG,
		UBASE_SUP_UBL_ETH, unic_dfx_reg_support
	}, {
		UNIC_REG_NUM_IDX_NL, UNIC_TAG_NL, UBASE_OPC_DFX_NL_REG,
		UBASE_SUP_UBL_ETH, unic_dfx_reg_support
	}, {
		UNIC_REG_NUM_IDX_DL, UNIC_TAG_DL, UBASE_OPC_DFX_DL_REG,
		UBASE_SUP_UBL, unic_dfx_reg_support
	},
};

static int unic_get_dfx_reg_num(struct unic_dev *unic_dev, u32 *reg_num,
				u32 reg_arr_size)
{
	struct ubase_cmd_buf in, out;
	int ret;

	ubase_fill_inout_buf(&in, UBASE_OPC_DFX_REG_NUM, true, 0, NULL);
	ubase_fill_inout_buf(&out, UBASE_OPC_DFX_REG_NUM, true,
			     reg_arr_size * sizeof(u32), reg_num);
	ret = ubase_cmd_send_inout(unic_dev->comdev.adev, &in, &out);
	if (ret && ret != -EPERM)
		unic_err(unic_dev,
			 "failed to query dfx reg num, ret = %d.\n", ret);

	return ret;
}

static int unic_get_res_regs_len(struct unic_dev *unic_dev,
				 const struct unic_res_regs_group *reg_arr,
				 u32 reg_arr_size)
{
	u32 i, count = 0;

	for (i = 0; i < reg_arr_size; i++) {
		if (reg_arr[i].is_supported &&
		    !reg_arr[i].is_supported(unic_dev->comdev.adev))
			continue;

		count += reg_arr[i].regs_count * sizeof(u32) +
			 sizeof(struct unic_tlv_hdr);
	}

	return count;
}

static int unic_get_dfx_regs_len(struct unic_dev *unic_dev,
				 struct unic_dfx_regs_group *reg_arr,
				 u32 reg_arr_size, u32 *reg_num)
{
	u32 i, count = 0;

	for (i = 0; i < reg_arr_size; i++) {
		if (!reg_arr[i].is_supported(unic_dev, reg_arr[i].property))
			continue;

		count += sizeof(struct unic_tlv_hdr) + sizeof(u32) *
			 reg_num[reg_arr[i].regs_idx];
	}

	return count;
}

int unic_get_regs_len(struct net_device *netdev)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	u32 reg_arr_size;
	int count = 0;
	u32 *reg_num;
	int ret;

	if (unic_resetting(netdev))
		return -EBUSY;

	count += unic_get_res_regs_len(unic_dev, unic_res_reg_arr,
				       ARRAY_SIZE(unic_res_reg_arr));
	reg_arr_size = ARRAY_SIZE(unic_dfx_reg_arr);
	reg_num = kcalloc(reg_arr_size, sizeof(u32), GFP_KERNEL);
	if (!reg_num)
		return -ENOMEM;

	ret = unic_get_dfx_reg_num(unic_dev, reg_num, reg_arr_size);
	if (!ret) {
		count += unic_get_dfx_regs_len(unic_dev, unic_dfx_reg_arr,
					       reg_arr_size, reg_num);
	} else if (ret != -EPERM) {
		unic_err(unic_dev,
			 "failed to get dfx regs length, ret = %d.\n", ret);
		kfree(reg_num);

		return -EBUSY;
	}

	kfree(reg_num);

	return count;
}

static u16 unic_fetch_res_regs(struct unic_dev *unic_dev, u8 *data, u16 tag,
			       u32 *regs_addr_arr, u32 reg_num)
{
	struct unic_tlv_hdr *tlv = (struct unic_tlv_hdr *)data;
	u32 *reg = (u32 *)(data + sizeof(struct unic_tlv_hdr));
	u32 i;

	tlv->tag = tag;
	tlv->len = sizeof(struct unic_tlv_hdr) + reg_num * sizeof(u32);

	for (i = 0; i < reg_num; i++)
		*reg++ = unic_read_reg(unic_dev, regs_addr_arr[i]);

	return tlv->len;
}

static u32 unic_get_res_regs(struct unic_dev *unic_dev, u8 *data)
{
	u32 i, data_len = 0;

	for (i = 0; i < ARRAY_SIZE(unic_res_reg_arr); i++) {
		if (unic_res_reg_arr[i].is_supported &&
		    !unic_res_reg_arr[i].is_supported(unic_dev->comdev.adev))
			continue;

		data_len += unic_fetch_res_regs(unic_dev, data + data_len,
						unic_res_reg_arr[i].tag,
						unic_res_reg_arr[i].regs_addr,
						unic_res_reg_arr[i].regs_count);
	}

	return data_len;
}

static int unic_query_regs_data(struct unic_dev *unic_dev, u8 *data,
				u32 reg_num, u16 opcode)
{
	u32 *reg = (u32 *)(data + sizeof(struct unic_tlv_hdr));
	struct ubase_cmd_buf in, out;
	u32 *out_regs;
	int ret;
	u32 i;

	out_regs = kcalloc(reg_num, sizeof(u32), GFP_KERNEL);
	if (!out_regs)
		return -ENOMEM;

	ubase_fill_inout_buf(&in, opcode, true, 0, NULL);
	ubase_fill_inout_buf(&out, opcode, true, reg_num * sizeof(u32),
			     out_regs);
	ret = ubase_cmd_send_inout(unic_dev->comdev.adev, &in, &out);
	if (ret) {
		unic_err(unic_dev,
			 "failed to send getting reg cmd(0x%x), ret = %d.\n",
			 opcode, ret);
		goto err_send_cmd;
	}

	for (i = 0; i < reg_num; i++)
		*reg++ = le32_to_cpu(*(out_regs + i));

err_send_cmd:
	kfree(out_regs);

	return ret;
}

static int unic_get_dfx_regs(struct unic_dev *unic_dev, u8 *data,
			     struct unic_dfx_regs_group *reg_arr,
			     u32 reg_arr_size, u32 *reg_num)
{
	struct unic_tlv_hdr *tlv;
	u16 idx;
	int ret;
	u32 i;

	for (i = 0; i < reg_arr_size; i++) {
		if (!reg_arr[i].is_supported(unic_dev, reg_arr[i].property))
			continue;

		idx = reg_arr[i].regs_idx;
		ret = unic_query_regs_data(unic_dev, data, reg_num[idx],
					   reg_arr[i].opcode);
		if (ret) {
			unic_err(unic_dev,
				 "failed to query dfx regs, ret = %d.\n", ret);
			return ret;
		}

		tlv = (struct unic_tlv_hdr *)data;
		tlv->tag = reg_arr[i].tag;
		tlv->len = sizeof(*tlv) + reg_num[idx] * sizeof(u32);
		data += tlv->len;
	}

	return 0;
}

void unic_get_regs(struct net_device *netdev, struct ethtool_regs *cmd,
		   void *data)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	u8 *pdata = (u8 *)data;
	u32 reg_arr_size;
	u32 *reg_num;
	int ret;

	if (unic_resetting(netdev)) {
		unic_err(unic_dev, "dev resetting, could not get regs.\n");
		return;
	}

	reg_arr_size = ARRAY_SIZE(unic_dfx_reg_arr);
	reg_num = kcalloc(reg_arr_size, sizeof(u32), GFP_KERNEL);
	if (!reg_num) {
		unic_err(unic_dev, "failed to alloc reg num array.\n");
		return;
	}

	pdata += unic_get_res_regs(unic_dev, pdata);
	ret = unic_get_dfx_reg_num(unic_dev, reg_num, reg_arr_size);
	if (!ret) {
		ret = unic_get_dfx_regs(unic_dev, pdata, unic_dfx_reg_arr,
					reg_arr_size, reg_num);
		if (ret)
			unic_err(unic_dev,
				 "failed to get dfx regs, ret = %d.\n", ret);
	} else if (ret != -EPERM) {
		unic_err(unic_dev,
			 "failed to get dfx reg num, ret = %d.\n", ret);
	}

	kfree(reg_num);
}

static void unic_get_fec_stats_total(struct unic_dev *unic_dev, u8 stats_flags,
				     struct ethtool_fec_stats *fec_stats)
{
	struct unic_fec_stats_item *total = &unic_dev->stats.fec_stats.total;

	if (stats_flags & UNIC_FEC_CORR_BLOCKS)
		fec_stats->corrected_blocks.total = total->corr_blocks;
	if (stats_flags & UNIC_FEC_UNCORR_BLOCKS)
		fec_stats->uncorrectable_blocks.total = total->uncorr_blocks;
	if (stats_flags & UNIC_FEC_CORR_BITS)
		fec_stats->corrected_bits.total = total->corr_bits;
}

static void unic_get_ubl_fec_stats(struct unic_dev *unic_dev,
				   struct ethtool_fec_stats *fec_stats)
{
	u32 fec_mode = unic_dev->hw.mac.fec_mode;
	u8 stats_flags = 0;

	switch (fec_mode) {
	case ETHTOOL_FEC_RS:
		stats_flags = UNIC_FEC_UNCORR_BLOCKS | UNIC_FEC_CORR_BITS;
		unic_get_fec_stats_total(unic_dev, stats_flags, fec_stats);
		break;
	default:
		unic_err(unic_dev,
			 "fec stats is not supported in mode(0x%x).\n",
			 fec_mode);
		break;
	}
}

void unic_get_fec_stats(struct net_device *ndev,
			struct ethtool_fec_stats *fec_stats)
{
	struct unic_dev *unic_dev = netdev_priv(ndev);

	if (!unic_dev_fec_stats_supported(unic_dev) ||
	    unic_dev->hw.mac.fec_mode == ETHTOOL_FEC_OFF)
		return;

	if (unic_update_fec_stats(unic_dev))
		return;

	if (unic_dev_ubl_supported(unic_dev))
		unic_get_ubl_fec_stats(unic_dev, fec_stats);
}
