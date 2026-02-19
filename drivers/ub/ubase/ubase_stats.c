// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include "ubase_cmd.h"
#include "ubase_stats.h"

static int ubase_update_mac_stats(struct ubase_dev *udev, u16 port_id, u64 *data,
				  u16 size, bool is_accumulate)
{
	u16 mac_stats_num = udev->caps.dev_caps.mac_stats_num;
	struct ubase_query_mac_stats_cmd *cmd;
	struct ubase_cmd_buf in, out;
	u16 i, cmd_size;
	int ret;

	if (!ubase_dev_mac_stats_supported(udev)) {
		ubase_err(udev, "not support get mac stats.\n");
		return -EOPNOTSUPP;
	}

	cmd_size = mac_stats_num * sizeof(u64) + sizeof(*cmd);
	cmd = kzalloc(cmd_size, GFP_KERNEL);
	if (!cmd) {
		ubase_err(udev, "failed to alloc cmdq out_regs.\n");
		return -ENOMEM;
	}

	cmd->port_id = cpu_to_le16(port_id);
	ubase_fill_inout_buf(&in, UBASE_OPC_STATS_MAC_ALL, true, cmd_size, cmd);
	ubase_fill_inout_buf(&out, UBASE_OPC_STATS_MAC_ALL, true, cmd_size, cmd);
	ret = __ubase_cmd_send_inout(udev, &in, &out);
	if (ret) {
		ubase_err(udev, "failed to get mac stats, ret = %d.\n", ret);
		goto out_send_cmd_fail;
	}

	mac_stats_num = min_t(u16, size, mac_stats_num);
	if (is_accumulate)
		for (i = 0; i < mac_stats_num; i++)
			*data++ += le64_to_cpu(cmd->stats_val[i]);
	else
		for (i = 0; i < mac_stats_num; i++)
			*data++ = le64_to_cpu(cmd->stats_val[i]);

out_send_cmd_fail:
	kfree(cmd);

	return ret;
}

/**
 * ubase_clear_eth_port_stats() - clear eth port stats
 * @adev: auxiliary device
 *
 * The function is used to clear eth port stats.
 *
 * Context: Process context. Takes and releases <lock>, BH-safe. Sleep.
 * Return: 0 on success, negative error code otherwise
 */
void ubase_clear_eth_port_stats(struct auxiliary_device *adev)
{
	struct ubase_eth_mac_stats *eth_stats;
	struct ubase_dev *udev;

	if (!adev)
		return;

	udev = __ubase_get_udev_by_adev(adev);
	eth_stats = &udev->stats.eth_stats;
	if (ubase_dev_eth_mac_supported(udev)) {
		mutex_lock(&udev->stats.stats_lock);
		memset(eth_stats, 0, sizeof(*eth_stats));
		mutex_unlock(&udev->stats.stats_lock);
	}
}
EXPORT_SYMBOL(ubase_clear_eth_port_stats);

/**
 * ubase_get_ub_port_stats() - get ub port stats
 * @adev: auxiliary device
 * @port_id: port id
 * @data: ub date link layer stats
 *
 * The function is used to get ub port stats.
 *
 * Context: Process context. Takes and releases <lock>, BH-safe. Sleep.
 * Return: 0 on success, negative error code otherwise
 */
int ubase_get_ub_port_stats(struct auxiliary_device *adev, u16 port_id,
			    struct ubase_ub_dl_stats *data)
{
	struct ubase_dev *udev;

	if (!adev || !data)
		return -EINVAL;

	udev = __ubase_get_udev_by_adev(adev);

	return ubase_update_mac_stats(udev, port_id, (u64 *)data,
				      sizeof(*data) / sizeof(u64), false);
}
EXPORT_SYMBOL(ubase_get_ub_port_stats);

int __ubase_get_eth_port_stats(struct ubase_dev *udev,
			       struct ubase_eth_mac_stats *data)
{
	struct ubase_eth_mac_stats *eth_stats = &udev->stats.eth_stats;
	u32 stats_num = sizeof(*eth_stats) / sizeof(u64);
	int ret;

	mutex_lock(&udev->stats.stats_lock);
	ret = ubase_update_mac_stats(udev, udev->caps.dev_caps.io_port_logic_id,
				     (u64 *)eth_stats, stats_num, true);
	if (ret) {
		mutex_unlock(&udev->stats.stats_lock);
		return ret;
	}

	memcpy(data, &udev->stats.eth_stats, sizeof(*data));
	mutex_unlock(&udev->stats.stats_lock);

	return 0;
}

int ubase_get_eth_port_stats(struct auxiliary_device *adev,
			     struct ubase_eth_mac_stats *data)
{
	struct ubase_dev *udev;

	if (!adev || !data)
		return -EINVAL;

	udev = __ubase_get_udev_by_adev(adev);

	return __ubase_get_eth_port_stats(udev, data);
}
EXPORT_SYMBOL(ubase_get_eth_port_stats);

void ubase_update_activate_stats(struct ubase_dev *udev, bool activate,
				 int result)
{
	struct ubase_activate_dev_stats *record = &udev->stats.activate_record;
	u64 idx, total;

	mutex_lock(&record->lock);

	if (activate)
		record->act_cnt++;
	else
		record->deact_cnt++;

	total = record->act_cnt + record->deact_cnt;
	idx = (total - 1) % UBASE_ACT_STAT_MAX_NUM;
	record->stats[idx].activate = activate;
	record->stats[idx].time = ktime_get_real_seconds();
	record->stats[idx].result = result;

	mutex_unlock(&record->lock);
}

int ubase_update_eth_stats_trylock(struct ubase_dev *udev)
{
	struct ubase_eth_mac_stats *eth_stats = &udev->stats.eth_stats;
	u32 stats_num = sizeof(*eth_stats) / sizeof(u64);
	int ret;

	if (!mutex_trylock(&udev->stats.stats_lock))
		return 0;

	ret = ubase_update_mac_stats(udev, udev->caps.dev_caps.io_port_logic_id,
				     (u64 *)eth_stats, stats_num, true);
	mutex_unlock(&udev->stats.stats_lock);

	return ret;
}
