// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include "udma_common.h"
#include "udma_dev.h"
#include <uapi/ub/urma/udma/udma_abi.h>
#include "udma_cmd.h"
#include "udma_jetty.h"
#include "udma_jfs.h"
#include "udma_jfc.h"
#include "udma_db.h"
#include "udma_ctrlq_tp.h"
#include <ub/urma/udma/udma_ctl.h>
#include "udma_def.h"

const char *udma_cqe_aux_info_type_str[] = {
	"TPP2TQEM_WR_CNT",
	"DEVICE_RAS_STATUS_2",
	"RXDMA_WR_PAYL_AXI_ERR",
	"RXDMA_HEAD_SPLIT_ERR_FLAG0",
	"RXDMA_HEAD_SPLIT_ERR_FLAG1",
	"RXDMA_HEAD_SPLIT_ERR_FLAG2",
	"RXDMA_HEAD_SPLIT_ERR_FLAG3",
	"TP_RCP_INNER_ALM",
	"TWP_AE_DFX",
	"PA_OUT_PKT_ERR_CNT",
	"TP_DAM_AXI_ALARM",
	"TP_DAM_VFT_BT_ALARM",
	"TP_EUM_AXI_ALARM",
	"TP_EUM_VFT_BT_ALARM",
	"TP_TPMM_AXI_ALARM",
	"TP_TPMM_VFT_BT_ALARM",
	"TP_TPGCM_AXI_ALARM",
	"TP_TPGCM_VFT_BT_ALARM",
	"TWP_ALM",
	"TP_RWP_INNER_ALM",
	"TWP_DFX21",
	"LQC_TA_RNR_TANACK_CNT",
	"FVT",
	"RQMT0",
	"RQMT1",
	"RQMT2",
	"RQMT3",
	"RQMT4",
	"RQMT5",
	"RQMT6",
	"RQMT7",
	"RQMT8",
	"RQMT9",
	"RQMT10",
	"RQMT11",
	"RQMT12",
	"RQMT13",
	"RQMT14",
	"RQMT15",
	"PROC_ERROR_ALM",
	"LQC_TA_TIMEOUT_TAACK_CNT",
	"TP_RRP_ERR_FLG_0",
};

const char *udma_ae_aux_info_type_str[] = {
	"TP_RRP_FLUSH_TIMER_PKT_CNT",
	"TPP_DFX5",
	"TWP_AE_DFX",
	"TP_RRP_ERR_FLG_0",
	"TP_RRP_ERR_FLG_1",
	"TP_RWP_INNER_ALM",
	"TP_RCP_INNER_ALM",
	"LQC_TA_TQEP_WQE_ERR",
	"LQC_TA_CQM_CQE_INNER_ALARM",
};

