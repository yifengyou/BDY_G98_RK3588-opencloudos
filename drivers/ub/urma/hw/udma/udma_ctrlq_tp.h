/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#ifndef __UDMA_CTRLQ_TP_H__
#define __UDMA_CTRLQ_TP_H__

#include "udma_common.h"

#define UDMA_EID_SIZE		16
#define UDMA_CNA_SIZE		16
#define UDMA_PID_MASK		0xFFFFFF
#define UDMA_DEFAULT_PID	1
#define UDMA_UE_NUM		64
#define UDMA_MAX_UE_IDX		256
#define UDMA_MAX_TPID_NUM	5

#define UDMA_CTRLQ_UBMEM_INFO_NUM (96)
#define UDMA_TPN_CNT_MASK 0x1F

enum udma_ctrlq_cmd_code_type {
	UDMA_CMD_CTRLQ_REMOVE_SINGLE_TP = 0x13,
	UDMA_CMD_CTRLQ_TP_FLUSH_DONE,
	UDMA_CMD_CTRLQ_CHECK_TP_ACTIVE,
	UDMA_CMD_CTRLQ_GET_TP_LIST = 0x21,
	UDMA_CMD_CTRLQ_ACTIVE_TP,
	UDMA_CMD_CTRLQ_DEACTIVE_TP,
	UDMA_CMD_CTRLQ_SET_TP_ATTR,
	UDMA_CMD_CTRLQ_GET_TP_ATTR,
	UDMA_CMD_CTRLQ_MAX
};

enum udma_ctrlq_ubmem_opcode {
	UDMA_CTRLQ_QUERY_UBMEM_INFO = 0x1,
};

enum udma_ctrlq_trans_type {
	UDMA_CTRLQ_TRANS_TYPE_TP_RM = 0,
	UDMA_CTRLQ_TRANS_TYPE_CTP,
	UDMA_CTRLQ_TRANS_TYPE_TP_UM,
	UDMA_CTRLQ_TRANS_TYPE_TP_RC = 4,
	UDMA_CTRLQ_TRANS_TYPE_MAX
};

enum udma_ctrlq_tpid_status {
	UDMA_CTRLQ_TPID_IN_USE = 0,
	UDMA_CTRLQ_TPID_EXITED,
	UDMA_CTRLQ_TPID_IDLE,
};

struct udma_ctrlq_tpid {
	uint32_t tpid : 24;
	uint32_t tpn_cnt : 8;
	uint32_t tpn_start : 24;
	uint32_t rsv0 : 4;
	uint32_t migr : 1;
	uint32_t rsv1 : 3;
};

struct udma_ctrlq_tpid_list_rsp {
	uint32_t tp_list_cnt : 16;
	uint32_t rsv : 16;
	struct udma_ctrlq_tpid tpid_list[UDMA_MAX_TPID_NUM];
};

struct udma_ctrlq_active_tp_req_data {
	uint32_t local_tp_id : 24;
	uint32_t local_tpn_cnt : 8;
	uint32_t local_tpn_start : 24;
	uint32_t rsv : 8;
	uint32_t remote_tp_id : 24;
	uint32_t remote_tpn_cnt : 8;
	uint32_t remote_tpn_start : 24;
	uint32_t rsv1 : 8;
	uint32_t local_psn;
	uint32_t remote_psn;
};

struct udma_ctrlq_active_tp_resp_data {
	uint32_t local_tp_id : 24;
	uint32_t local_tpn_cnt : 8;
	uint32_t local_tpn_start : 24;
	uint32_t rsv : 8;
};

struct udma_ctrlq_deactive_tp_req_data {
	uint32_t tp_id : 24;
	uint32_t tpn_cnt : 8;
	uint32_t start_tpn : 24;
	uint32_t rsv : 8;
	uint32_t pid_flag : 24;
	uint32_t rsv1 : 8;
};

struct udma_ctrlq_tp_flush_done_req_data {
	uint32_t tpn : 24;
	uint32_t rsv : 8;
};

struct udma_ctrlq_remove_single_tp_req_data {
	uint32_t tpn : 24;
	uint32_t tp_status : 8;
};

struct udma_ctrlq_tpn_data {
	uint32_t tpg_flag : 8;
	uint32_t rsv : 24;
	uint32_t tpgn : 24;
	uint32_t rsv1 : 8;
	uint32_t tpn_cnt : 8;
	uint32_t start_tpn : 24;
};

struct udma_ctrlq_check_tp_active_req_data {
	uint32_t tp_id : 24;
	uint32_t rsv : 8;
	uint32_t pid_flag : 24;
	uint32_t rsv1 : 8;
};

