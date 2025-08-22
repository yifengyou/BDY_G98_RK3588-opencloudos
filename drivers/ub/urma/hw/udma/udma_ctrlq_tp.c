// SPDX-License-Identifier: GPL-2.0+
/* Copyright(c) 2025 HiSilicon Technologies CO., Ltd. All rights reserved. */

#define dev_fmt(fmt) "UDMA: " fmt

#include <linux/slab.h>
#include <ub/ubase/ubase_comm_dev.h>
#include <ub/ubase/ubase_comm_cmd.h>
#include <ub/ubase/ubase_comm_ctrlq.h>
#include "udma_cmd.h"
#include <ub/urma/udma/udma_ctl.h>
#include "udma_common.h"
#include "udma_ctrlq_tp.h"

static void udma_ctrlq_set_tp_msg(struct ubase_ctrlq_msg *msg, void *in,
				  uint16_t in_len, void *out, uint16_t out_len)
{
	msg->service_ver = UBASE_CTRLQ_SER_VER_01;
	msg->service_type = UBASE_CTRLQ_SER_TYPE_TP_ACL;
	msg->need_resp = 1;
	msg->is_resp = 0;
	msg->in_size = in_len;
	msg->in = in;
	msg->out_size = out_len;
	msg->out = out;
}

int udma_ctrlq_remove_single_tp(struct udma_dev *udev, uint32_t tpn, int status)
{
	struct udma_ctrlq_remove_single_tp_req_data tp_cfg_req = {};
	struct ubase_ctrlq_msg msg = {};
	int r_status = 0;
	int ret;

	tp_cfg_req.tpn = tpn;
	tp_cfg_req.tp_status = (uint32_t)status;
	msg.opcode = UDMA_CMD_CTRLQ_REMOVE_SINGLE_TP;
	udma_ctrlq_set_tp_msg(&msg, (void *)&tp_cfg_req,
			      sizeof(tp_cfg_req), &r_status, sizeof(int));

	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &msg);
	if (ret)
		dev_err(udev->dev, "remove single tp %u failed, ret %d status %d.\n",
			tpn, ret, r_status);

	return ret;
}

static int udma_send_req_to_ue(struct udma_dev *udma_dev, uint8_t ue_idx)
{
	struct ubcore_resp *ubcore_req;
	int ret;

	ubcore_req = kzalloc(sizeof(*ubcore_req), GFP_KERNEL);
	if (!ubcore_req)
		return -ENOMEM;

	ret = send_resp_to_ue(udma_dev, ubcore_req, ue_idx,
			      UDMA_CMD_NOTIFY_UE_FLUSH_DONE);
	if (ret)
		dev_err(udma_dev->dev, "fail to notify ue the tp flush done, ret %d.\n", ret);

	kfree(ubcore_req);

	return ret;
}

static struct udma_ue_idx_table *udma_find_ue_idx_by_tpn(struct udma_dev *udev,
							 uint32_t tpn)
{
	struct udma_ue_idx_table *tp_ue_idx_info;

	xa_lock(&udev->tpn_ue_idx_table);
	tp_ue_idx_info = xa_load(&udev->tpn_ue_idx_table, tpn);
	if (!tp_ue_idx_info) {
		dev_warn(udev->dev, "ue idx info not exist, tpn %u.\n", tpn);
		xa_unlock(&udev->tpn_ue_idx_table);

		return NULL;
	}

	__xa_erase(&udev->tpn_ue_idx_table, tpn);
	xa_unlock(&udev->tpn_ue_idx_table);

	return tp_ue_idx_info;
}