static void dump_cqe_client_loc_len_err_aux_info(struct udma_dev *dev,
						 struct udma_cqe_aux_info_out *aux_info_out,
						 struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		TPP2TQEM_WR_CNT,
		DEVICE_RAS_STATUS_2,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_client_loc_access_err_aux_info(struct udma_dev *dev,
						    struct udma_cqe_aux_info_out *aux_info_out,
						    struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		RXDMA_WR_PAYL_AXI_ERR,
		RXDMA_HEAD_SPLIT_ERR_FLAG0,
		RXDMA_HEAD_SPLIT_ERR_FLAG1,
		RXDMA_HEAD_SPLIT_ERR_FLAG2,
		RXDMA_HEAD_SPLIT_ERR_FLAG3,
		TP_RCP_INNER_ALM_FOR_CQE,
		TWP_AE_DFX_FOR_CQE,
		PA_OUT_PKT_ERR_CNT,
		TP_DAM_AXI_ALARM,
		TP_DAM_VFT_BT_ALARM,
		TP_EUM_AXI_ALARM,
		TP_EUM_VFT_BT_ALARM,
		TP_TPMM_AXI_ALARM,
		TP_TPMM_VFT_BT_ALARM,
		TP_TPGCM_AXI_ALARM,
		TP_TPGCM_VFT_BT_ALARM,
		DEVICE_RAS_STATUS_2,
		TWP_ALM,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_client_rem_resp_len_err_aux_info(struct udma_dev *dev,
						      struct udma_cqe_aux_info_out *aux_info_out,
						      struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		TP_RWP_INNER_ALM_FOR_CQE,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void
dump_cqe_client_rem_access_abort_err_aux_info(struct udma_dev *dev,
					      struct udma_cqe_aux_info_out *aux_info_out,
					      struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		RXDMA_WR_PAYL_AXI_ERR,
		RXDMA_HEAD_SPLIT_ERR_FLAG0,
		RXDMA_HEAD_SPLIT_ERR_FLAG1,
		RXDMA_HEAD_SPLIT_ERR_FLAG2,
		RXDMA_HEAD_SPLIT_ERR_FLAG3,
		TP_RCP_INNER_ALM_FOR_CQE,
		TP_RRP_ERR_FLG_0_FOR_CQE,
		TPP2TQEM_WR_CNT,
		TWP_DFX21
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_client_ack_timeout_err_aux_info(struct udma_dev *dev,
						     struct udma_cqe_aux_info_out *aux_info_out,
						     struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		LQC_TA_TIMEOUT_TAACK_CNT,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void
dump_cqe_client_rnr_retry_cnt_exc_err_aux_info(struct udma_dev *dev,
					       struct udma_cqe_aux_info_out *aux_info_out,
					       struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		LQC_TA_RNR_TANACK_CNT,
		FVT,
		RQMT0,
		RQMT1,
		RQMT2,
		RQMT3,
		RQMT4,
		RQMT5,
		RQMT6,
		RQMT7,
		RQMT8,
		RQMT9,
		RQMT10,
		RQMT11,
		RQMT12,
		RQMT13,
		RQMT14,
		RQMT15,
		PROC_ERROR_ALM,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_server_loc_access_err_aux_info(struct udma_dev *dev,
						    struct udma_cqe_aux_info_out *aux_info_out,
						    struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		TP_RWP_INNER_ALM_FOR_CQE,
		RXDMA_WR_PAYL_AXI_ERR,
		RXDMA_HEAD_SPLIT_ERR_FLAG0,
		RXDMA_HEAD_SPLIT_ERR_FLAG1,
		RXDMA_HEAD_SPLIT_ERR_FLAG2,
		RXDMA_HEAD_SPLIT_ERR_FLAG3,
		TP_RCP_INNER_ALM_FOR_CQE,
		TP_RRP_ERR_FLG_0_FOR_CQE,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_server_loc_len_err_aux_info(struct udma_dev *dev,
						 struct udma_cqe_aux_info_out *aux_info_out,
						 struct udma_cmd_query_cqe_aux_info *info)
{
	enum udma_cqe_aux_info_type type[] = {
		TP_RWP_INNER_ALM_FOR_CQE,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[type[i]], info->cqe_aux_info[type[i]]);
}

static void dump_cqe_all_aux_info(struct udma_dev *dev,
				  struct udma_cqe_aux_info_out *aux_info_out,
				  struct udma_cmd_query_cqe_aux_info *info)
{
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= MAX_CQE_AUX_INFO_TYPE_NUM) {
		for (i = 0; i < MAX_CQE_AUX_INFO_TYPE_NUM; i++) {
			aux_info_out->aux_info_type[i] = i;
			aux_info_out->aux_info_value[i] = info->cqe_aux_info[i];
		}
		aux_info_out->aux_info_num = MAX_CQE_AUX_INFO_TYPE_NUM;
	}

	for (i = 0; i < MAX_CQE_AUX_INFO_TYPE_NUM; i++)
		dev_info(dev->dev, "%s\t0x%08x\n",
			 udma_cqe_aux_info_type_str[i], info->cqe_aux_info[i]);
}

static void (*udma_cqe_aux_info_dump[14][2])(struct udma_dev *dev,
	struct udma_cqe_aux_info_out *aux_info_out,
	struct udma_cmd_query_cqe_aux_info *info) = {
	{NULL, NULL},
	{dump_cqe_all_aux_info, dump_cqe_all_aux_info},
	{dump_cqe_server_loc_len_err_aux_info,
		dump_cqe_client_loc_len_err_aux_info},
	{NULL, NULL},
	{dump_cqe_server_loc_access_err_aux_info,
		dump_cqe_client_loc_access_err_aux_info},
	{dump_cqe_all_aux_info,
		dump_cqe_client_rem_resp_len_err_aux_info},
	{dump_cqe_all_aux_info, dump_cqe_all_aux_info},
	{NULL, NULL},
	{dump_cqe_all_aux_info,
		dump_cqe_client_rem_access_abort_err_aux_info},
	{dump_cqe_all_aux_info,
		dump_cqe_client_ack_timeout_err_aux_info},
	{dump_cqe_all_aux_info,
		dump_cqe_client_rnr_retry_cnt_exc_err_aux_info},
	{dump_cqe_all_aux_info, dump_cqe_all_aux_info},
	{NULL, NULL},
	{dump_cqe_all_aux_info, dump_cqe_all_aux_info},
};

static void dump_fill_aux_info(struct udma_dev *dev, struct udma_ae_aux_info_out *aux_info_out,
			       struct udma_cmd_query_ae_aux_info *info,
			       enum udma_ae_aux_info_type *type, uint32_t aux_info_num)
{
	int i;

	if (aux_info_out->aux_info_type != NULL &&
	    aux_info_out->aux_info_value != NULL &&
	    aux_info_out->aux_info_num >= aux_info_num) {
		for (i = 0; i < aux_info_num; i++) {
			aux_info_out->aux_info_type[i] = type[i];
			aux_info_out->aux_info_value[i] = info->ae_aux_info[type[i]];
		}
		aux_info_out->aux_info_num = aux_info_num;
	}

	for (i = 0; i < aux_info_num; i++)
		dev_info(dev->dev, "%s\t0x%08x\n", udma_ae_aux_info_type_str[type[i]],
			 info->ae_aux_info[type[i]]);
}

static void dump_ae_tp_flush_done_aux_info(struct udma_dev *dev,
					   struct udma_ae_aux_info_out *aux_info_out,
					   struct udma_cmd_query_ae_aux_info *info)
{
	enum udma_ae_aux_info_type type[] = {
		TP_RRP_FLUSH_TIMER_PKT_CNT,
		TPP_DFX5,
	};

	uint32_t aux_info_num = ARRAY_SIZE(type);

	dump_fill_aux_info(dev, aux_info_out, info, type, aux_info_num);
}

static void dump_ae_tp_err_aux_info(struct udma_dev *dev,
				    struct udma_ae_aux_info_out *aux_info_out,
				    struct udma_cmd_query_ae_aux_info *info)
{
	enum udma_ae_aux_info_type type[] = {
		TWP_AE_DFX_FOR_AE,
		TP_RRP_ERR_FLG_0_FOR_AE,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);

	dump_fill_aux_info(dev, aux_info_out, info, type, aux_info_num);
}

static void dump_ae_jetty_err_aux_info(struct udma_dev *dev,
				       struct udma_ae_aux_info_out *aux_info_out,
				       struct udma_cmd_query_ae_aux_info *info)
{
	enum udma_ae_aux_info_type type[] = {
		TP_RRP_ERR_FLG_0_FOR_AE,
		TP_RRP_ERR_FLG_1,
		TP_RWP_INNER_ALM_FOR_AE,
		TP_RCP_INNER_ALM_FOR_AE,
		LQC_TA_TQEP_WQE_ERR,
		LQC_TA_CQM_CQE_INNER_ALARM,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);

	dump_fill_aux_info(dev, aux_info_out, info, type, aux_info_num);
}

static void dump_ae_jfc_err_aux_info(struct udma_dev *dev,
				     struct udma_ae_aux_info_out *aux_info_out,
				     struct udma_cmd_query_ae_aux_info *info)
{
	enum udma_ae_aux_info_type type[] = {
		LQC_TA_CQM_CQE_INNER_ALARM,
	};
	uint32_t aux_info_num = ARRAY_SIZE(type);

	dump_fill_aux_info(dev, aux_info_out, info, type, aux_info_num);
}

static void dump_ae_aux_info(struct udma_dev *dev,
			     struct udma_ae_aux_info_out *aux_info_out,
			     struct udma_cmd_query_ae_aux_info *info)
{
	switch (info->event_type) {
	case UBASE_EVENT_TYPE_TP_FLUSH_DONE:
		dump_ae_tp_flush_done_aux_info(dev, aux_info_out, info);
		break;
	case UBASE_EVENT_TYPE_TP_LEVEL_ERROR:
		dump_ae_tp_err_aux_info(dev, aux_info_out, info);
		break;
	case UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR:
		if (info->sub_type == UBASE_SUBEVENT_TYPE_JFS_CHECK_ERROR)
			dump_ae_jetty_err_aux_info(dev, aux_info_out, info);
		else
			dump_ae_jfc_err_aux_info(dev, aux_info_out, info);
		break;
	default:
		break;
	}
}

static int send_cmd_query_cqe_aux_info(struct udma_dev *udma_dev,
				       struct udma_cmd_query_cqe_aux_info *info)
{
	struct ubase_cmd_buf cmd_in, cmd_out;
	int ret;

	udma_fill_buf(&cmd_in, UDMA_CMD_GET_CQE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_cqe_aux_info), info);
	udma_fill_buf(&cmd_out, UDMA_CMD_GET_CQE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_cqe_aux_info), info);

	ret = ubase_cmd_send_inout(udma_dev->comdev.adev, &cmd_in, &cmd_out);
	if (ret)
		dev_err(udma_dev->dev,
			"failed to query cqe aux info, ret = %d.\n", ret);

	return ret;
}

static void free_kernel_cqe_aux_info(struct udma_cqe_aux_info_out *user_aux_info_out,
				     struct udma_cqe_aux_info_out *aux_info_out)
{
	if (!user_aux_info_out->aux_info_type)
		return;

	kfree(aux_info_out->aux_info_type);
	aux_info_out->aux_info_type = NULL;

	kfree(aux_info_out->aux_info_value);
	aux_info_out->aux_info_value = NULL;
}

static int copy_out_cqe_data_from_user(struct udma_dev *udma_dev,
				       struct ubcore_user_ctl_out *out,
				       struct udma_cqe_aux_info_out *aux_info_out,
				       struct ubcore_ucontext *uctx,
				       struct udma_cqe_aux_info_out *user_aux_info_out)
{
	if (out->addr != 0 && out->len == sizeof(struct udma_cqe_aux_info_out)) {
		memcpy(aux_info_out, (void *)(uintptr_t)out->addr,
		       sizeof(struct udma_cqe_aux_info_out));
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			if (aux_info_out->aux_info_num > MAX_CQE_AUX_INFO_TYPE_NUM) {
				dev_err(udma_dev->dev,
					"invalid cqe aux info num %u.\n",
					aux_info_out->aux_info_num);
				return -EINVAL;
			}

			user_aux_info_out->aux_info_type = aux_info_out->aux_info_type;
			user_aux_info_out->aux_info_value = aux_info_out->aux_info_value;
			aux_info_out->aux_info_type =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(enum udma_cqe_aux_info_type), GFP_KERNEL);
			if (!aux_info_out->aux_info_type)
				return -ENOMEM;

			aux_info_out->aux_info_value =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(uint32_t), GFP_KERNEL);
			if (!aux_info_out->aux_info_value) {
				kfree(aux_info_out->aux_info_type);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static int copy_out_cqe_data_to_user(struct udma_dev *udma_dev,
				     struct ubcore_user_ctl_out *out,
				     struct udma_cqe_aux_info_out *aux_info_out,
				     struct ubcore_ucontext *uctx,
				     struct udma_cqe_aux_info_out *user_aux_info_out)
{
	unsigned long byte;

	if (out->addr != 0 && out->len == sizeof(struct udma_cqe_aux_info_out)) {
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_type,
					    (void *)aux_info_out->aux_info_type,
					    aux_info_out->aux_info_num *
					    sizeof(enum udma_cqe_aux_info_type));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info type failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_value,
					    (void *)aux_info_out->aux_info_value,
					    aux_info_out->aux_info_num *
					    sizeof(uint32_t));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info value failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			kfree(aux_info_out->aux_info_type);
			kfree(aux_info_out->aux_info_value);
			aux_info_out->aux_info_type = user_aux_info_out->aux_info_type;
			aux_info_out->aux_info_value = user_aux_info_out->aux_info_value;
		}
		memcpy((void *)(uintptr_t)out->addr, aux_info_out,
		       sizeof(struct udma_cqe_aux_info_out));
	}

	return 0;
}

int udma_query_cqe_aux_info(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			    struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out)
{
	struct udma_cqe_aux_info_out user_aux_info_out = {};
	struct udma_cqe_aux_info_out aux_info_out = {};
	struct udma_cmd_query_cqe_aux_info info = {};
	struct udma_cqe_info_in cqe_info_in = {};
	struct udma_dev *udev = to_udma_dev(dev);
	int ret;

	if (udma_check_base_param(in->addr, in->len, sizeof(struct udma_cqe_info_in))) {
		dev_err(udev->dev, "parameter invalid in query cqe aux info, in_len = %u.\n",
			in->len);
		return -EINVAL;
	}
	memcpy(&cqe_info_in, (void *)(uintptr_t)in->addr,
	       sizeof(struct udma_cqe_info_in));

	info.status = cqe_info_in.status;
	info.is_client = !(cqe_info_in.s_r & 1);
	if (cqe_info_in.status >= ARRAY_SIZE(udma_cqe_aux_info_dump) ||
		udma_cqe_aux_info_dump[info.status][info.is_client] == NULL) {
		dev_err(udev->dev, "status %u is invalid or does not need to be queried.\n",
			cqe_info_in.status);
		return -EINVAL;
	}

	ret = copy_out_cqe_data_from_user(udev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udev->dev,
			"copy out data from user failed, ret = %d.\n", ret);
		return ret;
	}

	ret = send_cmd_query_cqe_aux_info(udev, &info);
	if (ret) {
		dev_err(udev->dev,
			"send cmd query aux info failed, ret = %d.\n",
			ret);
		free_kernel_cqe_aux_info(&user_aux_info_out, &aux_info_out);
		return ret;
	}

	udma_cqe_aux_info_dump[info.status][info.is_client](udev, &aux_info_out, &info);

	ret = copy_out_cqe_data_to_user(udev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udev->dev,
			"copy out data to user failed, ret = %d.\n", ret);
		free_kernel_cqe_aux_info(&user_aux_info_out, &aux_info_out);
	}

	return ret;
}

