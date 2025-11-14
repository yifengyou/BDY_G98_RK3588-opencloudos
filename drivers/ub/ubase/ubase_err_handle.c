// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2025 HiSilicon Technologies Co., Ltd. All rights reserved.
 *
 */

#include "ubase_cmd.h"
#include "ubase_dev.h"
#include "ubase_reset.h"
#include "ubase_err_handle.h"

static void ubase_notify_himac_reset(struct ubase_dev *udev)
{
	struct ubase_cmd_buf in;
	int ret;

	__ubase_fill_inout_buf(&in, UBASE_OPC_HIMAC_RESET, false, 0, NULL);

	ret = __ubase_cmd_send_in(udev, &in);
	if (ret)
		ubase_err(udev,
			  "failed to send himac reset cmd, ret = %d.\n", ret);
}

void ubase_errhandle_service_task(struct ubase_delay_work *ubase_work)
{
	struct ubase_dev *udev;

	udev = container_of(ubase_work, struct ubase_dev, service_task);
	if (!test_and_clear_bit(UBASE_SERVICE_STATE_ERR_SCHED,
				&ubase_work->state))
		return;

	if (!ubase_dev_err_handle_supported(udev)) {
		ubase_err(udev, "not support err handle processing.\n");
		return;
	}

	if (test_and_clear_bit(UBASE_STATE_HIMAC_RESETTING_B, &udev->state_bits)) {
		ubase_err(udev, "ras occurred, ubase need to reset himac.\n");
		ubase_notify_himac_reset(udev);
		udev->reset_stat.himac_reset_cnt++;
		return;
	}

	if (test_and_clear_bit(UBASE_STATE_PORT_RESETTING_B, &udev->state_bits)) {
		ubase_err(udev, "ras occurred, ubase need to reset port.\n");
		ubase_port_reset(udev);
	}
}
