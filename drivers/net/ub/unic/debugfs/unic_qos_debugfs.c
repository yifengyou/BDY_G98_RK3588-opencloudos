// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include "unic_dcbnl.h"
#include "unic_debugfs.h"
#include "unic_hw.h"
#include "unic_qos_debugfs.h"

int unic_dbg_dump_vl_queue(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_vl *vl = &unic_dev->channels.vl;
	u8 i;

	seq_puts(s, "VL_ID  Q_OFFSET    Q_COUNT\n");

	for (i = 0; i < unic_dev->channels.rss_vl_num; i++) {
		seq_printf(s, "%-7d", i);
		seq_printf(s, "%-12u", vl->queue_offset[i]);
		seq_printf(s, "%-11u\n", vl->queue_count[i]);
	}

	return 0;
}

static void unic_dump_dscp_vl_map(struct unic_dev *unic_dev,
				  struct seq_file *s, u8 dscp, u8 hw_vl)
{
	struct auxiliary_device *adev = unic_dev->comdev.adev;
	struct ubase_adev_qos *qos = ubase_get_adev_qos(adev);
	struct unic_vl *vl = &unic_dev->channels.vl;
	u8 prio, sw_vl = 0;

	prio = vl->dscp_prio[dscp];
	if (prio == UNIC_INVALID_PRIORITY ||
	    vl->prio_vl[prio] >= unic_dev->channels.rss_vl_num)
		sw_vl = qos->nic_vl[0];
	else
		sw_vl = qos->nic_vl[vl->prio_vl[prio]];

	seq_printf(s, "%-6u", dscp);
	seq_printf(s, "%-7u", sw_vl);

	if (!unic_dev_ubl_supported(unic_dev) &&
	    unic_dev_ets_supported(unic_dev))
		seq_printf(s, "%-7u", hw_vl);
	else
		seq_puts(s, "--");

	seq_puts(s, "\n");
}

int unic_dbg_dump_dscp_vl_map(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_config_vl_map_cmd resp = {0};
	int ret;
	u8 i;

	if (__unic_resetting(unic_dev))
		return -EBUSY;

	if (!unic_dev_ubl_supported(unic_dev) &&
	    unic_dev_ets_supported(unic_dev)) {
		ret = unic_query_vl_map(unic_dev, &resp);
		if (ret)
			return ret;
	}

	seq_puts(s, "DSCP  SW_VL  HW_VL\n");

	for (i = 0; i < UBASE_MAX_DSCP; i++)
		unic_dump_dscp_vl_map(unic_dev, s, i, resp.dscp_vl[i]);

	return 0;
}

static void unic_dump_prio_vl_map(struct unic_dev *unic_dev,
				  struct seq_file *s, u8 prio, u8 hw_vl)
{
	struct auxiliary_device *adev = unic_dev->comdev.adev;
	struct ubase_adev_qos *qos = ubase_get_adev_qos(adev);
	struct unic_vl *vl = &unic_dev->channels.vl;
	u8 sw_vl = 0;

	if (vl->prio_vl[prio] >= unic_dev->channels.rss_vl_num)
		sw_vl = qos->nic_vl[0];
	else
		sw_vl = qos->nic_vl[vl->prio_vl[prio]];

	seq_printf(s, "%-6u", prio);
	seq_printf(s, "%-7u", sw_vl);

	if (!unic_dev_ubl_supported(unic_dev) &&
	    unic_dev_ets_supported(unic_dev))
		seq_printf(s, "%-7u", hw_vl);
	else
		seq_puts(s, "--");

	seq_puts(s, "\n");
}

int unic_dbg_dump_prio_vl_map(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_config_vl_map_cmd resp = {0};
	int ret;
	u8 i;

	if (__unic_resetting(unic_dev))
		return -EBUSY;

	if (!unic_dev_ubl_supported(unic_dev) &&
	    unic_dev_ets_supported(unic_dev)) {
		ret = unic_query_vl_map(unic_dev, &resp);
		if (ret)
			return ret;
	}

	seq_puts(s, "PRIO  SW_VL  HW_VL\n");

	for (i = 0; i < UNIC_MAX_PRIO_NUM; i++)
		unic_dump_prio_vl_map(unic_dev, s, i, resp.prio_vl[i]);

	return 0;
}

int unic_dbg_dump_dscp_prio(struct seq_file *s, void *data)
{
	struct unic_dev *unic_dev = dev_get_drvdata(s->private);
	struct unic_vl *vl = &unic_dev->channels.vl;
	u16 i;

	if (__unic_resetting(unic_dev))
		return -EBUSY;

	seq_puts(s, "DSCP  PRIO\n");

	for (i = 0; i < UBASE_MAX_DSCP; i++) {
		seq_printf(s, "%-6u", i);
		seq_printf(s, "%-7u\n", vl->dscp_prio[i]);
	}

	return 0;
}