static int to_hw_ae_event_type(struct udma_dev *udma_dev, uint32_t event_type,
			       struct udma_cmd_query_ae_aux_info *info)
{
	switch (event_type) {
	case UBCORE_EVENT_TP_FLUSH_DONE:
		info->event_type = UBASE_EVENT_TYPE_TP_FLUSH_DONE;
		break;
	case UBCORE_EVENT_TP_ERR:
		info->event_type = UBASE_EVENT_TYPE_TP_LEVEL_ERROR;
		break;
	case UBCORE_EVENT_JFS_ERR:
	case UBCORE_EVENT_JETTY_ERR:
		info->event_type = UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR;
		info->sub_type = UBASE_SUBEVENT_TYPE_JFS_CHECK_ERROR;
		break;
	case UBCORE_EVENT_JFC_ERR:
		info->event_type = UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR;
		info->sub_type = UBASE_SUBEVENT_TYPE_JFC_CHECK_ERROR;
		break;
	default:
		dev_err(udma_dev->dev, "Invalid event type %u.\n", event_type);
		return -EINVAL;
	}

	return 0;
}

static int send_cmd_query_ae_aux_info(struct udma_dev *udma_dev,
				      struct udma_cmd_query_ae_aux_info *info)
{
	struct ubase_cmd_buf cmd_in, cmd_out;
	int ret;

