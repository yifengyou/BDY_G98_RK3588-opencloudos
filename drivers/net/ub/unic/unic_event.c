// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#define dev_fmt(fmt) "unic: (pid %d) " fmt, current->pid

#include <net/rtnetlink.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <ub/ubase/ubase_comm_cmd.h>
#include <ub/ubase/ubase_comm_eq.h>
#include <ub/ubase/ubase_comm_hw.h>
#include <ub/ubase/ubase_comm_qos.h>
#include <ub/ubase/ubase_comm_ctrlq.h>

#include "unic_cmd.h"
#include "unic_crq.h"
#include "unic_dcbnl.h"
#include "unic_dev.h"
#include "unic_hw.h"
#include "unic_mac.h"
#include "unic_netdev.h"
#include "unic_qos_hw.h"
#include "unic_rack_ip.h"
#include "unic_reset.h"
#include "unic_event.h"

int unic_comp_handler(struct notifier_block *nb, unsigned long jfcn, void *data)
{
	struct auxiliary_device *adev = (struct auxiliary_device *)data;
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);
	struct unic_channels *channels = &unic_dev->channels;
	struct unic_cq *cq;
	u32 index;

	if (test_bit(UNIC_STATE_CHANNEL_INVALID, &unic_dev->state))
		return -EBUSY;

	index = jfcn < channels->num ? jfcn : jfcn - channels->num;
	if (index >= channels->num)
		return -EINVAL;

	if (jfcn > channels->num)
		cq = channels->c[index].rq->cq;
	else
		cq = channels->c[index].sq->cq;

	cq->event_cnt++;

	napi_schedule(&channels->c[index].napi);

	return 0;
}

static void unic_activate_event_process(struct unic_dev *unic_dev)
{
	struct unic_act_info *act_info = &unic_dev->act_info;
	struct net_device *netdev = unic_dev->comdev.netdev;
	int ret;

	if (test_bit(UNIC_STATE_DISABLED, &unic_dev->state)) {
		unic_err(unic_dev,
			 "failed to process activate event, device is not ready.\n");
		return;
	}

	rtnl_lock();

	if (!test_bit(UNIC_STATE_DEACTIVATE, &unic_dev->state))
		goto unlock;

	/* if network interface has already been stopped,
	 * no need to open by activate event
	 */
	if (test_bit(UNIC_STATE_DOWN, &unic_dev->state))
		goto out;

	ret = unic_net_open_no_link_change(netdev);
	if (ret)
		unic_warn(unic_dev, "failed to open net, ret = %d.\n", ret);

	ret = unic_activate_promisc_mode(unic_dev, true);
	if (ret)
		unic_warn(unic_dev, "failed to open promisc, ret = %d.\n", ret);
	else
		clear_bit(UNIC_VPORT_STATE_PROMISC_CHANGE, &unic_dev->vport.state);

	if (unic_dev_eth_mac_supported(unic_dev))
		unic_activate_mac_table(unic_dev);

out:
	mutex_lock(&act_info->mutex);
	act_info->deactivate = false;
	mutex_unlock(&act_info->mutex);
	clear_bit(UNIC_STATE_DEACTIVATE, &unic_dev->state);
unlock:
	rtnl_unlock();
}

static void unic_deactivate_event_process(struct unic_dev *unic_dev)
{
	struct unic_act_info *act_info = &unic_dev->act_info;
	struct net_device *netdev = unic_dev->comdev.netdev;
	int ret;

	if (test_bit(UNIC_STATE_DISABLED, &unic_dev->state)) {
		unic_err(unic_dev,
			 "failed to process deactivate event, device is not ready.\n");
		return;
	}

	rtnl_lock();

	if (test_bit(UNIC_STATE_DEACTIVATE, &unic_dev->state))
		goto unlock;

	/* when deactivate event occurs, set flag to true to prevent
	 * periodic tasks changing promisc
	 */
	mutex_lock(&act_info->mutex);
	act_info->deactivate = true;
	mutex_unlock(&act_info->mutex);

	if (unic_dev_eth_mac_supported(unic_dev))
		unic_deactivate_mac_table(unic_dev);

	ret = unic_activate_promisc_mode(unic_dev, false);
	if (ret)
		unic_warn(unic_dev, "failed to close promisc, ret = %d.\n", ret);
	else
		set_bit(UNIC_VPORT_STATE_PROMISC_CHANGE, &unic_dev->vport.state);

	/* if network interface has already been stopped,
	 * no need to stop again by deactivate event
	 */
	if (test_bit(UNIC_STATE_DOWN, &unic_dev->state))
		goto out;

	unic_net_stop_no_link_change(netdev);

out:
	set_bit(UNIC_STATE_DEACTIVATE, &unic_dev->state);
unlock:
	rtnl_unlock();
}

static void unic_activate_handler(struct auxiliary_device *adev, bool activate)
{
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);

	unic_info(unic_dev, "receive %s event callback.\n",
		  activate ? "activate" : "deactivate");

	if (activate)
		unic_activate_event_process(unic_dev);
	else
		unic_deactivate_event_process(unic_dev);
}

static void unic_rack_port_reset(struct unic_dev *unic_dev, bool link_up)
{
	if (link_up)
		unic_dev->hw.mac.link_status = UNIC_LINK_STATUS_UP;
	else
		unic_dev->hw.mac.link_status = UNIC_LINK_STATUS_DOWN;
}

static void unic_port_handler(struct auxiliary_device *adev, bool link_up)
{
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);
	struct net_device *netdev = unic_dev->comdev.netdev;

	if (!netif_running(netdev))
		return;

	unic_rack_port_reset(unic_dev, link_up);
}