int udma_ctrlq_tp_flush_done(struct udma_dev *udev, uint32_t tpn)
{
	struct udma_ctrlq_tp_flush_done_req_data tp_cfg_req = {};
	struct udma_ue_idx_table *tp_ue_idx_info;
	struct ubase_ctrlq_msg msg = {};
	int ret = 0;
	uint32_t i;

	tp_ue_idx_info = udma_find_ue_idx_by_tpn(udev, tpn);
	if (tp_ue_idx_info) {
		for (i = 0; i < tp_ue_idx_info->num; i++)
			(void)udma_send_req_to_ue(udev, tp_ue_idx_info->ue_idx[i]);

		kfree(tp_ue_idx_info);
	} else {
		ret = udma_open_ue_rx(udev, true, false, false, 0);
		if (ret)
			dev_err(udev->dev, "udma open ue rx failed in tp flush done.\n");
	}

	tp_cfg_req.tpn = tpn;
	msg.opcode = UDMA_CMD_CTRLQ_TP_FLUSH_DONE;
	udma_ctrlq_set_tp_msg(&msg, (void *)&tp_cfg_req, sizeof(tp_cfg_req), NULL, 0);
	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &msg);
	if (ret)
		dev_err(udev->dev, "tp flush done ctrlq tp %u failed, ret %d.\n", tpn, ret);

	return ret;
}

int udma_get_dev_resource_ratio(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
				struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out)
{
	struct udma_dev_resource_ratio dev_res = {};
	struct udma_dev_pair_info dev_res_out = {};
	struct udma_dev *udev = to_udma_dev(dev);
	struct ubase_ctrlq_msg ctrlq_msg = {};
	int ret = 0;

	if (udma_check_base_param(in->addr, in->len, sizeof(dev_res.index))) {
		dev_err(udev->dev, "parameter invalid in get dev res, len = %u.\n", in->len);
		return -EINVAL;
	}

	if (out->addr == 0 || out->len != sizeof(dev_res_out)) {
		dev_err(udev->dev, "get dev resource ratio, addr is NULL:%d, len:%u.\n",
			out->addr == 0, out->len);
		return -EINVAL;
	}

	memcpy(&dev_res.index, (void *)(uintptr_t)in->addr, sizeof(dev_res.index));

	ret = ubase_get_bus_eid(udev->comdev.adev, &dev_res.eid);
	if (ret) {
		dev_err(udev->dev, "get dev bus eid failed, ret is %d.\n", ret);
		return ret;
	}

	ctrlq_msg.service_type = UBASE_CTRLQ_SER_TYPE_DEV_REGISTER;
	ctrlq_msg.service_ver = UBASE_CTRLQ_SER_VER_01;
	ctrlq_msg.need_resp = 1;
	ctrlq_msg.in_size = sizeof(dev_res);
	ctrlq_msg.in = (void *)&dev_res;
	ctrlq_msg.out_size = sizeof(dev_res_out);
	ctrlq_msg.out = &dev_res_out;
	ctrlq_msg.opcode = UDMA_CTRLQ_GET_DEV_RESOURCE_RATIO;

	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &ctrlq_msg);
	if (ret) {
		dev_err(udev->dev, "get dev res send ctrlq msg failed, ret is %d.\n", ret);
		return ret;
	}
	memcpy((void *)(uintptr_t)out->addr, &dev_res_out, sizeof(dev_res_out));

	return ret;
}

static int udma_dev_res_ratio_ctrlq_handler(struct auxiliary_device *adev,
					    uint8_t service_ver, void *data,
					    uint16_t len, uint16_t seq)
{
	struct udma_dev *udev = (struct udma_dev *)get_udma_dev(adev);
	struct udma_ctrlq_event_nb *udma_cb;
	int ret;

	mutex_lock(&udev->npu_nb_mutex);
	udma_cb = xa_load(&udev->npu_nb_table, UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO);
	if (!udma_cb) {
		dev_err(udev->dev, "failed to query npu info cb while xa_load.\n");
		mutex_unlock(&udev->npu_nb_mutex);
		return -EINVAL;
	}

	ret = udma_cb->crq_handler(&udev->ub_dev, data, len);
	if (ret)
		dev_err(udev->dev, "npu crq handler failed, ret = %d.\n", ret);
	mutex_unlock(&udev->npu_nb_mutex);

	return ret;
}

