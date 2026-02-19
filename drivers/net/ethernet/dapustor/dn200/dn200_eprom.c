// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024, DapuStor Corporation.
 *
 * Author: Wang Peixiang <peixiang@dapustor.com>
 *
 * Update firmware for DN200
 */

#include <linux/firmware.h>
#include "dn200.h"
#include "dn200_cfg.h"
#include "dn200_eprom.h"

static int dn200_nvme_update_fw_for_others(struct dn200_priv *priv, const char *fw, size_t fw_size)
{
	int ret;
	u32 flag = 0;

	if (priv->plat_ex->vf_flag) {
		dev_err(priv->device, "[loading fw]stop update fw, please wait vf's operation first.\n");
		return -EOPNOTSUPP;
	}

	if (PRIV_IS_VF(priv)) {
		dev_err(priv->device, "[loading fw]stop update fw, please wait vf's operation first.\n");
		return -EOPNOTSUPP;
	}

	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, &flag);
	if (flag) {
		dev_warn(priv->device, "[loading fw]please wait update fw finish, then updage fw.\n");
		return -EOPNOTSUPP;
	}

	DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 1);
	ret = dn200_nvme_fw_load(&priv->plat_ex->ctrl, fw, fw_size);
	if (ret) {
		dev_err(priv->device, "[loading fw]download fw to controller fail.\n");
		return ret;
	}

	ret = dn200_nvme_fw_commit(&priv->plat_ex->ctrl);

	if (ret)
		dev_err(priv->device, "[loading fw]dn200 load fw fail, sf %#x.\n", ret);
	else
		dev_info(priv->device, "[loading fw]dn200 load fw ok.\n");

	DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 0);

	return ret;

}

static int dn200_nvme_update_fw_for_raid(struct dn200_priv *priv, const char *fw, size_t fw_size)
{
	int ret;
	unsigned long in_time_start;
	u32 flag = 0;

	if (priv->plat_ex->vf_flag) {
		dev_err(priv->device, "[loading fw]stop update fw, please wait vf's operation first.\n");
		return -EOPNOTSUPP;
	}

	if (PRIV_IS_VF(priv)) {
		dev_err(priv->device, "[loading fw]stop update fw, please wait vf's operation first.\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state)) {
		dev_warn(priv->device, "[loading fw]please wait update fw finish, then updage fw.\n");
		return -EOPNOTSUPP;
	}

	DN200_GET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, &flag);
	if (flag) {
		dev_warn(priv->device, "[loading fw]please wait update fw finish, then updage fw.\n");
		return -EOPNOTSUPP;
	}
	DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 1);

	priv->flag_upgrade = true;
	priv->update_fail = false;
	set_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);

	ret = dn200_nvme_fw_load(&priv->plat_ex->ctrl, fw, fw_size);
	if (ret) {
		DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 0);
		dev_err(priv->device, "[loading fw]download fw to controller fail.\n");
		clear_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state);
		return ret;
	}

	ret = dn200_nvme_fw_commit(&priv->plat_ex->ctrl);
	if (ret) /*for hot upgrade, it must success*/
		dev_err(priv->device, "[loading fw]dn200 load fw fail, sf %#x.\n", ret);
	else
		dev_info(priv->device, "[loading fw]dn200 loading fw, please wait a moment when hot upgrade finish.\n");

	mod_timer(&priv->upgrade_timer, jiffies + msecs_to_jiffies(500)); /*start timer*/
	in_time_start = jiffies;
	while (true) {
		if (!test_bit(ADMIN_UP_GRADE_FLAG, &priv->plat_ex->ctrl.admin_state)) {
			if (priv->update_fail) {
				dev_err(priv->device, "fw upgrade fail\n");
				ret = -EIO;
			} else {
				dev_info(priv->device, "fw upgrade success\n");
			}
			break;
		}
		if (time_after(jiffies, in_time_start + msecs_to_jiffies(40000))) {
			dev_info(priv->device, "fw upgrade exceed time\n");
			ret = -EIO;
			break;
		}
		usleep_range(100000, 200000);
	}
	priv->flag_upgrade = false;
	DN200_SET_LRAM_UPGRADE_MEMBER(priv->hw, upgrade_flag, 0);
	return ret;
}

static int dn200_check_fw_invalid(struct dn200_priv *priv, const char *fw, size_t fw_size)
{
	struct header_file_t *head;

	if (fw_size < sizeof(struct header_file_t) || !fw)
		return -EACCES;

	head = (struct header_file_t *)fw;

	if ((head->magic_num != HEADER_FILE_MAGIC || head->plat_id != 3) ||
		(priv->plat_ex->pdev->device == DN200_DEV_ID_SFP_10G_2P_SRIOV_PF &&
			!(head->board_id & (1 << board_type_xgmac_only))) ||
		(priv->plat_ex->pdev->device == DN200_DEV_ID_COPP_1G_4P_NVME_PUREPF &&
			!(head->board_id & (1 << board_type_gmac_combo))) ||
		(priv->plat_ex->pdev->device == DN200_DEV_ID_SFP_10G_2P_RAID_SRIOV_PF &&
			!(head->board_id & (1 << board_type_xgmac_combo)))) {
		dev_err(priv->device, "[loading fw]the firmware is invalid.\n");
		return -EINVAL;
	}

	return 0;
}

int dn200_load_firmware(struct net_device *netdev, struct ethtool_flash *efl)
{
	const struct firmware *fw;
	int status;
	struct dn200_priv *priv = netdev_priv(netdev);

	status = request_firmware(&fw, efl->data, priv->device);
	if (status) {
		dev_err(priv->device, "[loading fw]dn200 get fw fail.\n");
		goto fw_fail;
	}

	dev_info(priv->device, "[loading fw]dn200 get fw:%s size 0x%lx.\n",
		 efl->data, fw->size);

	if (dn200_check_fw_invalid(priv, fw->data, fw->size)) {
		status = -EINVAL;
		goto fw_fail;
	}

	if (priv->plat_ex->upgrade_with_flowing) {
		status = dn200_nvme_update_fw_for_raid(priv, fw->data, fw->size);
		if (status)
			status = -EINVAL;
	} else {
		status = dn200_nvme_update_fw_for_others(priv, fw->data, fw->size);
		if (status)
			status = -EINVAL;
	}


fw_fail:
	release_firmware(fw);
	return status;
}

static int dn200_get_product_info_from_eprom(struct dn200_priv *priv,
				      struct product_info_t *info)
{
	int ret;

	if (!priv || !info)
		return -EINVAL;

	ret = dn200_nvme_product_info_get(&priv->plat_ex->ctrl, info, sizeof(struct product_info_t));
	if (ret != 0)
		return ret;

	if (info->magic_num != PRODUCT_MAGIC_NUMBER) {
		dev_dbg(priv->device, "product info is invalid[%#x].\n", info->magic_num);
		return -EINVAL;
	}

	return ret;
}

void dn200_get_mac_from_firmware(struct dn200_priv *priv, struct dn200_resources *res)
{
	struct product_info_t info;
	int ret = dn200_get_product_info_from_eprom(priv, &info);

	if (!ret && priv->plat_ex->funcid < ARRAY_SIZE(info.mac_addr)) {
		memcpy(res->mac, info.mac_addr[priv->plat_ex->funcid].addr,
		       ETH_ALEN);
		return;
	}

	get_random_bytes(&res->mac, ETH_ALEN);
	/* Extract the MAC address from the high and low words */
	res->mac[0] = 0xd8;
	res->mac[1] = 0xbc;
	res->mac[2] = 0x59;
}
