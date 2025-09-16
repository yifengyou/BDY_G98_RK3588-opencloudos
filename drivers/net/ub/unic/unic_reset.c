// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include <ub/ubase/ubase_comm_dev.h>

#include "unic_cmd.h"
#include "unic_dev.h"
#include "unic_hw.h"
#include "unic_netdev.h"
#include "unic_rack_ip.h"
#include "unic_reset.h"

static void unic_dev_suspend(struct unic_dev *unic_dev)
{
	unic_uninit_channels(unic_dev);
}

static void unic_reset_down(struct auxiliary_device *adev)
{
	struct unic_dev *priv = (struct unic_dev *)dev_get_drvdata(&adev->dev);
	struct net_device *netdev = priv->comdev.netdev;
	bool if_running;
	int ret;

	if (!test_bit(UNIC_STATE_INITED, &priv->state) ||
	    test_and_set_bit(UNIC_STATE_DISABLED, &priv->state)) {
		unic_warn(priv, "failed to reset unic, device is not ready.\n");
		return;
	}

	set_bit(UNIC_STATE_RESETTING, &priv->state);
	if_running = netif_running(netdev);

	unic_info(priv, "unic reset start.\n");

	unic_remove_period_task(priv);

	/* due to lack of cmdq when resetting, need to close promisc first,
	 * to prevent that concurrent deactivate event ubable to close promisc
	 * when resetting
	 */
	ret = unic_activate_promisc_mode(priv, false);
	if (ret)
		unic_warn(priv, "failed to close promisc, ret = %d.\n", ret);
	else
		set_bit(UNIC_VPORT_STATE_PROMISC_CHANGE, &priv->vport.state);

	rtnl_lock();
	ret = if_running ? unic_net_stop(netdev) : 0;
	rtnl_unlock();
	if (ret)
		unic_err(priv, "failed to stop unic net, ret = %d.\n", ret);
}

static void unic_reset_uninit(struct auxiliary_device *adev)
{
	struct unic_dev *priv = (struct unic_dev *)dev_get_drvdata(&adev->dev);

	if (!test_bit(UNIC_STATE_RESETTING, &priv->state))
		return;

	unic_dev_suspend(priv);
}

static int unic_dev_resume(struct unic_dev *unic_dev)
{
	int ret;

	ret = unic_init_channels(unic_dev, unic_dev->channels.num);
	if (ret)
		unic_err(unic_dev, "failed to init channels, ret = %d.\n", ret);

	return ret;
}

static void unic_reset_init(struct auxiliary_device *adev)
{
	struct unic_dev *priv = (struct unic_dev *)dev_get_drvdata(&adev->dev);
	struct net_device *netdev = priv->comdev.netdev;
	bool if_running;
	int ret;

	if (!test_bit(UNIC_STATE_RESETTING, &priv->state))
		return;

	ret = unic_dev_resume(priv);
	if (ret)
		goto err_unic_resume;

	unic_query_rack_ip(adev);
	unic_start_period_task(netdev);

	if_running = netif_running(netdev);
	clear_bit(UNIC_STATE_RESETTING, &priv->state);
	clear_bit(UNIC_STATE_DISABLED, &priv->state);
	rtnl_lock();
	ret = if_running ? unic_net_open(netdev) : 0;
	rtnl_unlock();
	if (ret)
		unic_err(priv, "failed to up net, ret = %d.\n", ret);

	unic_info(priv, "unic reset done.\n");

	return;

err_unic_resume:
	clear_bit(UNIC_STATE_RESETTING, &priv->state);
	clear_bit(UNIC_STATE_DISABLED, &priv->state);
}

void unic_reset_handler(struct auxiliary_device *adev,
			enum ubase_reset_stage stage)
{
	switch (stage) {
	case UBASE_RESET_STAGE_DOWN:
		unic_reset_down(adev);
		break;
	case UBASE_RESET_STAGE_UNINIT:
		unic_reset_uninit(adev);
		break;
	case UBASE_RESET_STAGE_INIT:
		unic_reset_init(adev);
		break;
	default:
		break;
	}
}