int udma_register_npu_cb(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			 struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out)
{
	struct ubase_ctrlq_event_nb ubase_cb = {};
	struct udma_dev *udev = to_udma_dev(dev);
	struct udma_ctrlq_event_nb *udma_cb;
	int ret;

	if (udma_check_base_param(in->addr, in->len, sizeof(udma_cb->crq_handler))) {
		dev_err(udev->dev, "parameter invalid in register npu cb, len = %u.\n", in->len);
		return -EINVAL;
	}

	udma_cb = kzalloc(sizeof(*udma_cb), GFP_KERNEL);
	if (!udma_cb)
		return -ENOMEM;

	udma_cb->opcode = UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO;
	udma_cb->crq_handler = (void *)(uintptr_t)in->addr;

	mutex_lock(&udev->npu_nb_mutex);
	if (xa_load(&udev->npu_nb_table, UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO)) {
		dev_err(udev->dev, "query npu info callback exist.\n");
		ret = -EINVAL;
		goto err_release_udma_cb;
	}
	ret = xa_err(__xa_store(&udev->npu_nb_table, udma_cb->opcode, udma_cb, GFP_KERNEL));
	if (ret) {
		dev_err(udev->dev,
			"save crq nb entry failed, opcode is %u, ret is %d.\n",
			udma_cb->opcode, ret);
		goto err_release_udma_cb;
	}

	ubase_cb.service_type = UBASE_CTRLQ_SER_TYPE_DEV_REGISTER;
	ubase_cb.opcode = UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO;
	ubase_cb.back = udev->comdev.adev;
	ubase_cb.crq_handler = udma_dev_res_ratio_ctrlq_handler;
	ret = ubase_ctrlq_register_crq_event(udev->comdev.adev, &ubase_cb);
	if (ret) {
		__xa_erase(&udev->npu_nb_table, UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO);
		dev_err(udev->dev, "ubase register npu crq event failed, ret is %d.\n", ret);
		goto err_release_udma_cb;
	}
	mutex_unlock(&udev->npu_nb_mutex);

	return 0;

err_release_udma_cb:
	mutex_unlock(&udev->npu_nb_mutex);
	kfree(udma_cb);
	return ret;
}

int udma_unregister_npu_cb(struct ubcore_device *dev, struct ubcore_ucontext *uctx,
			   struct ubcore_user_ctl_in *in, struct ubcore_user_ctl_out *out)
{
	struct udma_dev *udev = to_udma_dev(dev);
	struct udma_ctrlq_event_nb *nb;

	ubase_ctrlq_unregister_crq_event(udev->comdev.adev,
					 UBASE_CTRLQ_SER_TYPE_DEV_REGISTER,
					 UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO);

	mutex_lock(&udev->npu_nb_mutex);
	nb = xa_load(&udev->npu_nb_table, UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO);
	if (!nb) {
		dev_warn(udev->dev, "query npu info cb not exist.\n");
		goto err_find_npu_nb;
	}

	__xa_erase(&udev->npu_nb_table, UDMA_CTRLQ_NOTIFY_DEV_RESOURCE_RATIO);
	kfree(nb);
	nb = NULL;

err_find_npu_nb:
	mutex_unlock(&udev->npu_nb_mutex);
	return 0;
}

static int udma_ctrlq_get_trans_type(struct udma_dev *dev,
				     enum ubcore_transport_mode trans_mode,
				     enum udma_ctrlq_trans_type *tp_type)
{
#define UDMA_TRANS_MODE_NUM 5

struct udma_ctrlq_trans_map {
	bool is_valid;
	enum udma_ctrlq_trans_type tp_type;
};
	static struct udma_ctrlq_trans_map ctrlq_trans_map[UDMA_TRANS_MODE_NUM] = {
		{false, UDMA_CTRLQ_TRANS_TYPE_MAX},
		{true, UDMA_CTRLQ_TRANS_TYPE_TP_RM},
		{true, UDMA_CTRLQ_TRANS_TYPE_TP_RC},
		{false, UDMA_CTRLQ_TRANS_TYPE_MAX},
		{true, UDMA_CTRLQ_TRANS_TYPE_TP_UM},
	};
	uint8_t transport_mode = (uint8_t)trans_mode;

	if ((transport_mode < UDMA_TRANS_MODE_NUM) &&
	    ctrlq_trans_map[transport_mode].is_valid) {
		*tp_type = ctrlq_trans_map[transport_mode].tp_type;
		return 0;
	}