static struct ubase_ctrlq_event_nb unic_ctrlq_events[] = {
	{
		.service_type = UBASE_CTRLQ_SER_TYPE_IP_ACL,
		.opcode = UBASE_CTRLQ_OPC_NOTIFY_IP,
		.crq_handler = unic_handle_notify_ip_event,
	}
};

static void unic_unregister_ctrlq_event(struct auxiliary_device *adev,
					u32 ctrlq_crq_event_num)
{
	u32 i;

	for (i = 0; i < ctrlq_crq_event_num; i++)
		ubase_ctrlq_unregister_crq_event(adev,
						 unic_ctrlq_events[i].service_type,
						 unic_ctrlq_events[i].opcode);
}

static int unic_register_ctrlq_event(struct auxiliary_device *adev)
{
	int ret;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(unic_ctrlq_events); i++) {
		unic_ctrlq_events[i].back = adev;
		ret = ubase_ctrlq_register_crq_event(adev, &unic_ctrlq_events[i]);
		if (ret) {
			dev_err(adev->dev.parent,
				"failed to register ctrlq event[%u], ret = %d.\n",
				i, ret);
			unic_unregister_ctrlq_event(adev, i);
			return ret;
		}
	}

	return 0;
}

static struct ubase_crq_event_nb unic_crq_events[] = {
	{
		.opcode = UBASE_OPC_QUERY_LINK_STATUS,
		.crq_handler = unic_handle_link_status_event,
	},
};

static void unic_unregister_crq_event(struct auxiliary_device *adev,
				      u32 crq_event_num)
{
	u32 i;

	for (i = 0; i < crq_event_num; i++)
		ubase_unregister_crq_event(adev, unic_crq_events[i].opcode);
}

static int unic_register_crq_event(struct auxiliary_device *adev)
{
	int ret;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(unic_crq_events); i++) {
		unic_crq_events[i].back = adev;

		ret = ubase_register_crq_event(adev, &unic_crq_events[i]);
		if (ret) {
			dev_err(adev->dev.parent,
				"failed to register crq event[%u], ret = %d.\n",
				i, ret);
			unic_unregister_crq_event(adev, i);
			return ret;
		}
	}

	return 0;
}

static void unic_unregister_ae_event(struct auxiliary_device *adev,
				     u8 asyn_event_num)
{
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);
	u8 i;

	for (i = 0; i < asyn_event_num; i++)
		ubase_event_unregister(adev, &unic_dev->ae_nbs[i]);
}

static int unic_ae_jetty_level_error(struct notifier_block *nb,
				     unsigned long event, void *data)
{
	struct ubase_event_nb *ev_nb = container_of(nb,
						    struct ubase_event_nb, nb);
	struct auxiliary_device *adev = (struct auxiliary_device *)ev_nb->back;
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);
	struct ubase_aeq_notify_info *info = data;
	u32 queue_num;

	/* Normally, UNIC does not report such abnormal events,
	 * but in order to maintain its scalability,
	 * unic reserves the reset processing of such events.
	 */
	queue_num = info->aeqe->event.queue_event.num;
	unic_err(unic_dev,
		 "recv jetty level error, event_type = 0x%x, sub_type = 0x%x, queue_num = %u.\n",
		 info->event_type, info->sub_type, queue_num);

	ubase_reset_event(adev, UBASE_UE_RESET);

	return 0;
}

static int unic_register_ae_event(struct auxiliary_device *adev)
{
	struct ubase_event_nb unic_ae_nbs[UNIC_AE_LEVEL_NUM] = {
		{
			UBASE_DRV_UNIC,
			UBASE_EVENT_TYPE_JETTY_LEVEL_ERROR,
			{ unic_ae_jetty_level_error },
			adev
		},
	};
	struct unic_dev *unic_dev = dev_get_drvdata(&adev->dev);
	int ret;
	u8 i;

	for (i = 0; i < ARRAY_SIZE(unic_ae_nbs); i++) {
		unic_dev->ae_nbs[i] = unic_ae_nbs[i];
		ret = ubase_event_register(adev, &unic_dev->ae_nbs[i]);
		if (ret) {
			dev_err(adev->dev.parent,
				"failed to register asyn event[%u], ret = %d.\n",
				unic_dev->ae_nbs[i].event_type, ret);
			unic_unregister_ae_event(adev, i);
			return ret;
		}
	}

	return ret;
}

int unic_register_event(struct auxiliary_device *adev)
{
	int ret;

	ret = unic_register_ae_event(adev);
	if (ret)
		return ret;

	ret = unic_register_crq_event(adev);
	if (ret)
		goto unregister_ae;

	ret = unic_register_ctrlq_event(adev);
	if (ret)
		goto unregister_crq;

	ubase_port_register(adev, unic_port_handler);
	ubase_reset_register(adev, unic_reset_handler);
	ubase_activate_register(adev, unic_activate_handler);

	return 0;

unregister_crq:
	unic_unregister_crq_event(adev, ARRAY_SIZE(unic_crq_events));
unregister_ae:
	unic_unregister_ae_event(adev, UNIC_AE_LEVEL_NUM);

	return ret;
}

void unic_unregister_event(struct auxiliary_device *adev)
{
	ubase_activate_unregister(adev);
	ubase_reset_unregister(adev);
	ubase_port_unregister(adev);
	unic_unregister_ctrlq_event(adev, ARRAY_SIZE(unic_ctrlq_events));
	unic_unregister_crq_event(adev, ARRAY_SIZE(unic_crq_events));
	unic_unregister_ae_event(adev, UNIC_AE_LEVEL_NUM);
}