	udma_fill_buf(&cmd_in, UDMA_CMD_GET_AE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_ae_aux_info), info);
	udma_fill_buf(&cmd_out, UDMA_CMD_GET_AE_AUX_INFO, true,
		      sizeof(struct udma_cmd_query_ae_aux_info), info);

	ret = ubase_cmd_send_inout(udma_dev->comdev.adev, &cmd_in, &cmd_out);
	if (ret)
		dev_err(udma_dev->dev,
			"failed to query ae aux info, ret = %d.\n", ret);

	return ret;
}

static void free_kernel_ae_aux_info(struct udma_ae_aux_info_out *user_aux_info_out,
				    struct udma_ae_aux_info_out *aux_info_out)
{
	if (!user_aux_info_out->aux_info_type)
		return;

	kfree(aux_info_out->aux_info_type);
	aux_info_out->aux_info_type = NULL;

	kfree(aux_info_out->aux_info_value);
	aux_info_out->aux_info_value = NULL;
}

static int copy_out_ae_data_from_user(struct udma_dev *udma_dev,
				      struct ubcore_user_ctl_out *out,
				      struct udma_ae_aux_info_out *aux_info_out,
				      struct ubcore_ucontext *uctx,
				      struct udma_ae_aux_info_out *user_aux_info_out)
{
	if (out->addr != 0 && out->len == sizeof(struct udma_ae_aux_info_out)) {
		memcpy(aux_info_out, (void *)(uintptr_t)out->addr,
		       sizeof(struct udma_ae_aux_info_out));
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			if (aux_info_out->aux_info_num > MAX_AE_AUX_INFO_TYPE_NUM) {
				dev_err(udma_dev->dev,
					"invalid ae aux info num %u.\n",
					aux_info_out->aux_info_num);
				return -EINVAL;
			}

			user_aux_info_out->aux_info_type = aux_info_out->aux_info_type;
			user_aux_info_out->aux_info_value = aux_info_out->aux_info_value;
			aux_info_out->aux_info_type =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(enum udma_ae_aux_info_type), GFP_KERNEL);
			if (!aux_info_out->aux_info_type)
				return -ENOMEM;

