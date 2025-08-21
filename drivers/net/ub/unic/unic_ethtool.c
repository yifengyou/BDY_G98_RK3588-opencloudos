// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include <linux/phy.h>
#include <ub/ubase/ubase_comm_cmd.h>

#include "unic.h"
#include "unic_dev.h"
#include "unic_hw.h"
#include "unic_netdev.h"
#include "unic_stats.h"
#include "unic_channel.h"
#include "unic_ethtool.h"

static u32 unic_get_link_status(struct net_device *netdev)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);

	unic_link_status_update(unic_dev);

	return unic_dev->sw_link_status;
}

static int unic_get_link_ksettings(struct net_device *netdev,
				   struct ethtool_link_ksettings *cmd)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);

	/* Ensure that the latest information is obtained. */
	unic_update_port_info(unic_dev);

	return 0;
}

static void unic_get_driver_info(struct net_device *netdev,
				 struct ethtool_drvinfo *drvinfo)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	struct ubase_caps *caps = ubase_get_dev_caps(unic_dev->comdev.adev);
	u32 fw_version = 0;

	if (!caps)
		unic_err(unic_dev,
			 "failed to get fw version, use default value 0.\n");
	else
		fw_version = caps->fw_version;

	strscpy(drvinfo->version, UNIC_MOD_VERSION, sizeof(drvinfo->version));
	strscpy(drvinfo->driver, unic_dev->comdev.adev->name,
		sizeof(drvinfo->driver));
	strscpy(drvinfo->bus_info, dev_name(unic_dev->comdev.adev->dev.parent),
		sizeof(drvinfo->bus_info));

	snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version), "%u.%u.%u.%u",
		 u32_get_bits(fw_version, UBASE_FW_VERSION_BYTE3_MASK),
		 u32_get_bits(fw_version, UBASE_FW_VERSION_BYTE2_MASK),
		 u32_get_bits(fw_version, UBASE_FW_VERSION_BYTE1_MASK),
		 u32_get_bits(fw_version, UBASE_FW_VERSION_BYTE0_MASK));
}

static int unic_get_fecparam(struct net_device *ndev,
			     struct ethtool_fecparam *fec)
{
	struct unic_dev *unic_dev = netdev_priv(ndev);
	struct unic_mac *mac = &unic_dev->hw.mac;

	if (!unic_dev_fec_supported(unic_dev))
		return -EOPNOTSUPP;

	fec->fec = mac->fec_ability;
	fec->active_fec = mac->fec_mode;

	return 0;
}

static int unic_set_fecparam(struct net_device *ndev,
			     struct ethtool_fecparam *fec)
{
	struct unic_dev *unic_dev = netdev_priv(ndev);
	struct unic_mac *mac = &unic_dev->hw.mac;
	u32 fec_mode;
	int ret;

	if (!unic_dev_fec_supported(unic_dev))
		return -EOPNOTSUPP;

	fec_mode = fec->fec;
	if (!(mac->fec_ability & fec_mode)) {
		unic_err(unic_dev,
			 "unsupported fec mode, fec mode = %u.\n", fec_mode);
		return -EINVAL;
	}

	if (mac->fec_mode == fec_mode)
		return 0;

	if (netif_msg_ifdown(unic_dev))
		unic_info(unic_dev, "set fec mode = %u.\n", fec_mode);

	ret = unic_set_fec_mode(unic_dev, fec_mode);
	if (ret)
		return ret;

	mac->user_fec_mode = fec_mode;

	return 0;
}

static int unic_get_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd,
			     struct kernel_ethtool_coalesce *kernel_coal,
			     struct netlink_ext_ack *extack)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	struct unic_coalesce *tx_coal = &unic_dev->channels.unic_coal.tx_coal;
	struct unic_coalesce *rx_coal = &unic_dev->channels.unic_coal.rx_coal;

	if (unic_resetting(netdev))
		return -EBUSY;

	cmd->tx_coalesce_usecs = tx_coal->int_gl;
	cmd->rx_coalesce_usecs = rx_coal->int_gl;

	cmd->tx_max_coalesced_frames = tx_coal->int_ql;
	cmd->rx_max_coalesced_frames = rx_coal->int_ql;

	return 0;
}

static int unic_check_gl_coalesce_para(struct net_device *netdev,
				       struct ethtool_coalesce *cmd)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	u32 rx_gl, tx_gl;

	if (cmd->rx_coalesce_usecs > unic_dev->caps.max_int_gl) {
		unic_err(unic_dev,
			 "invalid rx-usecs value, rx-usecs range is [0, %u].\n",
			 unic_dev->caps.max_int_gl);
		return -EINVAL;
	}

	if (cmd->tx_coalesce_usecs > unic_dev->caps.max_int_gl) {
		unic_err(unic_dev,
			 "invalid tx-usecs value, tx-usecs range is [0, %u].\n",
			 unic_dev->caps.max_int_gl);
		return -EINVAL;
	}

	rx_gl = unic_cqe_period_round_down(cmd->rx_coalesce_usecs);
	if (rx_gl != cmd->rx_coalesce_usecs) {
		unic_err(unic_dev,
			 "invalid rx_usecs(%u), because it must be power of 4.\n",
			 cmd->rx_coalesce_usecs);
		return -EINVAL;
	}

	tx_gl = unic_cqe_period_round_down(cmd->tx_coalesce_usecs);
	if (tx_gl != cmd->tx_coalesce_usecs) {
		unic_err(unic_dev,
			 "invalid tx_usecs(%u), because it must be power of 4.\n",
			 cmd->tx_coalesce_usecs);
		return -EINVAL;
	}

	return 0;
}