	dev_err(dev->dev, "the trans_mode %u is not support.\n", trans_mode);

	return -EINVAL;
}

static int udma_send_req_to_mue(struct udma_dev *dev, union ubcore_tp_handle *tp_handle)
{
	uint32_t data_len = (uint32_t)sizeof(struct udma_ue_tp_info);
	struct udma_ue_tp_info *data;
	struct ubcore_req *req_msg;
	int ret;

	req_msg = kzalloc(sizeof(*req_msg) + data_len, GFP_KERNEL);
	if (!req_msg)
		return -ENOMEM;

	data = (struct udma_ue_tp_info *)req_msg->data;
	data->start_tpn = tp_handle->bs.tpn_start;
	data->tp_cnt = tp_handle->bs.tp_cnt;
	req_msg->len = data_len;
	ret = send_req_to_mue(dev, req_msg, UDMA_CMD_NOTIFY_MUE_SAVE_TP);
	if (ret)
		dev_err(dev->dev, "fail to notify mue save tp, ret %d.\n", ret);

	kfree(req_msg);

	return ret;
}

static int udma_ctrlq_store_one_tpid(struct udma_dev *udev, struct xarray *ctrlq_tpid_table,
				     struct udma_ctrlq_tpid *tpid)
{
	struct udma_ctrlq_tpid *tpid_entity;
	int ret;

	if (debug_switch)
		dev_info(udev->dev, "udma ctrlq store one tpid start. tpid %u\n", tpid->tpid);

	if (xa_load(ctrlq_tpid_table, tpid->tpid)) {
		dev_warn(udev->dev,
			 "the tpid already exists in ctrlq tpid table, tpid = %u.\n",
			 tpid->tpid);
		return 0;
	}

	tpid_entity = kzalloc(sizeof(*tpid_entity), GFP_KERNEL);
	if (!tpid_entity)
		return -ENOMEM;

	memcpy(tpid_entity, tpid, sizeof(*tpid));

	ret = xa_err(xa_store(ctrlq_tpid_table, tpid->tpid, tpid_entity, GFP_KERNEL));
	if (ret) {
		dev_err(udev->dev,
			"store tpid entity failed, ret = %d, tpid = %u.\n",
			ret, tpid->tpid);
		kfree(tpid_entity);
	}

	return ret;
}

static void udma_ctrlq_erase_one_tpid(struct xarray *ctrlq_tpid_table,
				      uint32_t tpid)
{
	struct udma_ctrlq_tpid *tpid_entity;

	xa_lock(ctrlq_tpid_table);
	tpid_entity = xa_load(ctrlq_tpid_table, tpid);
	if (!tpid_entity) {
		xa_unlock(ctrlq_tpid_table);
		return;
	}
	__xa_erase(ctrlq_tpid_table, tpid);
	kfree(tpid_entity);
	xa_unlock(ctrlq_tpid_table);
}

static int udma_ctrlq_get_tpid_list(struct udma_dev *udev,
				    struct udma_ctrlq_get_tp_list_req_data *tp_cfg_req,
				    struct ubcore_get_tp_cfg *tpid_cfg,
				    struct udma_ctrlq_tpid_list_rsp *tpid_list_resp)
{
	enum udma_ctrlq_trans_type trans_type;
	struct ubase_ctrlq_msg msg = {};
	int ret;

	if (!tpid_cfg->flag.bs.ctp) {
		if (udma_ctrlq_get_trans_type(udev, tpid_cfg->trans_mode, &trans_type) != 0) {
			dev_err(udev->dev, "udma get ctrlq trans_type failed, trans_mode = %d.\n",
				tpid_cfg->trans_mode);
			return -EINVAL;
		}

		tp_cfg_req->trans_type = (uint32_t)trans_type;
	} else {
		tp_cfg_req->trans_type = UDMA_CTRLQ_TRANS_TYPE_CTP;
	}