			aux_info_out->aux_info_value =
				kcalloc(aux_info_out->aux_info_num,
					sizeof(uint32_t), GFP_KERNEL);
			if (!aux_info_out->aux_info_value) {
				kfree(aux_info_out->aux_info_type);
				return -ENOMEM;
			}
		}
	}

	return 0;
}

static int copy_out_ae_data_to_user(struct udma_dev *udma_dev,
				    struct ubcore_user_ctl_out *out,
				    struct udma_ae_aux_info_out *aux_info_out,
				    struct ubcore_ucontext *uctx,
				    struct udma_ae_aux_info_out *user_aux_info_out)
{
	unsigned long byte;

	if (out->addr != 0 && out->len == sizeof(struct udma_ae_aux_info_out)) {
		if (uctx && aux_info_out->aux_info_num > 0 &&
		    aux_info_out->aux_info_type != NULL &&
		    aux_info_out->aux_info_value != NULL) {
			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_type,
					    (void *)aux_info_out->aux_info_type,
					    aux_info_out->aux_info_num *
					    sizeof(enum udma_ae_aux_info_type));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info type failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			byte = copy_to_user((void __user *)user_aux_info_out->aux_info_value,
					    (void *)aux_info_out->aux_info_value,
					    aux_info_out->aux_info_num *
					    sizeof(uint32_t));
			if (byte) {
				dev_err(udma_dev->dev,
					"copy resp to aux info value failed, byte = %lu.\n", byte);
				return -EFAULT;
			}

			kfree(aux_info_out->aux_info_type);
			kfree(aux_info_out->aux_info_value);
			aux_info_out->aux_info_type = user_aux_info_out->aux_info_type;
			aux_info_out->aux_info_value = user_aux_info_out->aux_info_value;
		}
		memcpy((void *)(uintptr_t)out->addr, aux_info_out,
		       sizeof(struct udma_ae_aux_info_out));
	}

	return 0;
}