static int unic_check_ql_coalesce_para(struct net_device *netdev,
				       struct ethtool_coalesce *cmd)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);

	if ((cmd->tx_max_coalesced_frames || cmd->rx_max_coalesced_frames) &&
	    !unic_dev->caps.max_int_ql) {
		unic_err(unic_dev, "coalesced frames is not supported.\n");
		return -EOPNOTSUPP;
	}

	if (cmd->tx_max_coalesced_frames > unic_dev->caps.max_int_ql ||
	    cmd->rx_max_coalesced_frames > unic_dev->caps.max_int_ql) {
		unic_err(unic_dev,
			 "invalid coalesced frames value, range is [0, %u].\n",
			 unic_dev->caps.max_int_ql);
		return -ERANGE;
	}

	return 0;
}

static int
unic_check_coalesce_para(struct net_device *netdev,
			 struct ethtool_coalesce *cmd,
			 struct kernel_ethtool_coalesce *kernel_coal)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	int ret;

	if (cmd->use_adaptive_rx_coalesce || cmd->use_adaptive_tx_coalesce) {
		unic_err(unic_dev,
			 "not support to enable adaptive coalesce.\n");
		return -EINVAL;
	}

	ret = unic_check_gl_coalesce_para(netdev, cmd);
	if (ret) {
		unic_err(unic_dev,
			 "failed to check gl coalesce param, ret = %d.\n", ret);
		return ret;
	}

	ret = unic_check_ql_coalesce_para(netdev, cmd);
	if (ret)
		unic_err(unic_dev,
			 "failed to check ql coalesce param, ret = %d.\n", ret);

	return ret;
}

static int unic_set_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd,
			     struct kernel_ethtool_coalesce *kernel_coal,
			     struct netlink_ext_ack *extack)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	struct unic_coal_txrx *unic_coal = &unic_dev->channels.unic_coal;
	struct unic_coalesce *tx_coal = &unic_coal->tx_coal;
	struct unic_coalesce *rx_coal = &unic_coal->rx_coal;
	struct unic_coalesce old_tx_coal, old_rx_coal;
	int ret, ret1;

	if (test_bit(UNIC_STATE_DEACTIVATE, &unic_dev->state)) {
		unic_err(unic_dev,
			 "failed to set coalesce, due to dev deacitve.\n");
		return -EBUSY;
	}

	if (unic_resetting(netdev))
		return -EBUSY;

	ret = unic_check_coalesce_para(netdev, cmd, kernel_coal);
	if (ret)
		return ret;

	memcpy(&old_tx_coal, tx_coal, sizeof(struct unic_coalesce));
	memcpy(&old_rx_coal, rx_coal, sizeof(struct unic_coalesce));

	tx_coal->int_gl = cmd->tx_coalesce_usecs;
	rx_coal->int_gl = cmd->rx_coalesce_usecs;

	tx_coal->int_ql = cmd->tx_max_coalesced_frames;
	rx_coal->int_ql = cmd->rx_max_coalesced_frames;

	unic_net_stop_no_link_change(netdev);
	unic_uninit_channels(unic_dev);

	ret = unic_init_channels(unic_dev, unic_dev->channels.num);
	if (ret) {
		netdev_err(netdev, "failed to init channels, ret = %d.\n", ret);
		memcpy(tx_coal, &old_tx_coal, sizeof(struct unic_coalesce));
		memcpy(rx_coal, &old_rx_coal, sizeof(struct unic_coalesce));
		ret1 = unic_init_channels(unic_dev, unic_dev->channels.num);
		if (ret1) {
			unic_err(unic_dev,
				 "failed to recover old channels, ret = %d.\n",
				 ret1);
			return ret;
		}
	}

	ret1 = unic_net_open_no_link_change(netdev);
	if (ret1)
		unic_err(unic_dev, "failed to set net open, ret = %d.\n", ret1);

	return ret;
}

#define UNIC_ETHTOOL_RING	(ETHTOOL_RING_USE_RX_BUF_LEN | \
				 ETHTOOL_RING_USE_TX_PUSH)
#define UNIC_ETHTOOL_COALESCE	(ETHTOOL_COALESCE_USECS | \
				 ETHTOOL_COALESCE_USE_ADAPTIVE | \
				 ETHTOOL_COALESCE_MAX_FRAMES)

static const struct ethtool_ops unic_ethtool_ops = {
	.supported_ring_params = UNIC_ETHTOOL_RING,
	.cap_link_lanes_supported = true,
	.supported_coalesce_params = UNIC_ETHTOOL_COALESCE,
	.get_link = unic_get_link_status,
	.get_link_ksettings = unic_get_link_ksettings,
	.get_drvinfo = unic_get_driver_info,
	.get_regs_len = unic_get_regs_len,
	.get_regs = unic_get_regs,
	.get_channels = unic_get_channels,
	.set_channels = unic_set_channels,
	.get_ringparam = unic_get_channels_param,
	.set_ringparam = unic_set_channels_param,
	.get_fecparam = unic_get_fecparam,
	.set_fecparam = unic_set_fecparam,
	.get_fec_stats = unic_get_fec_stats,
	.get_coalesce = unic_get_coalesce,
	.set_coalesce = unic_set_coalesce,
};

void unic_set_ethtool_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &unic_ethtool_ops;
}