struct udma_ctrlq_check_tp_active_req_info {
	uint32_t num : 8;
	uint32_t rsv : 24;
	struct udma_ctrlq_check_tp_active_req_data data[];
};

struct udma_ctrlq_check_tp_active_rsp_data {
	uint32_t tp_id : 24;
	uint32_t result : 8;
};

struct udma_ctrlq_check_tp_active_rsp_info {
	uint32_t num : 8;
	uint32_t rsv : 24;
	struct udma_ctrlq_check_tp_active_rsp_data data[];
};

struct udma_ctrlq_get_tp_list_req_data {
	uint8_t seid[UDMA_EID_SIZE];
	uint8_t deid[UDMA_EID_SIZE];
	uint32_t trans_type : 4;
	uint32_t rsv : 4;
	uint32_t flag : 24;
};

enum udma_cmd_ue_opcode {
	UDMA_CMD_UBCORE_COMMAND = 0x1,
	UDMA_CMD_NOTIFY_MUE_SAVE_TP = 0x2,
	UDMA_CMD_NOTIFY_UE_FLUSH_DONE = 0x3,
};

struct udma_ue_tp_info {
	uint32_t tp_cnt : 8;
	uint32_t start_tpn : 24;
};

struct udma_ue_idx_table {
	uint32_t num;
	uint8_t ue_idx[UDMA_UE_NUM];
};

struct udma_ctrlq_ubmem_out_query {
	uint32_t data[UDMA_CTRLQ_UBMEM_INFO_NUM];
};

struct udma_ctrlq_tp_attr {
	uint32_t tp_attr_bitmap;
	struct ubcore_tp_attr_value tp_attr_value;
};

struct udma_ctrlq_get_tp_attr_req {
	struct udma_ctrlq_tpid tpid;
};

struct udma_ctrlq_set_tp_attr_req {
	uint32_t tpid : 24;
	uint32_t tpn_cnt : 8;
	uint32_t tpn_start : 24;
	uint32_t tp_attr_cnt : 8;
	struct udma_ctrlq_tp_attr tp_attr;
};

struct udma_ctrlq_get_tp_attr_resp {
	uint32_t tpid : 24;
	uint32_t tp_attr_cnt : 8;
	struct udma_ctrlq_tp_attr tp_attr;
};

struct udma_dev_resource_ratio {
	struct ubase_bus_eid eid;
	uint32_t index;
};

int udma_query_pair_dev_count(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			      struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out);

int udma_get_dev_resource_ratio(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
				struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out);

int udma_register_npu_cb(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			 struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out);

int udma_unregister_npu_cb(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			   struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out);
int udma_ctrlq_tp_flush_done(struct udma_dev *udev, uint32_t tpn);
int udma_ctrlq_remove_single_tp(struct udma_dev *udev, uint32_t tpn, int status);
int udma_get_tp_list(struct ubcore_device *dev, struct ubcore_get_tp_cfg *tpid_cfg,
		     uint32_t *tp_cnt, struct ubcore_tp_info *tp_list,
		     struct ubcore_udata *udata);

void udma_ctrlq_destroy_tpid_list(struct udma_dev *dev, struct xarray *ctrlq_tpid_table,
				  bool is_need_flush);
int udma_ctrlq_set_active_tp_ex(struct udma_dev *dev,
				struct ubcore_active_tp_cfg *active_cfg);
int udma_ctrlq_query_ubmem_info(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
				struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out);

int udma_set_tp_attr(struct ubcore_device *dev, const uint64_t tp_handle,
		     const uint8_t tp_attr_cnt, const uint32_t tp_attr_bitmap,
		     const struct ubcore_tp_attr_value *tp_attr, struct ubcore_udata *udata);
int udma_get_tp_attr(struct ubcore_device *dev, const uint64_t tp_handle,
		    uint8_t *tp_attr_cnt, uint32_t *tp_attr_bitmap,
		    struct ubcore_tp_attr_value *tp_attr, struct ubcore_udata *udata);
int send_resp_to_ue(struct udma_dev *udma_dev, struct ubcore_resp *req_host,
		    uint8_t dst_ue_idx, uint16_t opcode);
int send_req_to_mue(struct udma_dev *udma_dev, struct ubcore_req *req, uint16_t opcode);
int udma_active_tp(struct ubcore_device *dev, struct ubcore_active_tp_cfg *active_cfg);
int udma_deactive_tp(struct ubcore_device *dev, union ubcore_tp_handle tp_handle,
		     struct ubcore_udata *udata);

#endif /* __UDMA_CTRLQ_TP_H__ */