	udma_swap_endian(tpid_cfg->local_eid.raw, tp_cfg_req->seid,
			 UDMA_EID_SIZE);
	udma_swap_endian(tpid_cfg->peer_eid.raw, tp_cfg_req->deid,
			 UDMA_EID_SIZE);

	udma_ctrlq_set_tp_msg(&msg, (void *)tp_cfg_req, sizeof(*tp_cfg_req),
			      (void *)tpid_list_resp, sizeof(*tpid_list_resp));
	msg.opcode = UDMA_CMD_CTRLQ_GET_TP_LIST;

	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &msg);
	if (ret)
		dev_err(udev->dev, "ctrlq send msg failed, ret = %d.\n", ret);

	return ret;
}

static int udma_ctrlq_store_tpid_list(struct udma_dev *udev,
				      struct xarray *ctrlq_tpid_table,
				      struct udma_ctrlq_tpid_list_rsp *tpid_list_resp)
{
	int ret;
	int i;

	if (debug_switch)
		dev_info(udev->dev, "udma ctrlq store tpid list tp_list_cnt = %u.\n",
			 tpid_list_resp->tp_list_cnt);

	for (i = 0; i < (int)tpid_list_resp->tp_list_cnt; i++) {
		ret = udma_ctrlq_store_one_tpid(udev, ctrlq_tpid_table,
						&tpid_list_resp->tpid_list[i]);
		if (ret)
			goto err_store_one_tpid;
	}

	return 0;

err_store_one_tpid:
	for (i--; i >= 0; i--)
		udma_ctrlq_erase_one_tpid(ctrlq_tpid_table, tpid_list_resp->tpid_list[i].tpid);

	return ret;
}

int udma_get_tp_list(struct ubcore_device *dev, struct ubcore_get_tp_cfg *tpid_cfg,
		     uint32_t *tp_cnt, struct ubcore_tp_info *tp_list,
		     struct ubcore_udata *udata)
{
	struct udma_ctrlq_get_tp_list_req_data tp_cfg_req = {};
	struct udma_ctrlq_tpid_list_rsp tpid_list_resp = {};
	struct udma_dev *udev = to_udma_dev(dev);
	int ret;
	int i;

	if (!udata)
		tp_cfg_req.flag = UDMA_DEFAULT_PID;
	else
		tp_cfg_req.flag = (uint32_t)current->tgid & UDMA_PID_MASK;

	ret = udma_ctrlq_get_tpid_list(udev, &tp_cfg_req, tpid_cfg, &tpid_list_resp);
	if (ret) {
		dev_err(udev->dev, "udma ctrlq get tpid list failed, ret = %d.\n", ret);
		return ret;
	}

	if (tpid_list_resp.tp_list_cnt == 0 || tpid_list_resp.tp_list_cnt > *tp_cnt) {
		dev_err(udev->dev,
			"check tp list count failed, count = %u.\n",
			tpid_list_resp.tp_list_cnt);
		return -EINVAL;
	}

	for (i = 0; i < tpid_list_resp.tp_list_cnt; i++) {
		tp_list[i].tp_handle.bs.tpid = tpid_list_resp.tpid_list[i].tpid;
		tp_list[i].tp_handle.bs.tpn_start = tpid_list_resp.tpid_list[i].tpn_start;
		tp_list[i].tp_handle.bs.tp_cnt =
			tpid_list_resp.tpid_list[i].tpn_cnt & UDMA_TPN_CNT_MASK;
	}
	*tp_cnt = tpid_list_resp.tp_list_cnt;

	ret = udma_ctrlq_store_tpid_list(udev, &udev->ctrlq_tpid_table, &tpid_list_resp);
	if (ret)
		dev_err(udev->dev, "udma ctrlq store list failed, ret = %d.\n", ret);

	return ret;
}

void udma_ctrlq_destroy_tpid_list(struct udma_dev *dev, struct xarray *ctrlq_tpid_table,
				  bool is_need_flush)
{
	struct udma_ctrlq_tpid *tpid_entity = NULL;
	unsigned long tpid = 0;

	xa_lock(ctrlq_tpid_table);
	if (!xa_empty(ctrlq_tpid_table)) {
		xa_for_each(ctrlq_tpid_table, tpid, tpid_entity) {
			__xa_erase(ctrlq_tpid_table, tpid);
			kfree(tpid_entity);
		}
	}
	xa_unlock(ctrlq_tpid_table);
	xa_destroy(ctrlq_tpid_table);
}