int udma_query_ae_aux_info(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			   struct ubcore_user_ctl_in *in,
			   struct ubcore_user_ctl_out *out)
{
	struct udma_ae_aux_info_out user_aux_info_out = {};
	struct udma_ae_aux_info_out aux_info_out = {};
	struct udma_dev *udma_dev = to_udma_dev(dev);
	struct udma_cmd_query_ae_aux_info info = {};
	struct udma_ae_info_in ae_info_in = {};
	int ret;

	if (udma_check_base_param(in->addr, in->len, sizeof(struct udma_ae_info_in))) {
		dev_err(udma_dev->dev, "parameter invalid in query ae aux info, in_len = %u.\n",
			in->len);
		return -EINVAL;
	}
	memcpy(&ae_info_in, (void *)(uintptr_t)in->addr,
	       sizeof(struct udma_ae_info_in));
	ret = to_hw_ae_event_type(udma_dev, ae_info_in.event_type, &info);
	if (ret)
		return ret;

	ret = copy_out_ae_data_from_user(udma_dev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udma_dev->dev,
			"copy out data from user failed, ret = %d.\n", ret);
		return ret;
	}

	ret = send_cmd_query_ae_aux_info(udma_dev, &info);
	if (ret) {
		dev_err(udma_dev->dev,
			"send cmd query aux info failed, ret = %d.\n",
			ret);
		free_kernel_ae_aux_info(&user_aux_info_out, &aux_info_out);
		return ret;
	}

	dump_ae_aux_info(udma_dev, &aux_info_out, &info);

	ret = copy_out_ae_data_to_user(udma_dev, out, &aux_info_out, uctx, &user_aux_info_out);
	if (ret) {
		dev_err(udma_dev->dev,
			"copy out data to user failed, ret = %d.\n", ret);
		free_kernel_ae_aux_info(&user_aux_info_out, &aux_info_out);
	}

	return ret;
}

