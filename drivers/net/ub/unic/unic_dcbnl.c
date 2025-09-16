// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include <linux/dcbnl.h>

#include "unic_hw.h"
#include "unic_netdev.h"
#include "unic_dcbnl.h"

static int unic_dscp_prio_check(struct net_device *netdev, struct dcb_app *app)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);

	if (!unic_dev_ets_supported(unic_dev))
		return -EOPNOTSUPP;

	if (netif_running(netdev)) {
		unic_err(unic_dev,
			 "failed to set dscp-prio, due to network interface is up, pls down it first and try again.\n");
		return -EBUSY;
	}

	if (app->selector != IEEE_8021QAZ_APP_SEL_DSCP ||
	    app->protocol >= UBASE_MAX_DSCP ||
	    app->priority >= UNIC_MAX_PRIO_NUM)
		return -EINVAL;

	if (unic_resetting(netdev))
		return -EBUSY;

	return 0;
}

static int unic_set_app(struct net_device *netdev, struct dcb_app *app,
			struct unic_dev *unic_dev, struct unic_vl *vl)
{
	struct dcb_app old_app;
	int ret;

	unic_info(unic_dev, "setapp dscp = %u, priority = %u.\n",
		  app->protocol, app->priority);

	ret = dcb_ieee_setapp(netdev, app);
	if (ret) {
		unic_err(unic_dev, "failed to add app, ret = %d.\n", ret);
		return ret;
	}

	old_app.selector = IEEE_8021QAZ_APP_SEL_DSCP;
	old_app.protocol = app->protocol;
	old_app.priority = vl->dscp_prio[app->protocol];

	vl->dscp_prio[app->protocol] = app->priority;
	ret = unic_set_vl_map(unic_dev, vl->dscp_prio, vl->prio_vl,
			      UNIC_DSCP_VL_MAP);
	if (ret) {
		vl->dscp_prio[app->protocol] = old_app.priority;
		dcb_ieee_delapp(netdev, app);
		return ret;
	}

	if (old_app.priority == UNIC_INVALID_PRIORITY) {
		vl->dscp_app_cnt++;
	} else {
		ret = dcb_ieee_delapp(netdev, &old_app);
		if (ret)
			unic_err(unic_dev,
				 "failed to delete old app, ret = %d.\n", ret);
	}

	return ret;
}

static int unic_dcbnl_ieee_setapp(struct net_device *netdev,
				  struct dcb_app *app)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	struct unic_vl *vl = &unic_dev->channels.vl;
	int ret;

	ret = unic_dscp_prio_check(netdev, app);
	if (ret) {
		unic_err(unic_dev, "failed to set dscp-prio, ret = %d\n", ret);
		return ret;
	}

	/* dscp-prio already set */
	if (vl->dscp_prio[app->protocol] == app->priority)
		return 0;

	return unic_set_app(netdev, app, unic_dev, vl);
}

static int unic_del_app(struct net_device *netdev, struct dcb_app *app,
			struct unic_dev *unic_dev, struct unic_vl *vl)
{
	u8 map_type = UNIC_DSCP_VL_MAP;
	int ret;

	unic_info(unic_dev, "delapp dscp = %u, priority = %u\n",
		  app->protocol, app->priority);

	ret = dcb_ieee_delapp(netdev, app);
	if (ret)
		return ret;

	if (vl->dscp_app_cnt <= 1)
		map_type = UNIC_PRIO_VL_MAP;

	vl->dscp_prio[app->protocol] = UNIC_INVALID_PRIORITY;
	ret = unic_set_vl_map(unic_dev, vl->dscp_prio, vl->prio_vl,
			      map_type);
	if (ret) {
		vl->dscp_prio[app->protocol] = app->priority;
		dcb_ieee_setapp(netdev, app);
		return ret;
	}

	if (vl->dscp_app_cnt)
		vl->dscp_app_cnt--;

	return 0;
}

static int unic_dcbnl_ieee_delapp(struct net_device *netdev,
				  struct dcb_app *app)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);
	struct unic_vl *vl = &unic_dev->channels.vl;
	int ret;

	ret = unic_dscp_prio_check(netdev, app);
	if (ret) {
		unic_err(unic_dev, "failed to del dscp-prio, ret = %d.\n", ret);
		return ret;
	}

	if (app->priority != vl->dscp_prio[app->protocol]) {
		unic_err(unic_dev, "failed to del no match dscp-prio.\n");
		return -EINVAL;
	}

	return unic_del_app(netdev, app, unic_dev, vl);
}

static const struct dcbnl_rtnl_ops unic_dcbnl_ops = {
	.ieee_setapp = unic_dcbnl_ieee_setapp,
	.ieee_delapp = unic_dcbnl_ieee_delapp,
};

void unic_set_dcbnl_ops(struct net_device *netdev)
{
	struct unic_dev *unic_dev = netdev_priv(netdev);

	if (!unic_dev_ets_supported(unic_dev))
		return;

	netdev->dcbnl_ops = &unic_dcbnl_ops;

	unic_dev->dcbx_cap = DCB_CAP_DCBX_VER_IEEE | DCB_CAP_DCBX_HOST;
}