static int udma_k_ctrlq_create_active_tp_msg(struct udma_dev *udev,
					     struct ubcore_active_tp_cfg *active_cfg,
					     uint32_t *tp_id)
{
	struct udma_ctrlq_active_tp_resp_data active_tp_resp = {};
	struct udma_ctrlq_active_tp_req_data active_tp_req = {};
	struct ubase_ctrlq_msg msg = {};
	int ret;

	active_tp_req.local_tp_id = active_cfg->tp_handle.bs.tpid;
	active_tp_req.local_tpn_cnt = active_cfg->tp_handle.bs.tp_cnt;
	active_tp_req.local_tpn_start = active_cfg->tp_handle.bs.tpn_start;
	active_tp_req.local_psn = active_cfg->tp_attr.tx_psn;

	active_tp_req.remote_tp_id = active_cfg->peer_tp_handle.bs.tpid;
	active_tp_req.remote_tpn_cnt = active_cfg->peer_tp_handle.bs.tp_cnt;
	active_tp_req.remote_tpn_start = active_cfg->peer_tp_handle.bs.tpn_start;
	active_tp_req.remote_psn = active_cfg->tp_attr.rx_psn;

	if (debug_switch)
		udma_dfx_ctx_print(udev, "udma create active tp msg info",
				   active_tp_req.local_tp_id,
				   sizeof(struct udma_ctrlq_active_tp_req_data) / sizeof(uint32_t),
				   (uint32_t *)&active_tp_req);

	msg.opcode = UDMA_CMD_CTRLQ_ACTIVE_TP;
	udma_ctrlq_set_tp_msg(&msg, (void *)&active_tp_req, sizeof(active_tp_req),
			      (void *)&active_tp_resp, sizeof(active_tp_resp));

	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &msg);
	if (ret)
		dev_err(udev->dev, "udma active tp send failed, ret = %d.\n", ret);

	*tp_id = active_tp_resp.local_tp_id;

	return ret;
}

int udma_ctrlq_set_active_tp_ex(struct udma_dev *dev,
				struct ubcore_active_tp_cfg *active_cfg)
{
	uint32_t tp_id = active_cfg->tp_handle.bs.tpid;
	int ret;

	ret = udma_k_ctrlq_create_active_tp_msg(dev, active_cfg, &tp_id);
	if (ret)
		return ret;

	active_cfg->tp_handle.bs.tpid = tp_id;

	if (dev->is_ue)
		(void)udma_send_req_to_mue(dev, &(active_cfg->tp_handle));

	return 0;
}

static int udma_k_ctrlq_deactive_tp(struct udma_dev *udev, union ubcore_tp_handle tp_handle,
				    struct ubcore_udata *udata)
{
#define UDMA_RSP_TP_MUL 2
	uint32_t tp_id = tp_handle.bs.tpid & UDMA_TPHANDLE_TPID_SHIFT;
	struct udma_ctrlq_deactive_tp_req_data deactive_tp_req = {};
	uint32_t tp_num = tp_handle.bs.tp_cnt;
	struct ubase_ctrlq_msg msg = {};
	int ret;

	if (tp_num) {
		ret = udma_close_ue_rx(udev, true, false, false, tp_num * UDMA_RSP_TP_MUL);
		if (ret) {
			dev_err(udev->dev, "close ue rx failed in deactivate tp.\n");
			return ret;
		}
	}

	msg.opcode = UDMA_CMD_CTRLQ_DEACTIVE_TP;
	deactive_tp_req.tp_id = tp_id;
	deactive_tp_req.tpn_cnt = tp_handle.bs.tp_cnt;
	deactive_tp_req.start_tpn = tp_handle.bs.tpn_start;
	if (!udata)
		deactive_tp_req.pid_flag = UDMA_DEFAULT_PID;
	else
		deactive_tp_req.pid_flag = (uint32_t)current->tgid & UDMA_PID_MASK;

