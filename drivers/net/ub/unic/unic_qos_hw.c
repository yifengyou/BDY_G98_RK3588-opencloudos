// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#define dev_fmt(fmt) "unic: (pid %d) " fmt, current->pid

#include <ub/ubase/ubase_comm_qos.h>

#include "unic_hw.h"
#include "unic_qos_hw.h"

int unic_set_hw_vl_map(struct unic_dev *unic_dev, u8 *dscp_vl, u8 *prio_vl,
		       u8 map_type)
{
	struct auxiliary_device *adev = unic_dev->comdev.adev;
	struct unic_config_vl_map_cmd req = {0};
	struct ubase_cmd_buf in;
	int ret;

	memcpy(req.dscp_vl, dscp_vl, sizeof(req.dscp_vl));
	memcpy(req.prio_vl, prio_vl, sizeof(req.prio_vl));
	req.map_type = map_type;
	ubase_fill_inout_buf(&in, UBASE_OPC_CFG_VL_MAP, false,
			     sizeof(req), &req);
	ret = ubase_cmd_send_in(adev, &in);
	if (ret)
		dev_err(adev->dev.parent, "failed to set vl map. ret = %d.\n", ret);

	return ret;
}

int unic_query_vl_map(struct unic_dev *unic_dev,
		      struct unic_config_vl_map_cmd *resp)
{
	struct unic_config_vl_map_cmd req = {0};
	struct ubase_cmd_buf in, out;
	int ret;

	ubase_fill_inout_buf(&in, UBASE_OPC_CFG_VL_MAP, true, sizeof(req),
			     &req);
	ubase_fill_inout_buf(&out, UBASE_OPC_CFG_VL_MAP, false, sizeof(*resp),
			     resp);
	ret = ubase_cmd_send_inout(unic_dev->comdev.adev, &in, &out);
	if (ret)
		unic_err(unic_dev, "failed to query vl map, ret = %d.\n", ret);

	return ret;
}

/* vl_maxrate: byte per second */
int unic_config_vl_rate_limit(struct unic_dev *unic_dev, u64 *vl_maxrate,
			      u16 vl_bitmap)
{
	struct auxiliary_device *adev = unic_dev->comdev.adev;
	struct ubase_caps *caps = ubase_get_dev_caps(adev);
	u32 max_speed = max(unic_dev->channels.vl.maxrate,
			    unic_dev->hw.mac.max_speed);
	u32 rate[UBASE_MAX_VL_NUM] = {0};
	u32 vl_rate;
	int ret;
	u8 i;

	for (i = 0; i < caps->vl_num; i++) {
		vl_maxrate[i] = vl_maxrate[i] ? vl_maxrate[i] :
						(u64)max_speed * UNIC_MBYTE_PER_SEND;
		vl_rate = vl_maxrate[i] / UNIC_MBYTE_PER_SEND;
		rate[caps->req_vl[i]] = vl_rate;
		rate[caps->resp_vl[i]] = vl_rate;
	}

	ret = ubase_config_tm_vl_rate_limit(adev, vl_bitmap, rate);
	if (ret && ret != -EPERM)
		dev_err(adev->dev.parent,
			"failed to config vl rate limit, ret = %d.\n", ret);

	return ret;
}

int unic_mac_pause_en_cfg(struct unic_dev *unic_dev, u32 tx_pause, u32 rx_pause)
{
	struct unic_cfg_mac_pause_cmd req = {0};
	struct ubase_cmd_buf in;
	int ret;

	req.tx_en = cpu_to_le32(tx_pause);
	req.rx_en = cpu_to_le32(rx_pause);

	ubase_fill_inout_buf(&in, UBASE_OPC_CFG_MAC_PAUSE_EN, false,
			     sizeof(req), &req);
	ret = ubase_cmd_send_in(unic_dev->comdev.adev, &in);
	if (ret)
		dev_err(unic_dev->comdev.adev->dev.parent,
			"failed to config pause on|off, ret = %d.\n", ret);

	return ret;
}

int unic_pfc_pause_cfg(struct unic_dev *unic_dev, u8 pfc_en)
{
#define	UNIC_PFC_TX_RX_ON	1
#define	UNIC_PFC_TX_RX_OFF	0

	struct unic_cfg_pfc_pause_cmd req = {0};
	struct ubase_cmd_buf in;
	int ret;

	req.pri_bitmap = pfc_en;

	if (pfc_en) {
		req.tx_enable = UNIC_PFC_TX_RX_ON;
		req.rx_enable = UNIC_PFC_TX_RX_ON;
	} else {
		req.tx_enable = UNIC_PFC_TX_RX_OFF;
		req.rx_enable = UNIC_PFC_TX_RX_OFF;
	}

	ubase_fill_inout_buf(&in, UBASE_OPC_CFG_PFC_PAUSE_EN, false, sizeof(req),
			     &req);

	ret = ubase_cmd_send_in(unic_dev->comdev.adev, &in);
	if (ret)
		dev_err(unic_dev->comdev.adev->dev.parent,
			"failed to config pfc enable, ret = %d.\n", ret);

	return ret;
}