static udma_user_ctl_ops g_udma_user_ctl_k_ops[] = {
	[UDMA_USER_CTL_NPU_REGISTER_INFO_CB] = udma_register_npu_cb,
	[UDMA_USER_CTL_NPU_UNREGISTER_INFO_CB] = udma_unregister_npu_cb,
	[UDMA_USER_CTL_QUERY_CQE_AUX_INFO] = udma_query_cqe_aux_info,
	[UDMA_USER_CTL_QUERY_AE_AUX_INFO] = udma_query_ae_aux_info,
};

static udma_user_ctl_ops g_udma_user_ctl_u_ops[] = {
	[UDMA_USER_CTL_CREATE_JFS_EX] = NULL,
	[UDMA_USER_CTL_DELETE_JFS_EX] = NULL,
	[UDMA_USER_CTL_CREATE_JFC_EX] = NULL,
	[UDMA_USER_CTL_DELETE_JFC_EX] = NULL,
	[UDMA_USER_CTL_SET_CQE_ADDR] = NULL,
	[UDMA_USER_CTL_QUERY_UE_INFO] = NULL,
	[UDMA_USER_CTL_GET_DEV_RES_RATIO] = NULL,
	[UDMA_USER_CTL_NPU_REGISTER_INFO_CB] = NULL,
	[UDMA_USER_CTL_NPU_UNREGISTER_INFO_CB] = NULL,
	[UDMA_USER_CTL_QUERY_CQE_AUX_INFO] = udma_query_cqe_aux_info,
	[UDMA_USER_CTL_QUERY_AE_AUX_INFO] = udma_query_ae_aux_info,
	[UDMA_USER_CTL_QUERY_UBMEM_INFO] = NULL,
	[UDMA_USER_CTL_QUERY_PAIR_DEVNUM] = NULL,
};