	udma_ctrlq_set_tp_msg(&msg, (void *)&deactive_tp_req, sizeof(deactive_tp_req), NULL, 0);

	ret = ubase_ctrlq_send_msg(udev->comdev.adev, &msg);
	if (ret != -EAGAIN && ret) {
		dev_err(udev->dev, "deactivate tp send msg failed, tp_id = %u, ret = %d.\n",
			tp_id, ret);
		if (tp_num)
			udma_open_ue_rx(udev, true, false, false, tp_num * UDMA_RSP_TP_MUL);
		return ret;
	}

	udma_ctrlq_erase_one_tpid(&udev->ctrlq_tpid_table, tp_id);

	return (ret == -EAGAIN) ? 0 : ret;
}

int send_req_to_mue(struct udma_dev *udma_dev, struct ubcore_req *req, uint16_t opcode)
{
	struct udma_req_msg *req_msg;
	struct ubase_cmd_buf in;
	uint32_t msg_len;
	int ret;

	msg_len = sizeof(*req_msg) + req->len;
	req_msg = kzalloc(msg_len, GFP_KERNEL);
	if (!req_msg)
		return -ENOMEM;

	req_msg->resp_code = opcode;

	(void)memcpy(&req_msg->req, req, sizeof(*req));
	(void)memcpy(req_msg->req.data, req->data, req->len);
	udma_fill_buf(&in, UBASE_OPC_UE_TO_MUE, false, msg_len, req_msg);

	ret = ubase_cmd_send_in(udma_dev->comdev.adev, &in);
	if (ret)
		dev_err(udma_dev->dev,
			"send req msg cmd failed, ret is %d.\n", ret);

	kfree(req_msg);

	return ret;
}

int send_resp_to_ue(struct udma_dev *udma_dev, struct ubcore_resp *req_host,
		    uint8_t dst_ue_idx, uint16_t opcode)
{
	struct udma_resp_msg *udma_req;
	struct ubase_cmd_buf in;
	uint32_t msg_len;
	int ret;

	msg_len = sizeof(*udma_req) + req_host->len;
	udma_req = kzalloc(msg_len, GFP_KERNEL);
	if (!udma_req)
		return -ENOMEM;

	udma_req->dst_ue_idx = dst_ue_idx;
	udma_req->resp_code = opcode;

	(void)memcpy(&udma_req->resp, req_host, sizeof(*req_host));
	(void)memcpy(udma_req->resp.data, req_host->data, req_host->len);

	udma_fill_buf(&in, UBASE_OPC_MUE_TO_UE, false, msg_len, udma_req);

	ret = ubase_cmd_send_in(udma_dev->comdev.adev, &in);
	if (ret)
		dev_err(udma_dev->dev,
			"send resp msg cmd failed, ret is %d.\n", ret);

	kfree(udma_req);

	return ret;
}

int udma_active_tp(struct ubcore_device *dev, struct ubcore_active_tp_cfg *active_cfg)
{
	struct udma_dev *udma_dev = to_udma_dev(dev);
	int ret;

	if (debug_switch)
		udma_dfx_ctx_print(udma_dev, "udma active tp ex", active_cfg->tp_handle.bs.tpid,
				   sizeof(struct ubcore_active_tp_cfg) / sizeof(uint32_t),
				   (uint32_t *)active_cfg);
	ret = udma_ctrlq_set_active_tp_ex(udma_dev, active_cfg);
	if (ret)
		dev_err(udma_dev->dev, "Failed to set active tp msg, ret %d.\n", ret);

	return ret;
}

int udma_deactive_tp(struct ubcore_device *dev, union ubcore_tp_handle tp_handle,
		     struct ubcore_udata *udata)
{
	struct udma_dev *udma_dev = to_udma_dev(dev);

	if (debug_switch)
		dev_info(udma_dev->dev, "udma deactivate tp ex tp_id = %u\n", tp_handle.bs.tpid);

	return udma_k_ctrlq_deactive_tp(udma_dev, tp_handle, udata);
}