static int udma_user_data(struct ubcore_device *dev,
			  struct ubcore_user_ctl *k_user_ctl)
{
	struct udma_dev *udev = to_udma_dev(dev);
	struct ubcore_user_ctl_out out = {};
	struct ubcore_user_ctl_in in = {};
	unsigned long byte;
	int ret;

	if (k_user_ctl->in.len >= UDMA_HW_PAGE_SIZE || k_user_ctl->out.len >= UDMA_HW_PAGE_SIZE) {
		dev_err(udev->dev, "The len exceeds the maximum value in user ctrl.\n");
		return -EINVAL;
	}

	in.opcode = k_user_ctl->in.opcode;
	if (!g_udma_user_ctl_u_ops[in.opcode]) {
		dev_err(udev->dev, "invalid user opcode: 0x%x.\n", in.opcode);
		return -EINVAL;
	}

	if (k_user_ctl->in.len) {
		in.addr = (uint64_t)kzalloc(k_user_ctl->in.len, GFP_KERNEL);
		if (!in.addr)
			return -ENOMEM;

		in.len = k_user_ctl->in.len;
		byte = copy_from_user((void *)(uintptr_t)in.addr,
			(void __user *)(uintptr_t)k_user_ctl->in.addr,
			k_user_ctl->in.len);
		if (byte) {
			dev_err(udev->dev,
				"failed to copy user data in user ctrl, byte = %lu.\n", byte);
			kfree((void *)in.addr);
			return -EFAULT;
		}
	}

	if (k_user_ctl->out.len) {
		out.addr = (uint64_t)kzalloc(k_user_ctl->out.len, GFP_KERNEL);
		if (!out.addr) {
			kfree((void *)in.addr);

			return -ENOMEM;
		}
		out.len = k_user_ctl->out.len;

		if (k_user_ctl->out.addr) {
			byte = copy_from_user((void *)(uintptr_t)out.addr,
				(void __user *)(uintptr_t)k_user_ctl->out.addr,
				k_user_ctl->out.len);
			if (byte) {
				dev_err(udev->dev,
					"failed to copy user data out user ctrl, byte = %lu.\n",
					byte);
				kfree((void *)out.addr);
				kfree((void *)in.addr);

				return -EFAULT;
			}
		}
	}

	ret = g_udma_user_ctl_u_ops[in.opcode](dev, k_user_ctl->uctx, &in, &out);
	kfree((void *)in.addr);

	if (out.addr) {
		byte = copy_to_user((void __user *)(uintptr_t)k_user_ctl->out.addr,
				    (void *)(uintptr_t)out.addr, min(out.len, k_user_ctl->out.len));
		if (byte) {
			dev_err(udev->dev,
				"copy resp to user failed in user ctrl, byte = %lu.\n", byte);
			ret = -EFAULT;
		}

		kfree((void *)out.addr);
	}

	return ret;
}

int udma_user_ctl(struct ubcore_device *dev, struct ubcore_user_ctl *k_user_ctl)
{
	struct udma_dev *udev;

	if (dev == NULL || k_user_ctl == NULL)
		return -EINVAL;

	udev = to_udma_dev(dev);

	if (k_user_ctl->in.opcode >= UDMA_USER_CTL_MAX) {
		dev_err(udev->dev, "invalid opcode: 0x%x.\n", k_user_ctl->in.opcode);
		return -EINVAL;
	}

	if (k_user_ctl->uctx)
		return udma_user_data(dev, k_user_ctl);

	if (!g_udma_user_ctl_k_ops[k_user_ctl->in.opcode]) {
		dev_err(udev->dev, "invalid user opcode: 0x%x.\n", k_user_ctl->in.opcode);
		return -EINVAL;
	}

	return g_udma_user_ctl_k_ops[k_user_ctl->in.opcode](dev, k_user_ctl->uctx, &k_user_ctl->in,
	       &k_user_ctl->out);
}
